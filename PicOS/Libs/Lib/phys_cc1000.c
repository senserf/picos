/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "tcvphys.h"
#include "cc1000.h"

static int option (int, address);

/*
 * We are wasting precious RAM words, but we really want to make it efficient,
 * because interrupts will have to access this at CC1000's data clock rate.
 */
				// Pointer to static reception buffer
word		*zzr_buffer = NULL,
		*zzr_buffp,	// Pointer to next buffer word; also used to
				// indicate that a reception is pending
		*zzr_buffl,	// Pointer to LWA+1 of buffer area
		zzr_length,	// Length of received packet in words; this
				// is set after a complete reception
		*zzx_buffer,	// Pointer to dynamic transmission buffer
		*zzx_buffp,	// Next buffer word
		*zzx_buffl,	// LWA+1 of xmit buffer
		zzv_curbit,	// Current bit index
		zzv_status,	// Current interrupt mode (rcv/xmt/off)
		zzv_prmble,	// Preamble counter
		zzv_qevent,
		zzv_physid,
		zzv_statid,
		zzx_seed,	// For the random number generator
		zzx_backoff;	// Calculated backoff for xmitter

word 		zzv_istate = IRQ_OFF;

byte 		zzv_rxoff,
		zzv_txoff,
		zzv_hstat,
		zzx_power;

#if CC1000_FREQ == 868

#define TX_CURRENT	0xF3
#define RX_CURRENT	0x88
#define PLL_A		0x38
#define PLL_B		0x30
#define MODEM0_LEAST	0x05

static const byte chp_defcA [] = {
/*
 * Default contents of CC1000 registers
 */
	0x11,   // MAIN                 -> irrelevant

	0x66,   // FREQ_2A              -> set for RX 433 MHz @ REFDIV = 9
	0xE0,   // FREQ_1A
	0x00,   // FREQ_0A

	0x58,   // FREQ_2B              -> set for TX 433 MHz @ REFDIV = 14
	0x2C,   // FREQ_1B
	0x37,   // FREQ_0B

	0x01,   // FSEP1                -> set for RX
	0xAB,   // FSEP0

	0x88,   // CURRENT 0x88 ? 0x40 ??
	0x02,   // FRONT_END
	0x1,    // PA_POW               -> lowest power by default
	PLL_A,  // PLL                  -> updated
	0x10,   // LOCK
	0x26,   // CAL
	0xA1,   // 9C,  // MODEM2
	0x6F,   // MODEM1
	0x50 | MODEM0_LEAST,    // MODEM0       -> max baud + manchester
	0x70,   // MATCH
	0x01,   // FSCTRL
	0x00,   // PSHAPE7
	0x00,   // PSHAPE6
	0x00,   // PSHAPE5
	0x00,   // PSHAPE4
	0x00,   // PSHAPE3
	0x00,   // PSHAPE2
	0x00,   // PSHAPE1
	0x00,   // FSDELAY
	0x00    // PRESCALER
};

#endif

#if CC1000_FREQ == 433

#define TX_CURRENT	0x81
#define RX_CURRENT	0x40
#define PLL_A		0x70
#define PLL_B		0x48
#define MODEM0_LEAST	0x04

static const byte chp_defcA [] = {
/*
 * Default contents of CC1000 registers
 */
	0x11,	// MAIN			-> irrelevant

	0x66,	// FREQ_2A		-> set for RX 433 MHz @ REFDIV = 9
	0xA0,	// FREQ_1A
	0x00,	// FREQ_0A

	0x41,	// FREQ_2B		-> set for TX 433 MHz @ REFDIV = 14
	0xF2,	// FREQ_1B
	0x53,	// FREQ_0B

	0x02,	// FSEP1		-> set for RX
	0x80,	// FSEP0

	0x40,	// CURRENT
	0x02,	// FRONT_END
	0x1,	// PA_POW		-> lowest power by default
	PLL_A,	// PLL			-> updated
	0x10,	// LOCK
	0x26,	// CAL
	0xB7,	// 9C,	// MODEM2
  	0x6F,	// MODEM1
	0x50 | MODEM0_LEAST,	// MODEM0	-> max baud + manchester
	0x70,	// MATCH
	0x01,	// FSCTRL
	0x00,	// PSHAPE7
	0x00,	// PSHAPE6
	0x00,	// PSHAPE5
	0x00,	// PSHAPE4
	0x00,	// PSHAPE3
	0x00,	// PSHAPE2
	0x00,	// PSHAPE1
	0x00,	// FSDELAY
	0x00 	// PRESCALER
};

#endif

#ifndef	TX_CURRENT
#error	ILLEGAL CC1000 FREQUENCY, MUST BE 433 OR 868
#endif

static const byte chp_defcB [7] = {
/*
 * These ones occupy locations non-adjacent to the main chunk (see above)
 */
	0x10,	// TEST6
	0x08,	// TEST5
	0x3f,	// TEST4
	0x04,	// TEST3
	0x00,	// TEST2
	0x00,	// TEST1
	0x00 	// TEST0
};

typedef	struct {
/*
 * Description of a baud rate setting
 */
	word rate;
	byte baud;
	byte xosc;
} chp_rate_t;

static const chp_rate_t chp_rates [] =   { 
/*
 * Available baud rates: the first column is /100
 */
				{ (word)    6,  0, 3 },
				{ (word)   12,  1, 3 },
				{ (word)   24,  2, 3 },
				{ (word)   48,  3, 3 },
				{ (word)   96,  4, 3 },
				{ (word)  192,  5, 3 },
				{ (word)  384,  5, 1 },
				{ (word)  768,  5, 0 }
			};

/* ========================================= */

#include "checksum.h"

/* ========================================= */

static void chp_wconf (byte reg, byte data) {
/*
 * Write a configuration register
 */
	int i;

	// Make sure PDATA direction is out
	chp_pdirout;
	chp_paleup;

	// PALE goes down for the address part
	chp_paledown;
	// Make sure the write bit is set
	reg = (reg << 1) | 1;

	// The address bits + W
	for (i = 0; i < 8; i++) {
		chp_pclkup;
		chp_outpbit (reg & 0x80);
		reg <<= 1;
		chp_pclkdown;
	}
	chp_pclkup;
	// End of address
	chp_paleup;

	for (i = 0; i < 8; i++) {
		chp_pclkup;
		chp_outpbit (data & 0x80);
		data <<= 1;
		chp_pclkdown;
	}

	chp_pclkup;
}

static byte chp_rconf (byte reg) {
/*
 * Read a configuration register
 */
	byte i, res;

	// Send the address
	chp_pdirout;
	chp_paledown;

	// W bit is zero
	reg = (reg << 1);

	for (i = 0; i < 8; i++) {
		chp_pclkup;
		chp_outpbit (reg & 0x80);
		reg <<= 1;
		chp_pclkdown;
	}
	chp_pclkup;
	// Reset data direction to input
	chp_outpbit (0);
	chp_pdirin;
	chp_paleup;

	res = 0;
	for (i = 0; i < 8; i++) {
		chp_pclkdown;
		res = (res << 1) | chp_getpbit;
		chp_pclkup;
	}

	// Reset data direction to output (not really needed)
	chp_outpbit (0);
	chp_pdirout;

	return res;
}

static void xmt_enable_warm (void) {
/*
 * Switch from RX to TX
 */
	// Switch into TX, switch to freq. reg B
	chp_wconf (CC1000_MAIN, 0xF1);
	chp_wconf (CC1000_PLL, PLL_B);
	// Program VCO current for TX (why do we have to do it twice PG?)
	chp_wconf (CC1000_CURRENT, TX_CURRENT);
	// Set transmit power
	chp_wconf (CC1000_PA_POW, zzx_power);
	// Switch the transmitter on
	chp_wconf (CC1000_MAIN, 0xE1);
}

static void rcv_enable_warm (void) {
/*
 * Switch from TX to RX
 */
	// Switch into RX, switch to freq. reg A
	chp_wconf (CC1000_MAIN, 0x31);
	chp_wconf (CC1000_PLL, PLL_A);
	// Program VCO current for RX 
	chp_wconf (CC1000_CURRENT,RX_CURRENT);
	// Switch the receiver on
	chp_wconf (CC1000_MAIN,0x11);
}

static void xmt_enable_cold (void) {
/*
 * Enable transmitter from sleep
 */
	// Turn on xtal oscillator core
	chp_wconf (CC1000_MAIN, 0xFB);

	// Program VCO current for TX
	chp_wconf (CC1000_CURRENT, TX_CURRENT);

	// Wait for 5ms until the oscillator stabilizes
	mdelay (5);
	// Turn on bias generator
	chp_wconf (CC1000_MAIN, 0xF9);
	// Turn on frequency synthesiser
	chp_wconf (CC1000_MAIN, 0xF1);
	xmt_enable_warm ();
}

static void rcv_enable_cold (void) {
/*
 * Enable receiver from sleep
 */
	// Oscillator
	chp_wconf (CC1000_MAIN, 0x3B);

	// VCO current for RX
	chp_wconf (CC1000_CURRENT, RX_CURRENT);

	// Wait for 5ms until the oscillator stabilizes
	mdelay (5);
  
	// Bias generator
	chp_wconf (CC1000_MAIN, 0x39);
	// Frequency synthesizer
	chp_wconf (CC1000_MAIN, 0x31);  // Turn on frequency synthesiser

	rcv_enable_warm ();
}

static void xcv_reset (void) {

	byte reg;
  
	reg = chp_rconf (CC1000_MAIN);
	chp_wconf (CC1000_MAIN, reg & 0xFE);
	chp_wconf (CC1000_MAIN, reg | 0x01);
}

static INLINE void xcv_disable (void) {
/*
 * Power down
 */
	chp_wconf (CC1000_PA_POW, 0);
	chp_wconf (CC1000_MAIN, 0x3F);
}

static int w_calibrate (word rate) {
/*
 * Sets the bit rate and calibrates the chip
 */
	// I have redone this code according to the book (page 25 of the
	// data sheet)

	if (rate >= 96)
		chp_wconf (CC1000_TEST4, 0x3f);

	// Reset CAL
	chp_wconf (CC1000_CAL, 0x26);

	// Write MAIN: RX frequency register A is calibrated first
	// RXTX = 0; F_REG = 0; RX_PD = 0; TX_PD = 1; FS_PD = 0;
	// CORE_PD = 0; BIAS_PD = 0; RESET_N=1
	chp_wconf (CC1000_MAIN, 0x11);

	// Write CURRENT = RX current; Write PLL = RX pll
	chp_wconf (CC1000_CURRENT, RX_CURRENT);
	chp_wconf (CC1000_PLL, PLL_A);

	// Write CAL: CAL_START=1
	chp_wconf (CC1000_CAL, 0xA6);

	// Wait for maximum 34 ms, or Read CAL and wait until CAL_COMPLETE=1
	do {
		udelay (100);
	} while ((chp_rconf (CC1000_CAL) & 0x08) == 0);

	// Write CAL: CAL_START=0
	chp_wconf (CC1000_CAL, 0x26);

	// Write MAIN: RXTX = 1; F_REG = 1; RX_PD = 1; TX_PD = 0; FS_PD = 0;
	// CORE_PD = 0; BIAS_PD = 0; RESET_N=1
	chp_wconf (CC1000_MAIN, 0xE1);

	// Write CURRENT = TX current; Write PLL = TX pll; Write PA_POW = 00h
	chp_wconf (CC1000_CURRENT, TX_CURRENT);
	chp_wconf (CC1000_PLL, PLL_B);
	chp_wconf (CC1000_PA_POW, 0x00);

	// Write CAL: CAL_START=1
	chp_wconf (CC1000_CAL, 0xA6);
	
	// Wait for maximum 34 ms, or Read CAL and wait until CAL_COMPLETE=1
	do {
		udelay (100);
	} while ((chp_rconf (CC1000_CAL) & 0x08) == 0);

	// Write CAL: CAL_START=0
	chp_wconf (CC1000_CAL, 0x26);

	return ((chp_rconf (CC1000_LOCK) & 0x01) == 0);
}

static void ini_cc1000 (int baud) {
/*
 * Initialize the device
 */
	int i;

	ini_regs;

	// This must be up by default
	chp_paleup;
	// Start powered down
	xcv_disable ();
	// Hard reset
	xcv_reset ();

	// Load the defaults
	for (i = 1; i < sizeof (chp_defcA); i++)
		chp_wconf ((byte)i, chp_defcA [i]);
	// The test registers
	for (i = 0; i < sizeof (chp_defcB); i++)
		chp_wconf ((byte)i + 0x40, chp_defcB [i]);

	// Run the calibration
	if (baud <= 0)
		baud = RADIO_DEF_BITRATE;

	// Found the proper discrete rate
	for (i = 0; i < sizeof (chp_rates) - 1; i++)
		if (chp_rates [i] . rate >= baud)
			break;

	// Set the baud rate
	chp_wconf (CC1000_MODEM0, (chp_rates [i] . baud << 4) | MODEM0_LEAST |
		chp_rates [i] . xosc);

	i = w_calibrate (chp_rates [i] . rate);

	// Unlock the averaging filter
	chp_wconf (CC1000_MODEM1, 0x09);
	mdelay (10);
	xcv_disable ();

	diag ("CC1000 1000 %u MHz calibrated at %u00 bps, status = %u",
		(CC1000_FREQ == 868 ? 868 : 433), baud, i);
}

static void hstat (word status) {
/*
 * Change chip status: xmt, rcv, sleep
 */
	if (zzv_hstat == status)
		return;

	switch (status) {

		case HSTAT_SLEEP:
			xcv_disable ();
			break;
		case HSTAT_RCV:
			if (zzv_hstat == HSTAT_SLEEP)
				rcv_enable_cold ();
			else
				rcv_enable_warm ();
			break;
		default:
			if (zzv_hstat == HSTAT_SLEEP)
				xmt_enable_cold ();
			else
				xmt_enable_warm ();
			break;
	}

	zzv_hstat = status;
}

/* ========================================= */

static byte rssi_cnv (word v) {
/*
 * Converts the RSSI to a single byte 0-255
 */
	add_entropy (v);

#if RSSI_MIN >= 0x8000
	// RSSI is signed
	if ((v & RSSI_MIN))
		v |= RSSI_MIN;
#endif

#if RSSI_MIN != 0
	v -= RSSI_MIN;
#endif
	// Higher RSSI means lower signal level
	return (byte) ((((int)RSSI_MAX - (int)RSSI_MIN) - (int) v) >> RSSI_SHF);
}

#include "xcvcommon.h"

void phys_cc1000 (int phy, int mbs, int bau) {
/*
 * phy  - interface number
 * mbs  - maximum packet length (excluding checksum, must be divisible by 4)
 * bau  - baud rate /100 (default assumed if zero)
 */
	if (zzr_buffer != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_cc1000");

	if (mbs < 8 || mbs & 0x3) {
		if (mbs == 0)
			mbs = RADIO_DEF_BUF_LEN;
		else
			syserror (EREQPAR, "phys_cc1000 mbs");
	}

	/* For reading RSSI */
	adc_config;

	if ((zzr_buffer = umalloc (mbs)) == NULL)
		syserror (EMALLOC, "phys_cc1000 (b)");

	/* This is static and will never change */
	zzr_buffl = zzr_buffer + (mbs >> 1);
	/* This also indicates that there's no pending reception */
	zzr_buffp = 0;

	zzv_status = 0;

	zzx_power = RADIO_DEF_XPOWER;

	zzv_statid = 0;
	zzv_physid = phy;
	zzx_backoff = 0;

	zzx_seed = 12345;

	/* Register the phy */
	zzv_qevent = tcvphy_reg (phy, option, INFO_PHYS_CC1000);

	/* Both parts are initially inactive */
	zzv_rxoff = zzv_txoff = 1;
	LEDI (0, 0);
	LEDI (1, 0);

	/* Start the device */
	ini_cc1000 (bau);

	zzv_hstat = HSTAT_SLEEP;
}

static int option (int opt, address val) {
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = ((zzv_txoff == 0) << 1) | (zzv_rxoff == 0);
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		zzv_txoff = 0;
		LEDI (1, 1);
		if (!running (xmtradio))
			fork (xmtradio, NULL);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_RXON:

		zzv_rxoff = 0;
		LEDI (0, 1);
		if (!running (rcvradio))
			fork (rcvradio, NULL);
		trigger (rxevent);
		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		zzv_txoff = 2;
		LEDI (1, 0);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_TXHOLD:

		zzv_txoff = 1;
		LEDI (1, 0);
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_RXOFF:

		zzv_rxoff = 1;
		LEDI (0, 0);
		trigger (rxevent);
		break;

	    case PHYSOPT_CAV:

		/* Force an explicit backoff */
		if (val == NULL)
			zzx_backoff = 0;
		else
			zzx_backoff = *val;
		trigger (zzv_qevent);
		break;

	    case PHYSOPT_SENSE:

		ret = (zzv_status != 0);
		break;

	    case PHYSOPT_SETPOWER:

		if (val == NULL || *val == 0)
			zzx_power = RADIO_DEF_XPOWER;
		else
			zzx_power = (*val) > 0xff ? 0xff : (byte) (*val);
		break;

	    case PHYSOPT_GETPOWER:

		ret = ((int) rssi_cnv (adc_value)) & 0xff;

		if (val != NULL)
			*val = ret;

		break;

	    case PHYSOPT_SETSID:

		zzv_statid = (val == NULL) ? 0 : *val;
		break;

            case PHYSOPT_GETSID:

		ret = (int) zzv_statid;
		if (val != NULL)
			*val = ret;
		break;

	    default:

		syserror (EREQPAR, "phys_cc1000 option");
	}
	return ret;
}