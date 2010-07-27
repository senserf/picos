/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
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
word		*__pi_r_buffer = NULL,
		*__pi_r_buffp,	// Pointer to next buffer word; also used to
				// indicate that a reception is pending
		*__pi_r_buffl,	// Pointer to LWA+1 of buffer area
		__pi_r_length,	// Length of received packet in words; this
				// is set after a complete reception
		*__pi_x_buffer,	// Pointer to dynamic transmission buffer
		*__pi_x_buffp,	// Next buffer word
		*__pi_x_buffl,	// LWA+1 of xmit buffer
		__pi_v_curbit,	// Current bit index
		__pi_v_status,	// Current interrupt mode (rcv/xmt/off)
		__pi_v_prmble,	// Preamble counter
		__pi_v_qevent,
		__pi_v_physid,
		__pi_v_statid,
		__pi_x_backoff;	// Calculated backoff for xmitter

word 		__pi_v_istate = IRQ_OFF;

byte 		__pi_v_rxoff,
		__pi_v_txoff,
		__pi_v_hstat,
		__pi_x_power;

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
#if 1
//
// Fixed based on George's test code, not sure if for the better (PG)
//
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

	0x8E,	// 9C,	// MODEM2
  	0x69,	// MODEM1
	0x50 | MODEM0_LEAST,	// MODEM0 (irrelevant, will be set by init)

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
#endif
#if 0
//
// Previous settings
//
	0x66,	// FREQ_2A		-> set for RX 433 MHz @ REFDIV = 9
	0xA0,	// FREQ_1A
	0x00,	// FREQ_0A

	0x41,	// FREQ_2B		-> set for TX 433 MHz @ REFDIV = 14
	0xF2,	// FREQ_1B
	0x53,	// FREQ_0B

	0x03,	// FSEP1		-> set for RX
	0x80,	// FSEP0

	0x40,	// CURRENT
	0x02,	// FRONT_END
	0x1,	// PA_POW		-> lowest power by default
	PLL_A,	// PLL			-> updated
	0x10,	// LOCK
	0x26,	// CAL

	0x8e,	// 9C,	// MODEM2
  	0x6F,	// MODEM1
	0x57,	// MODEM0	-> max baud + manchester
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
#endif
};

#endif

#ifndef	TX_CURRENT
#error	"S: Illegal CC1000 frequency, must be 433 or 868"
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

	// Reset data direction to output (this should be the default state)
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
	chp_wconf (CC1000_PA_POW, __pi_x_power);
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
	word i;
	int res;

	res = 0;

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
	for (i = 0; i < 35; i++) {
		mdelay (1);
		if ((chp_rconf (CC1000_CAL) & 0x08) != 0) {
			res |= 4;
			break;
		}
	}

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
	for (i = 0; i < 35; i++) {
		mdelay (1);
		if ((chp_rconf (CC1000_CAL) & 0x08) != 0) {
			res |= 2;
			break;
		}
	}

	// Write CAL: CAL_START=0
	chp_wconf (CC1000_CAL, 0x26);

	// Wait for the lock
	for (i = 0; i < 35; i++) {
		mdelay (1);
		if ((chp_rconf (CC1000_LOCK) & 0x01)) {
			res |= 1;
			break;
		}
	}

	return res;
}

static void ini_cc1000 (word baud) {
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
	if (baud == 0)
		baud = ((word)RADIO_DEFAULT_BITRATE/100);

	// Found the proper discrete rate
	for (i = 0; i < sizeof (chp_rates) - 1; i++)
		if (chp_rates [i] . rate >= baud)
			break;

	// Set the baud rate
	chp_wconf (CC1000_MODEM0, (chp_rates [i] . baud << 4) | MODEM0_LEAST |
		chp_rates [i] . xosc);

	i = w_calibrate (chp_rates [i] . rate);

	// Unlock the averaging filter ???
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
	if (__pi_v_hstat == status)
		return;

	switch (status) {

		case HSTAT_SLEEP:
			pins_rf_disable;
			chp_pdioout;
			xcv_disable ();
			break;
		case HSTAT_RCV:
			pins_rf_enable_rcv;
			chp_pdioin;
			if (__pi_v_hstat == HSTAT_SLEEP)
				rcv_enable_cold ();
			else
				rcv_enable_warm ();
			break;
		default:
			pins_rf_enable_xmt;
			chp_pdioout;
			if (__pi_v_hstat == HSTAT_SLEEP)
				xmt_enable_cold ();
			else
				xmt_enable_warm ();
			break;
	}

	__pi_v_hstat = status;
}

/* ========================================= */

static byte rssi_cnv (word v) {
/*
 * Converts the RSSI to a single byte 0-255
 */
	add_entropy (v);

#if 0
#if RSSI_MIN != 0
	if (v < RSSI_MIN)
		v = RSSI_MIN;
	else
		v -= RSSI_MIN;
#endif
	// Higher RSSI means lower signal level
	return (byte) ((((int)RSSI_MAX - (int)RSSI_MIN) - (int) v) >> RSSI_SHF);
#endif	/* DISABLED CODE */

#if 1
	// This is the costly but accurate way

	if (v >= RSSI_MAX)
		return 0;

	if (v <= RSSI_MIN)
		return 255;

	return (byte) ( (((lword)RSSI_MAX - (lword)v) * 255) /
		(RSSI_MAX - RSSI_MIN) );
#endif

}

#ifdef __ECOG1__
#if RADIO_LBT_DELAY > 0

static Boolean lbt_ok (word v) {

	if (v > RSSI_MAX)
		return YES;
#if 0
	diag ("LBT %d", (int)
		((((lword)RSSI_MAX - (lword)v) * 100) / (RSSI_MAX - RSSI_MIN)));
#endif
	return 
		((((lword)RSSI_MAX - (lword)v) * 100) / (RSSI_MAX - RSSI_MIN))
			< RADIO_LBT_THRESHOLD;
}

#endif	/* RADIO_LBT_DELAY */
#endif	/* __ECOG1__ */
	
#include "xcvcommon.h"

void phys_cc1000 (int phy, int mbs, int bau) {
/*
 * phy  - interface number
 * mbs  - maximum packet length (excluding checksum, must be divisible by 4)
 * bau  - baud rate /100 (default assumed if zero)
 */
	if (__pi_r_buffer != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_cc1000");

	if (mbs < 8 || mbs & 0x3) {
		if (mbs == 0)
			mbs = RADIO_DEF_BUF_LEN;
		else
			syserror (EREQPAR, "phys_cc1000 mbs");
	}

	/* For reading RSSI */
	adc_config_rssi;

	if ((__pi_r_buffer = umalloc (mbs)) == NULL)
		syserror (EMALLOC, "phys_cc1000 (b)");

	/* This is static and will never change */
	__pi_r_buffl = __pi_r_buffer + (mbs >> 1);
	/* This also indicates that there's no pending reception */
	__pi_r_buffp = 0;

	__pi_v_status = 0;

	__pi_x_power = RADIO_DEFAULT_POWER;

	__pi_v_statid = 0;
	__pi_v_physid = phy;
	__pi_x_backoff = 0;

	/* Register the phy */
	__pi_v_qevent = tcvphy_reg (phy, option, INFO_PHYS_CC1000);

	/* Both parts are initially inactive */
	__pi_v_rxoff = __pi_v_txoff = 1;
	LEDI (0, 0);
	LEDI (1, 0);
	LEDI (2, 0);

	/* Start the device */
	ini_cc1000 ((word)bau);

	__pi_v_hstat = HSTAT_SLEEP;

	if (run_rf_driver () == 0)
		syserror (ERESOURCE, "phys_cc1000");
}

static int option (int opt, address val) {
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

#include "xcvcommop.h"

	    case PHYSOPT_SENSE:

		ret = (__pi_v_status != 0);
		break;

	    case PHYSOPT_SETPOWER:

		if (val == NULL || *val == 0)
			__pi_x_power = RADIO_DEFAULT_POWER;
		else
			__pi_x_power = (*val) > 0xff ? 0xff : (byte) (*val);
		break;

	    case PHYSOPT_GETPOWER:

		ret = (int) __pi_x_power;

		if (val != NULL)
			*val = ret;

		break;

	    default:

		syserror (EREQPAR, "phys_cc1000 option");
	}
	return ret;
}
