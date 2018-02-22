/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

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
#define	DSTATE_RXON	0x01	// Rx logically on
#define	DSTATE_RXAC	0x02	// Rx active
#define	DSTATE_TXIP	0x04	// Tx in progress
#define	DSTATE_RFON	0x10	// Device on
#define	DSTATE_IRST	0x80	// Internal error / reset request

// The number of simultaneous receive buffers
#define	NRBUFFS		2

// The secret command (stolen from tirtos)
#define	RF_CMD0		0x0607

#if RADIO_BITRATE_INDEX > 0

// We can change the rate; rates 1-3 are settable, 0 must be hardwired

static byte vrate = RADIO_BITRATE_INDEX;

#endif

static byte channel = RADIO_DEFAULT_CHANNEL;

#if RADIO_DEFAULT_POWER <= 7

// Power can be changed; the max power must be hardwired (with rate 0) for
// long range operation

static const word patable [] = CC1350_PATABLE;

#endif

static word	physid,
		statid = 0,
		bckf_timer = 0,
		offdelay = RADIO_DEFAULT_OFFDELAY;

#if RADIO_BITRATE_INDEX > 0

typedef struct {
		word	ps,	// prescale
			rw;	// rate word
} ratable_t;

static const ratable_t ratable [] = CC1350_RATABLE;

#endif

// Pointer to the queue of reception buffers
#define	rbuffs 		(RF_cmdPropRx . pQueue)

// Reception statistics (can be extracted via PHYSOPT_ERROR)
static rfc_propRxOutput_t	rxstat;

// Events
static aword drvprcs, qevent;

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
static rfc_CMD_PROP_CS_t cmd_cs = {
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
		.csConf.timeoutRes = 0,	// Timeout on invalid = busy (aot idle)
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
		.csEndTime = (RADIO_LBT_SENSE_TIME * 4 * 1024),
	};
#endif

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)

#if RADIO_DEFAULT_POWER > 7
#error "S: PXOPTIONS requires RADIO_DEFAULT_POWER < 8"
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

static void halt_cmd () {
//
// Make sure the device is not running any command; will this do?
//
	issue_cmd (CMDR_DIR_CMD (CMD_ABORT));
}

// ============================================================================

static void rf_on () {
//
// Turn the radio on
//
	if (dstate & DSTATE_RFON)
		return;

#ifdef	RADIO_PINS_ON
	// Any special pin settings?
	RADIO_PINS_ON;
#endif
	// The oscillator (do we need this?); no, this is the default, anyway
	// OSCXHfPowerModeSet (HIGH_POWER_XOSC);

	// The default setting of OSC clock source is OSC_RCOSC_HF; we shall
	// revert to it when the radio is switched off
	OSCHF_TurnOnXosc();
	do { udelay (10); } while (!OSCHF_AttemptToSwitchToXosc ());

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

#if 0
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
	_BIC (dstate, DSTATE_IRST);

	// Interrupts, we only care about reception and system error (?)
	HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIEN) = 0;
	HWREG(RFC_DBELL_BASE+RFC_DBELL_O_RFCPEIFG) = 0;
	IntEnable (INT_RFC_CPE_0);

	LEDI (0, 1);

#if (RADIO_OPTIONS & RADIO_OPTION_RBKF)
	bckf_lbt = 0;
#endif

}

static void rf_off () {
//
// Turn the radio off
//
	if ((dstate & DSTATE_RFON) == 0)
		return;

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
}

static void rx_ac () {
//
// Activate the receiver
//
	rfc_dataEntryGeneral_t *re;
	int i;

	if (dstate & DSTATE_RXAC)
		return;

	// Make sure the buffers are clean
	for (re = (rfc_dataEntryGeneral_t*) (rbuffs->pCurrEntry), i = 0;
	    i < NRBUFFS; i++, re = (rfc_dataEntryGeneral_t*)(re->pNextEntry))
		re->status = DATA_ENTRY_PENDING;

	// Issue the command
	issue_cmd ((lword)&RF_cmdPropRx);

	_BIS (dstate, DSTATE_RXAC);
}

static void rx_de () {
//
// Deactivate the receiver
//
	if ((dstate & DSTATE_RXAC) == 0)
		return;

	issue_cmd (CMDR_DIR_CMD (CMD_ABORT));
	_BIC (dstate, DSTATE_RXAC);
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
	RF_cmdPropRadioDivSetup.symbolRate.preScale = ratable [vrate - 1] . ps;
	RF_cmdPropRadioDivSetup.symbolRate.rateWord = ratable [vrate - 1] . rw;
}

#endif

static void rx_int_enable () {

	rfc_dataEntryGeneral_t *db;
	int i, pl, nr;

	// This clears all interrupt flags and enables those that we want; this
	// is different than (e.g.) for CC1100/MSP430, because events can be
	// lost (the clear operation may remove an event that we haven't looked
	// at yet; so we do the reception with interrupts enabled; if an
	// interrupt arrives while we are looking, it will just unblock the
	// driver thread which will force (later) another invocation of this
	// function
	RFCCpe0IntEnable (RFC_DBELL_RFCPEIEN_RX_OK |
		RFC_DBELL_RFCPEIEN_INTERNAL_ERROR);

#define	__dp	(&(db->data))

	// Receive
	for (db = (rfc_dataEntryGeneral_t*) (rbuffs->pCurrEntry), i = nr = 0;
	    i < NRBUFFS; i++, db = (rfc_dataEntryGeneral_t*)(db->pNextEntry)) {
		if (db->status == DATA_ENTRY_FINISHED) {
			LEDI (2, 1);
			nr++;
			// Consistency checks
			if (__dp [0] == __dp [1] + 3 && __dp [1] <= rbuffl &&
			   (__dp [1] & 1) == 0) {
				// Payload length (including two extra bytes)
				pl = __dp [1] + 2;
				// Swap RSS and status, actually move RSS one
				// byte up and zero the status (for now); RSS
				// is unbiased to nonnegative (as usual)
				__dp [pl + 1] = __dp [pl] - 128;
				__dp [pl] = 0;
				tcvphy_rcv (physid, (address)(__dp + 2), pl);
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

	if (nr) {
		LEDI (2, 0);
#if RADIO_LBT_BACKOFF_RX
		gbackoff (RADIO_LBT_BACKOFF_RX);
#endif
	}
}

void RFCCPE0IntHandler (void) {
//
// This is extremely simple and effective: any interrupt (of the enabled
// kind, of course) unhangs the driver thread which sees what's happened
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

	p_trigger (drvprcs, qevent);

	RISE_N_SHINE;
}

// ============================================================================

#define	DR_LOOP		0
#define	DR_XMIT		1
#define	DR_GOOF		2

#ifdef	DETECT_HANGUPS
static	int txcounter;
#endif

thread (cc1350_driver)

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
	address ppm;
	word pcav;
#endif

	entry (DR_LOOP)

DR_LOOP__:
		if (dstate & DSTATE_IRST) {
			// Reset request
			rf_off ();
			// Need some delay?
			_BIC (dstate,
			 DSTATE_RXAC | DSTATE_RFON | DSTATE_TXIP | DSTATE_IRST);
		}

		// Keep RX state in sync with the requested state
		if (dstate & DSTATE_RXAC) {
			if ((dstate & DSTATE_RXON) == 0)
				rx_de ();
		} else if (dstate & DSTATE_RXON) {
			// Request to activate the receiver
			rf_on ();
			rx_ac ();
		}

		// Try to get a packet to transmit
		if (paylen == 0) {
			if ((RF_cmdPropTx.pPkt = (byte*)tcvphy_get (physid,
			    &paylen)) != NULL) {

				// Ignore the two extra bytes right away, we
				// have no use for them
				paylen -= 2;
				sysassert (paylen <= rbuffl && paylen > 0 &&
				    (paylen & 1) == 0, "cc13 py");

				LEDI (1, 1);

				if (statid != 0xffff)
					// Insert NetId
					((address)(RF_cmdPropTx.pPkt)) [0] =
						statid;

				RF_cmdPropTx.pktLen = (byte) paylen;
			}
		}

		if (paylen == 0) {
			// Nothing to transmit
			wait (qevent, DR_LOOP);
			if (dstate & DSTATE_RXAC) {
				// Try to receive
				rx_int_enable ();
				// For lost interrups (test)
				// delay (1024, DR_LOOP);
			} else if (dstate & DSTATE_RFON) {
				// Delay until off
				delay (offdelay, DR_GOOF);
			}
			release;
		}

		if (bckf_timer) {
			// Backoff wait
#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
Bkf:
#endif
			wait (qevent, DR_LOOP);
			delay (bckf_timer, DR_LOOP);
			if (dstate & DSTATE_RXAC)
				rx_int_enable ();
			release;
		}

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
		// Check for CAV requested in the packet
		ppm = (address)(RF_cmdPropTx.pPkt + paylen);
		if ((pcav = (*ppm) & 0x0fff)) {
			// Remove for next turn
			*ppm &= ~0x0fff;
			utimer_set (bckf_timer, pcav);
			goto Bkf;
		}
#endif
		// Make sure we are up and in the right state
		rf_on ();
		rx_de ();

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
		if ((pcav = patable [(*ppm >> 12) & 0x7]) !=
		    RF_cmdPropRadioDivSetup.txPower) {
			// Need to change
		    	RF_cmdPropRadioDivSetup.txPower = 
				cmd_sp.txPower = pcav;
			issue_cmd ((lword)&cmd_sp);
		}
#endif

#if (RADIO_OPTIONS & RADIO_OPTION_RBKF)
		((address)(RF_cmdPropTx.pPkt)) [1] = bckf_lbt;
		// Do not zero the counter as it may still grow
#endif

#if RADIO_LBT_SENSE_TIME > 0

		// ============================================================
		// LBT ON =====================================================
		// ============================================================

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
		if (*ppm & 0x8000) {
			// LBT off requested by packet
			txtries = RADIO_LBT_MAX_TRIES;
			goto Force;
		}
#endif
		RF_cmdPropTx . status = 0;
		HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) = 
			~RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE;
		issue_cmd ((lword)&cmd_cs);

	entry (DR_XMIT)

		if (dstate & DSTATE_IRST)
			goto DR_LOOP__;

		if ((HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) &
		    RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE) == 0) {
			// Keep waiting
			delay (1, DR_XMIT);
			release;
		}

		// Check the TX status
		if (RF_cmdPropTx.status != PROP_DONE_OK) {
			if (txtries >= RADIO_LBT_MAX_TRIES) {
#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
Force:
#endif
				// diag ("FORCED");
				RF_cmdPropTx . status = 0;
				HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) = 
					~RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE;
				issue_cmd ((lword)&RF_cmdPropTx);
				proceed (DR_XMIT);
			}
#if RADIO_LBT_MAX_TRIES < 255
			txtries++;
#endif
			// Backoff
			// diag ("BUSY");
#if RADIO_LBT_BACKOFF_EXP == 0
			delay (1, DR_LOOP);
			update_bckf_lbt (1);
			release;
#else
			gbackoff (RADIO_LBT_BACKOFF_EXP);
			update_bckf_lbt (bckf_timer);
			goto DR_LOOP__;
#endif
		}
#else	/* RADIO_LBT_SENSE_TIME */

		// ============================================================
		// LBT OFF ====================================================
		// ============================================================

		RF_cmdPropTx . status = 0;
		HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) = 
			~RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE;
		issue_cmd ((lword)&RF_cmdPropTx);

#ifdef	DETECT_HANGUPS
		txcounter = DETECT_HANGUPS;
#endif
	entry (DR_XMIT)

		if (dstate & DSTATE_IRST)
			goto DR_LOOP__;

		if ((HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) &
		    RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE) == 0) {
#ifdef	DETECT_HANGUPS
			if (txcounter-- == 0) {
				diag ("TX HANG: %lx", RF_cmdPropTx.status);
				syserror (EHARDWARE, "txhung");
			}
#endif
			delay (1, DR_XMIT);
			release;
		}

		if (RF_cmdPropTx.status != PROP_DONE_OK) {
			// Something went wrong; actually, this can happen
			// when transmitting immediately after rf_on
#ifdef	DETECT_HANGUPS
			// diag ("TX ERR: %lx", RF_cmdPropTx.status);
			// syserror (EHARDWARE, "txerr");
#endif
			delay (1, DR_LOOP);
			release;
		}

#endif	/* RADIO_LBT_SENSE_TIME */

		// ============================================================
		// ============================================================
		// ============================================================

		// Release the packet
		tcvphy_end ((address)(RF_cmdPropTx.pPkt));
		paylen = 0;

#if (RADIO_OPTIONS & RADIO_OPTION_RBKF)
		bckf_lbt = 0;
#endif

#if RADIO_LBT_SENSE_TIME > 0
		txtries = 0;
#endif
		LEDI (1, 0);

#if RADIO_LBT_XMIT_SPACE
		utimer_set (bckf_timer, RADIO_LBT_XMIT_SPACE);
		// If we don't care to space (don't we?), then perhaps we
		// should send whatever pending packets we have in the queue
		// without restarting the receiver
#endif
		// This will restart RX, if RX is logically on
		goto DR_LOOP__;

	entry (DR_GOOF)

		// Inactivity timeout with RX OFF
		if (!(dstate & DSTATE_IRST) && (dstate & DSTATE_RXON) == 0 &&
		    tcvphy_top (physid) == NULL)
			rf_off ();

		goto DR_LOOP__;
endthread

#undef	DR_LOOP
#undef	DR_XMIT
#undef	DR_GOOF

// ============================================================================

static void init_rbuffs () {
//
// Set up the receive buffer pool
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
			goto OREvnt;

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
				// Random backoff
				gbackoff (RADIO_LBT_BACKOFF_EXP);
			else
				utimer_set (bckf_timer, *val);

			goto OREvnt;

		case PHYSOPT_GETPOWER:

#if RADIO_DEFAULT_POWER < 8
			for (ret = 0; ret < 8; ret++)
				if (RF_cmdPropRadioDivSetup.txPower
					== patable [ret])
						break;
#else
			ret = 8;
#endif
			goto RVal;

#if RADIO_DEFAULT_POWER < 8 && (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS) == 0
		// Can be set
		case PHYSOPT_SETPOWER:

			// This is a global reset; apparently, we can do
			// per-packet power via a special command

			ret = (val == NULL) ? RADIO_DEFAULT_POWER :
				(*val > 7) ? 7 : *val;
			RF_cmdPropRadioDivSetup.txPower = patable [ret];
			_BIS (dstate, DSTATE_IRST);
			goto OREvnt;
#endif
		case PHYSOPT_GETCHANNEL:

			ret = (int) channel;
			goto RVal;

		case PHYSOPT_SETCHANNEL:

			channel = (val == NULL) ? RADIO_DEFAULT_CHANNEL :
				(*val > 7) ? 7 : *val;
			plugch ();
			_BIS (dstate, DSTATE_IRST);
			goto OREvnt;

		case PHYSOPT_GETRATE:

#if RADIO_BITRATE_INDEX > 0
			ret = (int) vrate;
#endif
			goto RVal;

#if RADIO_BITRATE_INDEX > 0

		case PHYSOPT_SETRATE:

			vrate = (val == NULL) ? RADIO_BITRATE_INDEX :
				(*val > 3) ? 3 : (*val < 1) ? 1 : *val;
			plugch ();
			_BIS (dstate, DSTATE_IRST);
			goto OREvnt;
#endif
		case PHYSOPT_SETPARAMS:

			// For now, this sets the off delay
			offdelay = (val == NULL) ? RADIO_DEFAULT_OFFDELAY :
				*val;
			goto RRet;

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

#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (rbuffl != 0)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "cc13");
#endif
	if (mbs == 0)
		mbs = CC1350_MAXPLEN;

#if (RADIO_OPTIONS & RADIO_OPTION_NOCHECKS) == 0
	if (mbs < 6 || mbs > CC1350_MAXPLEN)
		/* We are allowed to do it only once */
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
	diag ("CC1350: %d, %d, %d", RADIO_BITRATE_INDEX, RADIO_DEFAULT_POWER,
		RADIO_DEFAULT_CHANNEL);
#endif

	// Install the backoff timer
	utimer_add (&bckf_timer);

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
	RF_cmdPropRadioDivSetup.txPower = patable [RADIO_DEFAULT_POWER];
#endif

#if RADIO_BITRATE_INDEX > 0
	plugrt ();
#endif
	plugch ();

	// Make sure, prop mode is selected in PRCM (this is supposed to be
	// the default)
	HWREG (PRCM_BASE + PRCM_O_RFCMODESEL) =  RF_MODE_PROPRIETARY_SUB_1;

	// Precompute the Trim
	RFCRTrim ((rfc_radioOp_t*)&RF_cmdPropRadioDivSetup);
       	RFCRfTrimRead ((rfc_radioOp_t*)&RF_cmdPropRadioDivSetup,
			(rfTrim_t*)&rfTrim);

	// Direct all doorbell interrupts permanently to CPE0
	HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFCPEISL) = 0;
}

//
// Notes:
//
//	CMD_PING can be a way to trigger an interrupt, especially that we put
//	them all into a single basket
//
//	Implement timeouts for commands that wait for interrupts; when the
//	receiver is on, recover every second or so (reimplement xmit using
//	interrupts and keep a special thread issuing periodic triggers or
//	something like that
