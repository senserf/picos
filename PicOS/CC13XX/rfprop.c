/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Code for RF in the proprietary mode

#define	DEVICE_FAMILY	cc13x0

#define	mkmk_eval
#include "smartrf_settings.h"
#undef	mkmk_eval

#include <driverlib/rf_common_cmd.h>
#include <driverlib/rf_prop_cmd.h>
#include <driverlib/rf_prop_mailbox.h>
#include <driverlib/rf_data_entry.h>
#include <driverlib/rfc.h>

#include <rf_patches/rf_patch_cpe_genfsk.h>
#include <rf_patches/rf_patch_rfe_genfsk.h>

//+++ "smartrf_settings.c"

#include "sysio.h"

// ============================================================================

#define	RFCMDWAIT		100
#define	RF_MAX_COMMAND_SIZE	256

// ============================================================================

procname (rf_test_transmitter);
procname (rf_test_receiver);

static lword cmdstat;	// Probably not needed

// Init command data
static rfc_CMD_GET_FW_INFO_t	cmd_gfi = { .commandNo = CMD_GET_FW_INFO };
static rfc_CMD_PING_t		cmd_pin = { .commandNo = CMD_PING };
static rfc_CMD_SYNC_START_RAT_t	cmd_srt = { .commandNo = CMD_SYNC_START_RAT };

int __rfprop_cmd (lword cmd) {
//
// Issue a command to the radio, wait for status, detect low-level problems;
// we shall see whether we should do it via interrupts (RFCMDACK) or FSM
// delay
//
	int i, res;

	HWREG (RFC_DBELL_BASE + RFC_DBELL_O_CMDR) = cmd;
	for (i = 0; i < RFCMDWAIT; i++) {
		udelay (1);
		cmdstat = HWREG (RFC_DBELL_BASE + RFC_DBELL_O_CMDSTA);
		if ((res = cmdstat & 0xff))
			break;
	}

	if (i == RFCMDWAIT) {
		// Timeout
		diag ("RCMD TO: %lx", cmd);
		syserror (EHARDWARE, "rt0");
	}

	if (res == 0x01)
		// This means OK
		return 0;

	// Generic error, never OK
	diag ("RCMD ER: %lx %x", cmd, res);
	return 1;
}

void __rfprop_wcm (rfc_radioOp_t *cmd, lword tstat, lword timeout) {
//
// Wait for a radio operation command to complete
//

#if 1	/* Debug (show status changes) */
		word os = 0xffff;
		diag ("WS: %x", tstat);
#endif
	while (1) {
#if 1
		if (cmd->status != os) {
  			os = cmd->status;
  			diag ("ST: %x", os);
		}
#endif
		if (cmd->status == tstat)
			// Target status
			return;
		if (timeout-- == 0) {
			diag ("RF OT: %x", cmd->commandNo);
			syserror (EHARDWARE, "rt1");
		}
		udelay (1);
	}
}

// ============================================================================

void __rfprop_initbuffs ();
		
void __rfprop_initialize () {
//
// This is probably very temporary
//

#ifdef	RADIO_PINS_ON
	// In case special signals are required
	RADIO_PINS_ON;
#endif
	// Void, reset value
	HWREG (PRCM_BASE + PRCM_O_RFCMODESEL) =  RF_MODE_PROPRIETARY_SUB_1;

	// The oscillator (do we need this?)
	// OSCXHfPowerModeSet (HIGH_POWER_XOSC);

	if (OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_XOSC_HF) {
		diag ("OSC!");
		OSCHF_TurnOnXosc();
		do { mdelay (1); } while (!OSCHF_AttemptToSwitchToXosc ());
	}

	mdelay (10);
#if 1
// In main
	// RAT synced with RTC
	HWREGBITW (AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) =
		1;
#endif

	// Power up the domain
	PRCMPowerDomainOn (PRCM_DOMAIN_RFCORE);
	while (PRCMPowerDomainStatus (PRCM_DOMAIN_RFCORE) !=
		PRCM_DOMAIN_POWER_ON);

#if 0
	// This is not a peripheral !!!
	// Enable
	PRCMPeripheralRunEnable (PRCM_PERIPH_RFCORE);
	PRCMPeripheralSleepEnable (PRCM_PERIPH_RFCORE);
	PRCMPeripheralDeepSleepEnable (PRCM_PERIPH_RFCORE);
	PRCMLoadSet ();
	while (!PRCMLoadGet ());
#endif

	RFCClockEnable ();
	// Check how much is needed
	mdelay (100);

#if 0

HWREG(AON_WUC_BASE + AON_WUC_O_AUXCTL) |= AON_WUC_AUXCTL_AUX_FORCE_ON;
while( !(HWREG(AON_WUC_BASE + AON_WUC_O_PWRSTAT)&AON_WUC_PWRSTAT_AUX_PD_ON) ){;}

HWREG(AUX_WUC_BASE + AUX_WUC_O_MODCLKEN0) |= AUX_WUC_MODCLKEN0_AUX_DDI0_OSC|AUX_WUC_MODCLKEN0_SMPH;

//HWREG(AUX_DDI0_OSC_BASE + DDI_0_OSC_O_CTL0) |= DDI_0_OSC_CTL0_SCLK_HF_SRC_SEL;
DDI32BitsSet(AUX_DDI0_OSC_BASE,DDI_0_OSC_O_CTL0,DDI_0_OSC_CTL0_SCLK_HF_SRC_SEL);

#endif

	diag ("CMD0");

#if 1
#define RF_CMD0 0x0607
	HWREG (RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0;
	__rfprop_cmd (
		CMDR_DIR_CMD_2BYTE (RF_CMD0,
			RFC_PWR_PWMCLKEN_MDMRAM | RFC_PWR_PWMCLKEN_RFERAM));
#endif

	diag ("PATCHING");







	// Patches
	rf_patch_cpe_genfsk ();

#if 1
	rf_patch_rfe_genfsk ();		// Check if needed
#endif
	diag ("PATHCHED");

#if 1
        // Turn off additional clocks
        __rfprop_cmd (CMDR_DIR_CMD_2BYTE (RF_CMD0, 0));
#endif




	diag ("BUS");


#if 1
        //Initialize bus request
        HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0;
	__rfprop_cmd (CMDR_DIR_CMD_1BYTE(CMD_BUS_REQUEST, 1));
#endif


	diag ("DONE");





	RFCAdi3VcoLdoVoltageMode (true);


#if 1
	// To be done only once !!!
	RFCRTrim ((rfc_radioOp_t*)&RF_cmdPropRadioDivSetup);

        rfTrim_t rfTrim;
        RFCRfTrimRead ((rfc_radioOp_t*)&RF_cmdPropRadioDivSetup,
		(rfTrim_t*)&rfTrim);
        RFCRfTrimSet ((rfTrim_t*)&rfTrim);

#endif

	diag ("TRIMMED");





#if 1
	// Check connection
	__rfprop_cmd ((lword)&cmd_gfi);

	// Show the result
	diag ("GFI: %x %x %x %x", cmd_gfi.versionNo, cmd_gfi.startOffset,
		cmd_gfi.freeRamSz, cmd_gfi.availRatCh);
diag ("PING");
	// Ping
	__rfprop_cmd ((lword)&cmd_pin);
#endif

	mdelay (10);

diag ("SPRO");
	// Setup proprietary mode
	__rfprop_cmd ((lword)&RF_cmdPropRadioDivSetup);
	// Wait for done
	__rfprop_wcm ((rfc_radioOp_t*)&RF_cmdPropRadioDivSetup, PROP_DONE_OK,
		100000);

diag ("SRAT");
	// Start RAT
	__rfprop_cmd ((lword)&cmd_srt);

diag ("SFRE");

	// Power up frequency synthesizer, if 
	// CMD_PROP_RADIO_DIV_SETUP.bNoFsPowrup == 1 (it is zero)

	// Set frequency
	__rfprop_cmd ((lword)&RF_cmdFs);
	__rfprop_wcm ((rfc_radioOp_t*)&RF_cmdFs, DONE_OK, 100000);

diag ("DONE");





	//
	// runthread (rf_test_transmitter);
	runthread (rf_test_receiver);
}

// Default PROP_TX:
//
//	pktConf.bfsOff = 0	(keep Freq Syn on after command)
//	pktConf.bUseCrc = 1
//	pktConf.bVarLen = 1	(length as first byte)
//
// Still don't know how to interpret length

static byte tseqnum = 0;

#define	PAYLEN	32
#define	MARGIN	8
#define	TOTLEN	(PAYLEN + MARGIN)


static	byte tbuff [TOTLEN];	// Including a margin

#define	RTT_LOOP	0

int	next_paylen = 28;

thread (rf_test_transmitter)

	int i;

	entry (RTT_LOOP)

		tbuff [0] = 0xf9;
		tbuff [1] = tseqnum++;

		for (i = 2; i < next_paylen; i++)
			tbuff [i] = (byte) i;

		while (i < TOTLEN)
			tbuff [i++] = 0xfa;

		RF_cmdPropTx.pPkt = tbuff;
		RF_cmdPropTx.pktLen = next_paylen;

		__rfprop_cmd ((lword)&RF_cmdPropTx);
		__rfprop_wcm ((rfc_radioOp_t*)&RF_cmdPropTx, PROP_DONE_OK,
			100000);

		diag ("TX: %d, %x %x %x %x ... %x %x",
			next_paylen,
			tbuff [0], tbuff [1], tbuff [2], tbuff [3],
				tbuff [next_paylen - 1], tbuff [next_paylen]);

		next_paylen = (next_paylen < 32) ? next_paylen + 1 : 28;

		delay (2048, RTT_LOOP);

endthread

// ============================================================================

#define	NRXBUFFS	2

static	byte rbuff [NRXBUFFS * TOTLEN];

static	rfc_dataEntryPointer_t dbuffs [NRXBUFFS];

static	dataQueue_t dqueue;

static	rfc_propRxOutput_t rxstat;

void __clean_rbuff (byte *rb) {

	for (int i = 0; i < TOTLEN; i++)
		rb [i] = 0xAB;
}

void __rfprop_initbuffs () {

	byte *rb;
	rfc_dataEntryPointer_t *da, *db;
	int i;

	for (db = &(dbuffs [0]), rb = rbuff, i = 0; i < NRXBUFFS; i++) {

		db->status = 0;		// Status == pending

		if (i)
			da->pNextEntry = (byte*) db;

		if (i == NRXBUFFS - 1)
			db->pNextEntry = (byte*) &(dbuffs [0]);

		db->config.type = 2;	// Pointer entry
		db->config.lenSz = 1;	// Single byte for length
		db->config.irqIntv = 0;	// Irrelevant
		db->length = TOTLEN-2;
		db->pData = rb;
		__clean_rbuff (rb);

		rb += TOTLEN;
		da = db++;
	}

	dqueue.pCurrEntry = (byte*) &(dbuffs [0]);
	dqueue.pLastEntry = NULL;
}

#define	RCV_INIT	0
#define	RCV_START	1
#define	RCV_LOOP	2

thread (rf_test_receiver)

	int i;
	byte flength, plength;
	rfc_dataEntryPointer_t *db;

	entry (RCV_INIT)

		__rfprop_initbuffs ();

	entry (RCV_START)

		RF_cmdPropRx . pQueue = &dqueue;	
		RF_cmdPropRx . pOutput = (byte*) &rxstat;	

		// Is this how we keep it in a loop?
		RF_cmdPropRx . pktConf . bRepeatOk = 1;
		RF_cmdPropRx . pktConf . bRepeatNok = 1;

		// Discard rejects from the queue
		RF_cmdPropRx . rxConf . bAutoFlushIgnored = 1;
		RF_cmdPropRx . rxConf . bAutoFlushCrcErr = 1;
		RF_cmdPropRx . rxConf . bAppendRssi = 1;

		// By default discards CRC appends status byte

		__rfprop_cmd ((lword)&RF_cmdPropRx);

	entry (RCV_LOOP)

		for (i = 0; i < NRXBUFFS; i++) {
			db = dbuffs + i;
			if (db->status == DATA_ENTRY_FINISHED) {

				flength = db->pData [0];
				plength = db->pData [1];

				if (flength > PAYLEN + 4) {
					diag ("FL BIG: %u", flength);
					flength = PAYLEN + 4;
				}

				if (plength > PAYLEN) {
					diag ("PL BIG: %u", plength);
					plength = PAYLEN;
				}

	// With AppendRssi, two bytes past payload are used: RSSI + 0,
	// without, just one zero, the buffer length must be
	// paylen + 2 + rss + 1; the packet looks this way:
	//	total paylen pay ... pay rssi 0
	// total is paylen + 3 (with RSSI), even though paylen + 4 bytes
	// are actually written
	diag ("RX: %d %d, %x %x %x %x ... %x | %x %x %x %x %x %x",
					flength, plength,
					db->pData [2],
					db->pData [3],
					db->pData [4],
					db->pData [5],
					db->pData [2 + plength - 1],
					db->pData [2 + plength - 0],
					db->pData [2 + plength + 1],
					db->pData [2 + plength + 2],
					db->pData [2 + plength + 3],
					db->pData [2 + plength + 4],
					db->pData [2 + plength + 5] );

	diag ("RS: %u %u %u %u %u <%d> %lu",
		rxstat.nRxOk,
		rxstat.nRxNok,
		rxstat.nRxIgnored,
		rxstat.nRxStopped,
		rxstat.nRxBufFull,
		rxstat.lastRssi,
		rxstat.timeStamp);

				__clean_rbuff (db->pData);
				db->status = DATA_ENTRY_PENDING;
			}
		}

		delay (2, RCV_LOOP);
endthread
