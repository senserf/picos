/*
	Copyright 2002-2023 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// CC1350 RF driver operating in the so-called proprietary mode ===============
// ============================================================================

// Needed by some library includes
#define	DEVICE_FAMILY	cc13x0

#include <driverlib/rf_common_cmd.h>
#include <driverlib/rf_prop_cmd.h>
#include <driverlib/rf_prop_mailbox.h>
#include <driverlib/rf_data_entry.h>
#include <driverlib/rfc.h>
#include <inc/hw_rfc_rat.h>

// ============================================================================
// Notes and issues:
// ============================================================================
//
// Check if using RF memory allows us to lower the power budget. This is tricky
// because we have to circumvent the patch area.
//
// How can we prevent interrupting reception in progress by a transmission?
// We can have reception with a timeout while waiting for sync, which should do
// for synchronized acks (and stuff like that). I don't think we can detect
// reception and postpone transmission any other way.
//
// Cold-starting up the transmitter takes 5ms; check the reference code again
// Can we do better?
//
// ============================================================================

#if 0
#define	TIME_COLDSTART		do { \
		GPIO_setOutputEnableDio (IOID_29, GPIO_OUTPUT_ENABLE); \
		GPIO_clearDio (IOID_29); \
	} while (0)
#define	TIME_COLDSTART_ON	GPIO_setDio (IOID_29)
#define	TIME_COLDSTART_OFF	GPIO_clearDio (IOID_29)
#else
#define	TIME_COLDSTART		CNOP
#define	TIME_COLDSTART_ON	CNOP
#define	TIME_COLDSTART_OFF	CNOP
#endif

// ============================================================================

// The RAT clock rate is 4MHz
#define	RAT_TICKS_PER_US		4

// TX command header size (shared by TX and ADV)
#define	TX_ADV_HEADER_SIZE		14

#include <rf_patches/rf_patch_cpe_genfsk.h>
#include <rf_patches/rf_patch_rfe_genfsk.h>

#include "sysio.h"
#include "tcvphys.h"
#include "cc1350.h"

// Radio parameter settings by TI
#define	mkmk_eval
#include "smartrf_settings.h"
#undef	mkmk_eval

#define	DETECT_HANGUPS	100

// ============================================================================

static	int paylen = 0;			// Current TX payload length

static byte	rbuffl = 0,		// Input buffer length
		dstate = 0;		// Driver state

#if RADIO_LBT_SENSE_TIME > 0
static byte	txtries = 0;		// Number of TX attempts for current pkt
#endif

#if RADIO_LBT_MAX_TRIES > 255
#undef	RADIO_LBT_MAX_TRIES
#define	RADIO_LBT_MAX_TRIES	255
#endif

// State flags
#define	DSTATE_RXON	0x01	// Rx formally turned on
#define	DSTATE_RXAC	0x02	// Rx physically active
#define	DSTATE_TXIP	0x04	// Tx in progress
#define	DSTATE_RFON	0x10	// Device physically on
#define	DSTATE_IRST	0x80	// Reset request

// The number of simultaneous receive buffers
#define	NRBUFFS		2

// The secret (undocumented) command (stolen from tirtos)
#define	RF_CMD0		0x0607

static byte	vrate = RADIO_BITRATE_INDEX,
		channel = RADIO_DEFAULT_CHANNEL;

#if RADIO_DEFAULT_POWER <= 7
// ============================================================================
// Power can be changed; the max power must be hardwired (with rate 0) for
// long range operation
// ============================================================================

static byte txpower;

// Used to be const, but we want to be able to modify the values from the
// default settings and also be able to revert to the defaults
static word patable_def [] = CC1350_PATABLE;	// Default power settings
static address patable = patable_def;		// Effective settings

#endif

static word	physid,
		statid = 0,
		bckf_timer = 0;

static cc1350_rfparams_t params = {
//
// These are the parameters settable with PHYSOPT_SETPARAMS, see cc1350.h
//
	// Delay before the device goes off if the RX is off and the output
	// queue dries out. Expressed in PicOS msecs.
	.offdelay	= RADIO_DEFAULT_OFFDELAY,
};

typedef struct {
		word	ps,	// prescale
			rw;	// rate word
		// The actual exact rate computed from the raw parameters, so
		// we don't have to compute it
		lword	rt;
} ratable_t;

static const ratable_t ratable [] = CC1350_RATABLE;

#define	NRATES	(sizeof (ratable)/sizeof (ratable_t))

// Pointer to the queue of reception buffers
#define	rbuffs 		(RF_cmdPropRx . pQueue)

// Reception statistics (can be extracted via PHYSOPT_ERROR)
static rfc_propRxOutput_t	rxstat;

// Events
static aword drvprcs, qevent, to_trigger;
// This can be any address
#define	txevent ((aword)&to_trigger)

// ============================================================================

#if 0
// A command to retrieve firmware version and stuff
static rfc_CMD_GET_FW_INFO_t	cmd_gfi = { .commandNo = CMD_GET_FW_INFO };
#endif

#if 0
// Ping command
static rfc_CMD_PING_t		cmd_pin = { .commandNo = CMD_PING };
#endif

// Command to start the RAT (timer)
static rfc_CMD_SYNC_START_RAT_t	cmd_srt = { .commandNo = CMD_SYNC_START_RAT };

#if RADIO_LBT_SENSE_TIME > 0

#if RADIO_LBT_CORR_PERIOD == 0
#define	ENABLE_CORR_LBT	0
#else
#define	ENABLE_CORR_LBT	1
#endif

// Command for carrier sense (LBT)
static volatile rfc_CMD_PROP_CS_t cmd_cs = {
		.commandNo = CMD_PROP_CS,
		// Chained to TX
		.pNextOp = (rfc_radioOp_t*) &RF_cmdPropTx,
		.startTime = 0,
		.startTrigger.triggerType = 0,	// Immediately
		.startTrigger.bEnaCmd = 0,
		.startTrigger.triggerNo = 0,
		.startTrigger.pastTrig = 0,
		// True means channel busy
		.condition.rule = COND_STOP_ON_TRUE,
		.condition.nSkip = 0,
		// Keep everything running, no matter what
		.csFsConf.bFsOffIdle = 0,
		.csFsConf.bFsOffBusy = 0,
		// What we measure
		.csConf.bEnaRssi = 1,
		.csConf.bEnaCorr = ENABLE_CORR_LBT,
		.csConf.operation = 0,	// EITHER (as opposed to AND)
		.csConf.busyOp = 1,	// End sensing when channel found busy
		.csConf.idleOp = 0,	// Continue when idle
		.csConf.timeoutRes = 0,	// Timeout on invalid = busy (not idle)
		// Converted from absolute to signed
		.rssiThr = (byte)(RADIO_LBT_RSSI_THRESHOLD - 128),
		.numRssiIdle = RADIO_LBT_RSSI_NIDLE,
		.numRssiBusy = RADIO_LBT_RSSI_NBUSY,
		.corrPeriod = RADIO_LBT_CORR_PERIOD,
		.corrConfig.numCorrInv = RADIO_LBT_CORR_NINVD,
		.corrConfig.numCorrBusy = RADIO_LBT_CORR_NBUSY,
		// How to end: time relative to start
		.csEndTrigger.triggerType = TRIG_REL_START,
		.csEndTrigger.bEnaCmd = 0,
		.csEndTrigger.triggerNo = 0,
		.csEndTrigger.pastTrig = 0,
		// Converted from milliseconds to RAT 4MHz ticks
		.csEndTime = (RADIO_LBT_SENSE_TIME * RAT_TICKS_PER_US * 1024),
	};
#endif /* RADIO_LBT_SENSE > 0 */

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)

#if RADIO_DEFAULT_POWER > 7
#error "S: PXOPTIONS requires RADIO_DEFAULT_POWER <= 7"
#endif

static rfc_CMD_SET_TX_POWER_t cmd_sp = {
		.commandNo = CMD_SET_TX_POWER
	};

#endif

#if (RADIO_OPTIONS & RADIO_OPTION_RBKF)
// Accumulated backoff
static word	bckf_lbt = 0;
#define	update_bckf_lbt(v)	bckf_lbt += (v)
#else
#define	update_bckf_lbt(v)	CNOP
#endif
		
// As far as I can tell, the trim structure can be set up once (RF need not be
// powered on) and then written to RF core when it comes up, so I am splitting
// the operation into two parts ... we shall see
static rfTrim_t rfTrim;

// ============================================================================

static void issue_cmd (lword cmd) {
//
// Issue a command to the radio, wait for (immediate) status, detect low-level
// problems (no interrupts)
//
	int res;

#ifdef	DETECT_HANGUPS
	int cnt = DETECT_HANGUPS * 900;
#endif
	while (1) {
		if ((res = RFCDoorbellSendTo (cmd) & 0xff) == 0x01)
			return;
		// We ignore the (larger) status for now; do we need it?
		if (res != 0x86)
			// Some fundamental problem
			syserror (EHARDWARE, "rt1");
		// Otherwise it means: previous command not completed, so keep
		// trying
#ifdef	DETECT_HANGUPS
		if (cnt-- == 0) {
			diag ("HUP %lx %lx", cmd, res);
			syserror (EHARDWARE, "hang ic0");
		}
#endif
		udelay (1);
	}
}

static void wait_cmd (rfc_radioOp_t *cmd, lword tstat, lword timeout) {
//
// Wait for a radio operation command to complete (for those commands for
// which we want to wait without involving interrupts)
//
	while (1) {
		if (cmd->status == tstat)
			// Target status reached
			return;
		if (timeout-- == 0) {
#ifdef DETECT_HANGUPS
			diag ("HUP %lx %lx %lx", cmd->commandNo,
				cmd->status, tstat);
#endif
			syserror (EHARDWARE, "rt2");
		}
		udelay (1);
	}
}

static void enable_event (aword ev) {
//
// Make the specified event trigger-able by an RF interrupt
//
	to_trigger = ev;
	RFCCpeIntClear (LWNONE);
	RFCCpe0IntEnable (RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE |
				RFC_DBELL_RFCPEIEN_RX_OK |
					RFC_DBELL_RFCPEIEN_INTERNAL_ERROR);
}

// ============================================================================

static void rf_on () {
//
// Turn the radio on
//
	if (dstate & DSTATE_RFON)
		return;

	TIME_COLDSTART_ON;

#ifdef	RADIO_PINS_ON
	// Any special pin settings?
	RADIO_PINS_ON;
#endif
	// The oscillator (do we need this?); no, this is the default, anyway
	// OSCXHfPowerModeSet (HIGH_POWER_XOSC);

	// The default setting of OSC clock source is OSC_RCOSC_HF; we shall
	// revert to it when the radio is switched off
	OSCHF_TurnOnXosc();
	while (!OSCHF_AttemptToSwitchToXosc ());
	//do { udelay (10); } while (!OSCHF_AttemptToSwitchToXosc ());

	// Make sure RAT is driven by RTC
	HWREGBITW (AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) =
		1;

	// Power up the domain (there's no peripheral for RF)
	__pi_ondomain (PRCM_DOMAIN_RFCORE);

	// Enable the clock
	RFCClockEnable ();

	// Set up some magic for the patches to work
	issue_cmd (
		CMDR_DIR_CMD_2BYTE (RF_CMD0,
			RFC_PWR_PWMCLKEN_MDMRAM | RFC_PWR_PWMCLKEN_RFERAM));
	// Apply the patches
	rf_patch_cpe_genfsk ();
	rf_patch_rfe_genfsk ();	
	// Undo the magic
        issue_cmd (CMDR_DIR_CMD_2BYTE (RF_CMD0, 0));

#if 1
        // Initialize bus request; AFAICT, this is only needed if we want to
	// deep sleep while the radio is sending data to our RAM
	issue_cmd (CMDR_DIR_CMD_1BYTE (CMD_BUS_REQUEST, 1));
#endif

	RFCAdi3VcoLdoVoltageMode (true);

	// This must be done with RF core on, on every startup
       	RFCRfTrimSet (&rfTrim);

#if 0
	// Show up the firmware version on first power up
	issue_cmd ((lword)&cmd_gfi);
	diag ("RF: %x %x %x %x",
					cmd_gfi.versionNo,
					cmd_gfi.startOffset,
					cmd_gfi.freeRamSz,
					cmd_gfi.availRatCh);
#endif

#if 0
	// Check connection?
	issue_cmd ((lword)&cmd_pin);
#endif

	// Setup the "proprietary" mode
	issue_cmd ((lword)&RF_cmdPropRadioDivSetup);
	// Wait until done
	wait_cmd ((rfc_radioOp_t*)&RF_cmdPropRadioDivSetup, PROP_DONE_OK,
		10000);

	// Start RAT
	issue_cmd ((lword)&cmd_srt);

	// Power up frequency synthesizer; apparently this is needed if
	// CMD_PROP_RADIO_DIV_SETUP.bNoFsPowrup == 1 (it is zero, as set by
	// SmartRF Studio)

	// Set the frequency
	issue_cmd ((lword)&RF_cmdFs);
	wait_cmd ((rfc_radioOp_t*)&RF_cmdFs, DONE_OK, 10000);

	_BIS (dstate, DSTATE_RFON);

	// Interrupts, we only care about reception and system error (?)
	HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIEN) = 0;
	HWREG (RFC_DBELL_BASE+RFC_DBELL_O_RFCPEIFG) = 0;
	IntEnable (INT_RFC_CPE_0);

	LEDI (0, 1);

#if (RADIO_OPTIONS & RADIO_OPTION_RBKF)
	bckf_lbt = 0;
#endif
	TIME_COLDSTART_OFF;
}

static void rx_de () {
//
// Deactivate the receiver
//
	if ((dstate & DSTATE_RXAC)) {
		issue_cmd (CMDR_DIR_CMD (CMD_ABORT));
		_BIC (dstate, DSTATE_RXAC);
		to_trigger = 0;
	}
}

static void rf_off () {
//
// Turn the radio off
//
	if ((dstate & DSTATE_RFON) == 0)
		return;

	TIME_COLDSTART_ON;

	rx_de ();

	IntDisable (INT_RFC_CPE_0);

	RFCAdi3VcoLdoVoltageMode (false);

	// Without this, the domain isn't truly switched off
	RFCSynthPowerDown ();

	RFCClockDisable ();

	__pi_offdomain (PRCM_DOMAIN_RFCORE);

	// Decouple RAT from RTC
	HWREGBITW (AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) =
		0;

	OSCHF_SwitchToRcOscTurnOffXosc ();

#ifdef	RADIO_PINS_OFF
	RADIO_PINS_OFF;
#endif
	_BIC (dstate, DSTATE_RFON);
	LEDI (0, 0);

	TIME_COLDSTART_OFF;
}

static void rx_ac () {
//
// Activate the receiver
//
	rfc_dataEntryGeneral_t *re;
	int i;

	// One hard prereq
	rf_on ();
	if (dstate & DSTATE_RXAC)
		return;

	// Make sure the buffers are clean
	for (re = (rfc_dataEntryGeneral_t*) (rbuffs->pCurrEntry), i = 0;
    	    i < NRBUFFS; i++, re = (rfc_dataEntryGeneral_t*)(re->pNextEntry))
			re->status = DATA_ENTRY_PENDING;

	_BIS (dstate, DSTATE_RXAC);
	issue_cmd ((lword)&RF_cmdPropRx);
}

static void plugch () {
//
// Insert the channel number as the right frequency in the proper place
//
	RF_cmdFs.frequency = 
		// Channel number is just the megahertz increment
		RF_cmdPropRadioDivSetup.centerFreq = CC1350_BASEFREQ + channel;
}

#if RADIO_BITRATE_INDEX > 0

static void plugrt () {
//
// Insert the rate parameters
//
	RF_cmdPropRadioDivSetup.symbolRate.preScale = ratable [vrate] . ps;
	RF_cmdPropRadioDivSetup.symbolRate.rateWord = ratable [vrate] . rw;
}

#endif

static int rx_int_enable () {

	rfc_dataEntryGeneral_t *db;
	int i, pl, nr;

	// This clears all interrupt flags and enables those that we want; this
	// is different than (e.g.) in CC1100/MSP430, because events can be
	// lost (the clear operation may remove an event that we haven't looked
	// at yet); so we do the reception with interrupts enabled; if an
	// interrupt arrives while we are looking, it will just unblock the
	// driver thread which will force (later) another invocation of this
	// function. But it is important to always make sure that when the
	// function is being invoked, the driver thread is already waiting for
	// qevent.

	enable_event (qevent);

#define	__dp	(&(db->data))
#define	__pk	((address)(__dp + 2))
#define	__ni	(__pk [0])

	// Receive
	for (db = (rfc_dataEntryGeneral_t*) (rbuffs->pCurrEntry), i = nr = 0;
	    i < NRBUFFS; i++, db = (rfc_dataEntryGeneral_t*)(db->pNextEntry)) {
		if (db->status == DATA_ENTRY_FINISHED) {
			LEDI (2, 1);

			// Consistency checks
			if (
				// Physical length matches logical length ...
				     __dp [0] == __dp [1] + 3 
				// ... and the packet fits into RX buffer ...
				  && __dp [1] <= rbuffl
				// ... and it has at least 2 bytes of payload
				// and the payload length is even ...
			    	  && __dp [1] >= 2 && (__dp [1] & 1) == 0
				// ... and the NetId check passes which means
				// that:
#if 1
				  && (
					// the NetIds are same ...
					     __ni == statid
					// ... or the packet is global ...
					  || __ni == 0
					// ... or we are promiscuous
					  || statid == 0 || statid == 0xffff
				  )
#endif
			    ) {
				// Packet length plus two extra bytes at the
				// end
				pl = __dp [1] + 2;
				// Swap RSS and status, actually move RSS one
				// byte up and zero the status (for now); RSS
				// is unbiased to nonnegative (as per usual)
				add_entropy (__dp [pl]);
				__dp [pl + 1] = __dp [pl] + 128;
				__dp [pl] = 0;
				nr++;
				tcvphy_rcv (physid, __pk, pl);
			}
#ifdef	DETECT_HANGUPS
			else {
				diag ("BAD RX");
			}
#endif
			// Mark the buffer as free
			db->status = DATA_ENTRY_PENDING;
		}
	}

#undef __dp
#undef __pk
#undef __ni

	if (nr) {
		gbackoff_rcv;
	}

	LEDI (2, 0);
	// State OK
	return NO;
}

void RFCCPE0IntHandler (void) {
//
// Any interrupt (of the enabled kind) just unhangs the driver thread
//
	if (HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) &
	    RFC_DBELL_RFCPEIFG_INTERNAL_ERROR)
		// Internal error
		_BIS (dstate, DSTATE_IRST);


	// Clear all interrupts (why do they do it in a loop?); must be some
	// magic, because it didn't work for me the sane way
	RFCCpeIntClear (LWNONE);
	// HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) = 0;
	// Disable
	RFCCpeIntDisable (LWNONE);
	// HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIEN) = 0;

	if (to_trigger) {
		p_trigger (drvprcs, to_trigger);
		// Only once ?
		to_trigger = 0;
	}

	RISE_N_SHINE;
}

// ============================================================================

#define	DR_LOOP		0
#define	DR_XMIT		1
#define	DR_FRCE		2
#define	DR_FXMT		3
#define	DR_GOOF		4
#define	DR_WRES		5
#define	DR_WURG		6
#define	DR_WXMT		7

#define	the_packet 		(RF_cmdPropTx.pPkt)
#define	the_packet_length 	(RF_cmdPropTx.pktLen)

#ifdef	DETECT_HANGUPS
static	lword txcounter;
#endif

thread (cc1350_driver)

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
	address ppm;
	word pcav;
#endif
	entry (DR_LOOP)

		// We come here for anything that happens; dstate tells us
		// where we are statewise:
		//
		// RXON = we want RX to be on (modified by PHYSOPT only)
		// RFON = the device is on
		// RXAC	= RX active
		// IRST = reset requested
		// TXIP = packet being transmitted
DR_LOOP__:
		// Make sure the packet is unlocked whenever we get here; it
		// is definitely not being transmitted
		_BIC (dstate, DSTATE_TXIP);
		
		// Reset request is easy; make sure this is the only place
		// where reset is actually handled (and the flag is cleared)
		if (dstate & DSTATE_IRST) {
			// Remove the flag
			_BIC (dstate, DSTATE_IRST);
			// Turn the device off; this will clear: RFON, RXAC
			rf_off ();
			// Things should get back to where they were in the
			// following sequence
		}

		if (dstate & DSTATE_RXON) {
			// The RX is ON
			if ((dstate & DSTATE_RXAC) == 0) {
				rx_ac ();
			}
		} else {
			// RX is meant to be off, make sure it is
			rx_de ();
		}

		// We have gotten the state right, see if there is a packet
		// to transmit
		if (paylen == 0) {
			// We have to look at the queue. If paylen != 0, it
			// means that we have a previous packet (already
			// extracted from the queue)
			if ((the_packet = (byte*)tcvphy_get (physid, &paylen))
			    != NULL) {
				// Ignore the two extra bytes right away, we
				// have no use for them
				paylen -= 2;
				sysassert (paylen <= rbuffl && paylen > 0 &&
				    (paylen & 1) == 0, "cc13 py");

				LEDI (1, 1);

				if (statid != 0xffff)
					// Insert NetId
					((address)the_packet) [0] = statid;

				// Plug it into the command
				the_packet_length = (byte) paylen;
			}
		}

		if (paylen == 0) {
			// If paylen is still zero, there is nothing to
			// transmit at the moment
			wait (qevent, DR_LOOP);
			if (dstate & DSTATE_RXAC) {
				// Try to receive. Note that we have
				// to issue wait before this operation because
				// of the interrupt race
				if (rx_int_enable ())
					goto DR_LOOP__;
				// For lost interrupts (test)
				// delay (1024, DR_LOOP);
			} else if (dstate & DSTATE_RFON) {
				// Nothing to transmit and the receiver is OFF,
				// turn the radio off after a delay.
				delay (params . offdelay, DR_GOOF);
			}
			// Do nothing until something happens
			release;
		}

		if (bckf_timer) {
			// Backoff wait
#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
Bkf:
#endif
			wait (qevent, DR_LOOP);
			delay (bckf_timer, DR_LOOP);
			if (dstate & DSTATE_RXAC) {
				// Don't forget about receiving packets instead
				// of sitting on your hands
				if (rx_int_enable ())
					goto DR_LOOP__;
			}
			release;
		}

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
		// Check for CAV requested in the packet
		ppm = (address)(the_packet + paylen);
		if ((pcav = (*ppm) & 0x0fff)) {
			// Remove for subsequent turns
			*ppm &= ~0x0fff;
			utimer_set (bckf_timer, pcav);
			goto Bkf;
		}
#endif
		// Make sure the device is ready
		rf_on ();
		rx_de ();

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
		// Set the transmit power according to what the packet requests
		if ((pcav = patable [(*ppm >> 12) & 0x7]) !=
		    RF_cmdPropRadioDivSetup.txPower) {
			// Need to change
		    	RF_cmdPropRadioDivSetup.txPower = 
				cmd_sp.txPower = pcav;
			issue_cmd ((lword)&cmd_sp);
		}
#endif

#if (RADIO_OPTIONS & RADIO_OPTION_RBKF)
		// The obscure optional report (for tests only)
		((address)(the_packet)) [1] = bckf_lbt;
#endif
		// Lock the packet
		_BIS (dstate, DSTATE_TXIP);

#if RADIO_LBT_SENSE_TIME > 0

// ============================================================================
// LBT ON =====================================================================
// ============================================================================

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
		if (*ppm & 0x8000) {
			// LBT off requested by packet
			txtries = RADIO_LBT_MAX_TRIES;
			proceed (DR_FRCE);
		}
#endif
		cmd_cs . status = RF_cmdPropTx . status = 0;
		// Carrier sense + transmit if OK
		issue_cmd ((lword)&cmd_cs);

	entry (DR_XMIT)

		if (dstate & DSTATE_IRST)
			goto DR_LOOP__;

		wait (txevent, DR_XMIT);
		enable_event (txevent);
		if ((cmd_cs.status & 0x3000) != 0x3000)
			// Wait for command completion
			release;

		// Check the CS status
		if (cmd_cs.status == PROP_DONE_BUSY || 
		    cmd_cs.status == PROP_DONE_BUSYTIMEOUT) {
			// Carrier sense failed
			if (txtries >= RADIO_LBT_MAX_TRIES)
				// Forced TX, no CS
				proceed (DR_FRCE);
			txtries++;
			// Backoff
			// diag ("BUSY");
			gbackoff_lbt;
			update_bckf_lbt (bckf_timer);
			goto DR_LOOP__;
		}
TXEnd:
		// Release the packet
		tcvphy_end ((address)the_packet);
		paylen = 0;

#if (RADIO_OPTIONS & RADIO_OPTION_RBKF)
		bckf_lbt = 0;
#endif

#if RADIO_LBT_SENSE_TIME > 0
		txtries = 0;
#endif
		LEDI (1, 0);
		gbackoff_xmt;
		// If we don't care to delay after xmt, then perhaps we
		// should send whatever pending packets we have in the queue
		// without restarting the receiver
		goto DR_LOOP__;

	entry (DR_FRCE)

		// Transmit without LBT
		RF_cmdPropTx . status = 0;
		issue_cmd ((lword)&RF_cmdPropTx);

	entry (DR_FXMT)

		if (dstate & DSTATE_IRST)
			goto DR_LOOP__;

		wait (txevent, DR_FXMT);
		enable_event (txevent);
		if ((RF_cmdPropTx.status & 0x3000) != 0x3000)
			// Wait for command completion
			release;

		// Check the CS status
		if (RF_cmdPropTx.status == PROP_DONE_OK)
			goto TXEnd;

		// Something wrong
		delay (1, DR_LOOP);
		release;

#else	/* RADIO_LBT_SENSE_TIME */

// ============================================================================
// LBT OFF ====================================================================
// ============================================================================

		RF_cmdPropTx . status = 0;
		issue_cmd ((lword)&RF_cmdPropTx);

	entry (DR_XMIT)

		if (dstate & DSTATE_IRST)
			goto DR_LOOP__;

		wait (txevent, DR_XMIT);
		enable_event (txevent);
		if ((RF_cmdPropTx.status & 0x3000) != 0x3000)
			// Wait for command completion
			release;

		// Check the CS status
		if (RF_cmdPropTx.status != PROP_DONE_OK) {
			// Something wrong
			delay (1, DR_LOOP);
			release;
		}

		// Release the packet
		tcvphy_end ((address)the_packet);
		paylen = 0;

#if (RADIO_OPTIONS & RADIO_OPTION_RBKF)
		bckf_lbt = 0;
#endif

#if RADIO_LBT_SENSE_TIME > 0
		txtries = 0;
#endif
		LEDI (1, 0);
		gbackoff_xmt;
		goto DR_LOOP__;

#endif	/* RADIO_LBT_SENSE_TIME */

// ============================================================================
// END LBT ====================================================================
// ============================================================================

	entry (DR_GOOF)

		// Inactivity timeout with RX OFF
		if (!(dstate & DSTATE_IRST) && (dstate & DSTATE_RXON) == 0 &&
		    tcvphy_top (physid) == NULL)
			rf_off ();

		goto DR_LOOP__;
endthread

#undef	the_packet
#undef	the_packet_length

#undef	DR_LOOP
#undef	DR_XMIT
#undef	DR_FRCE
#undef	DR_FXMT
#undef	DR_GOOF
#undef	DR_WRES
#undef	DR_WURG
#undef	DR_WXMT

// ============================================================================

static void init_rbuffs () {
//
// Set up the receive buffer pool (called once upon startup)
//
	int i;
	rfc_dataEntryGeneral_t *re, *da, *db;

	i = 0;
	while (1) {
		db = (rfc_dataEntryGeneral_t*)
			umalloc (sizeof (rfc_dataEntryGeneral_t) - 1 + rbuffl +
				2);
			// 2 extra bytes needed on top of two extra bytes
			// from mbs; at this stage rbuffl = max paylen + 2
#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
		if (db == NULL)
			syserror (EMALLOC, "cc13");
#endif
		db->status = DATA_ENTRY_PENDING;

		if (i)
			// Link to previous
			da->pNextEntry = (byte*) db;
		else
			re = db;

		db->config.type = 0;	// General (data in structure)
		db->config.lenSz = 1;	// Single byte for length
		db->config.irqIntv = 0;	// Irrelevant
		db->length = rbuffl + 2;
		if (++i == NRBUFFS)
			break;
		da = db;
	}

	// Link last to first
	db->pNextEntry = (byte*) re;

	// This is an alias for RF_cmdPropRx.pQueue
	rbuffs = (dataQueue_t*) umalloc (sizeof (dataQueue_t));
		
#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (rbuffs == NULL)
		syserror (EMALLOC, "cc13");
#endif
	rbuffs->pCurrEntry = (byte*) re;
	rbuffs->pLastEntry = NULL;

	// Prepare the command to start reading
	RF_cmdPropRx . pOutput = (byte*) &rxstat;

	// I guess this is how we make sure the command runs in a loop
	RF_cmdPropRx . pktConf . bRepeatOk = 1;
	RF_cmdPropRx . pktConf . bRepeatNok = 1;

	// Auto-discard rejects from the queue
	RF_cmdPropRx . rxConf . bAutoFlushIgnored = 1;
	RF_cmdPropRx . rxConf . bAutoFlushCrcErr = 1;

	// RSSI appended to the received packet; also, by default, the CRC
	// is discarded and a status byte is appended
	RF_cmdPropRx . rxConf . bAppendRssi = 1;

	// Make rbuffl exactly equal to max payload length
	rbuffl -= 2;
}

static void set_driver_params (cc1350_rfparams_t *pt) {

	if (pt) {
		// From the table
		memcpy (&params, pt, sizeof (cc1350_rfparams_t));
	} else {
		// The defaults
		params . offdelay = RADIO_DEFAULT_OFFDELAY;
	}
}

static int option (int opt, address val) {

	int ret = 0;

	switch (opt) {

		case PHYSOPT_STATUS:

			ret = 2 | ((dstate & DSTATE_RXON) != 0);
			goto RVal;

		case PHYSOPT_RXON:

			_BIS (dstate, DSTATE_RXON);
OREvnt:
			p_trigger (drvprcs, qevent);

		case PHYSOPT_TXON:
		case PHYSOPT_TXOFF:
		case PHYSOPT_TXHOLD:

			goto RRet;

		case PHYSOPT_RXOFF:

			_BIC (dstate, DSTATE_RXON);
			// Include reset
			goto ORst;

		case PHYSOPT_SETSID:

			statid = (val == NULL) ? 0 : *val;
			goto RRet;

		case PHYSOPT_GETSID:

			ret = (int) statid;
			goto RVal;

		case PHYSOPT_GETMAXPL:

			ret = rbuffl + 2;
			goto RVal;

	    	case PHYSOPT_ERROR:

			if (val != NULL) {
				memcpy (val, &rxstat,
					sizeof (rfc_propRxOutput_t));
			} else {
				memset (&rxstat, 0, 
					sizeof (rfc_propRxOutput_t));
			}

			goto RRet;

		case PHYSOPT_CAV:

			if (val == NULL)
				// Random LBT backoff
				gbackoff_lbt;
			else
				utimer_set (bckf_timer, *val);

			goto OREvnt;

		case PHYSOPT_GETPOWER:

#if RADIO_DEFAULT_POWER <= 7
			ret = txpower;
#else
			ret = RADIO_DEFAULT_POWER;
#endif
			goto RVal;

		case PHYSOPT_RESET:

#if RADIO_DEFAULT_POWER <= 7
			// Replace the patable
			patable = (val == NULL) ? patable_def : val;
Reset_pwr:
			RF_cmdPropRadioDivSetup.txPower = patable [txpower];
#endif
			goto ORst;

#if RADIO_DEFAULT_POWER <= 7 && (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS) == 0
		// Can be set
		case PHYSOPT_SETPOWER:

			// This goes via a global reset; apparently, we can do
			// per-packet power via a special command; the global
			// reset makes sense for a casual SETPOWER, because we
			// can execute it sensibly regardless of the RF state

			ret = txpower = (val == NULL) ? RADIO_DEFAULT_POWER :
				(*val > 7) ? 7 : *val;
			goto Reset_pwr;
#endif
		case PHYSOPT_GETCHANNEL:

			ret = (int) channel;
			goto RVal;

		case PHYSOPT_SETCHANNEL:

			channel = (val == NULL) ? RADIO_DEFAULT_CHANNEL :
				(*val > 7) ? 7 : *val;
			plugch ();
ORst:
			_BIS (dstate, DSTATE_IRST);
			goto OREvnt;

		case PHYSOPT_GETRATE:

			ret = (int) vrate;
			goto RVal;

#if RADIO_BITRATE_INDEX > 0

		case PHYSOPT_SETRATE:

			vrate = (val == NULL) ? RADIO_BITRATE_INDEX :
				(*val >= NRATES) ? NRATES - 1 :
					(*val < 1) ? 1 : *val;
			plugrt ();
			goto ORst;
#endif
		case PHYSOPT_SETPARAMS:

			set_driver_params ((cc1350_rfparams_t*) val);
			goto ORst;

#if TARP_COMPRESS
	    	// I make this conditional on a TARP parameter, although the
	    	// feature is potentially generally useful
	    	case PHYSOPT_REVOKE:

			ret = 0;
			if (paylen != 0 && (dstate & DSTATE_TXIP) == 0) {
				// Check the currently pending packet, if not
				// locked
				if (((pktqual_t)val)
					((address)RF_cmdPropTx.pPkt)) {
					// Delete current
					tcvphy_end ((address)
						RF_cmdPropTx.pPkt);
					paylen = 0;
					ret++;
				}
			}

			// Now try the queued packets
			ret += tcvphy_erase (physid, (pktqual_t)val);
			break;
#endif
		default:

			syserror (EREQPAR, "cc13 op");
	}
RRet:
	return ret;
RVal:
	if (val != NULL)
		*val = ret;
	goto RRet;
}

// ============================================================================

void phys_cc1350 (int phy, int mbs) {
//
// For combatibility with cc1100, mbs does cover the checksum (or status
// bytes), so it is equal to max payload length + 2. Not sure what the maximum
// packet length is (and I have reasons not to trust the manual), so let me
// assume it is 255 - 4 - 1 = 250. We shall be careful.
//
	TIME_COLDSTART;

#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (rbuffl != 0)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "cc13");
#endif
	if (mbs == 0)
		mbs = CC1350_MAXPLEN;

#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (mbs < 6 || mbs > CC1350_MAXPLEN)
		syserror (EREQPAR, "cc13 mb");
#endif

	rbuffl = (byte) mbs;

	// Allocate the receive pool
	init_rbuffs ();

	physid = phy;

	// Register the phy
	qevent = tcvphy_reg (phy, option, INFO_PHYS_CC1350);

	LEDI (0, 0);
	LEDI (1, 0);
	LEDI (2, 0);

#if DIAG_MESSAGES
	diag ("CC1350: %u, %u, %u", RADIO_BITRATE_INDEX, RADIO_DEFAULT_POWER,
		RADIO_DEFAULT_CHANNEL);
#endif
	// Install the backoff timer
	utimer_add (&bckf_timer);

#if RADIO_SYSTEM_IDENT
	// Zero means use the value produced by SmartStudio
	RF_cmdPropRx . syncWord = RF_cmdPropTx . syncWord = RADIO_SYSTEM_IDENT;
#endif
	// Start the driver process
	drvprcs = runthread (cc1350_driver);

#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (drvprcs == 0)
		syserror (ERESOURCE, "cc13");
#endif
	// Preset the power, rate, and channel

#if RADIO_DEFAULT_POWER <= 7
#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
	cmd_sp.txPower =
#endif
	RF_cmdPropRadioDivSetup.txPower =
		patable [txpower = RADIO_DEFAULT_POWER];
#endif

#if RADIO_BITRATE_INDEX > 0
	plugrt ();
#endif
	plugch ();

	// Make sure prop mode is selected in PRCM (this is supposed to be
	// the default)
	HWREG (PRCM_BASE + PRCM_O_RFCMODESEL) =  RF_MODE_PROPRIETARY_SUB_1;

	// Precompute the Trim
	RFCRTrim ((rfc_radioOp_t*)&RF_cmdPropRadioDivSetup);
       	RFCRfTrimRead ((rfc_radioOp_t*)&RF_cmdPropRadioDivSetup,
			(rfTrim_t*)&rfTrim);

	// Direct all doorbell interrupts permanently to CPE0
	HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFCPEISL) = 0;

	// Set default parameters
	set_driver_params (NULL);
}

