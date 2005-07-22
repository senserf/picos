#include "kernel.h"
#include "tcvphys.h"
#include "chipcon.h"

static int option (int, address);

radioblock_t *zzv_rdbk = NULL;

/*
 * We are wasting precious RAM words, but we really want to make it efficient,
 * because interrupts will have to access this at CHIPCON's data clock rate.
 */
word		*zzr_buffer,	// Pointer to static reception buffer
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
		zzv_prmble;	// Preamble counter

word 		zzv_istate = IRQ_OFF;

#if CHIPCON_FREQ == 868

#define TX_CURRENT	0xF3
#define RX_CURRENT	0x88
#define PLL_A		0x38
#define PLL_B		0x30
#define MODEM0_LEAST	0x05

static const byte chp_defcA [] = {
/*
 * Default contents of CHIPCON registers
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

#if CHIPCON_FREQ == 433

#define TX_CURRENT	0x81
#define RX_CURRENT	0x40
#define PLL_A		0x70
#define PLL_B		0x48
#define MODEM0_LEAST	0x04

static const byte chp_defcA [] = {
/*
 * Default contents of CHIPCON registers
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
#error	ILLEGAL CHIPCON FREQUENCY, MUST BE 433 OR 868
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
	chp_wconf (CC1000_PA_POW, zzv_rdbk->xpower);
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

static void ini_chipcon (int baud) {
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

	diag ("CHIPCON 1000 %u MHz calibrated at %u00 bps, status = %u",
		(CHIPCON_FREQ == 868 ? 868 : 433), baud, i);
}

static void chp_hstat (word status) {
/*
 * Change chip status: xmt, rcv, sleep
 */
	if (zzv_rdbk->hstat == status)
		return;

	switch (status) {

		case HSTAT_SLEEP:
			xcv_disable ();
			break;
		case HSTAT_RCV:
			if (zzv_rdbk->hstat == HSTAT_SLEEP)
				rcv_enable_cold ();
			else
				rcv_enable_warm ();
			break;
		default:
			if (zzv_rdbk->hstat == HSTAT_SLEEP)
				xmt_enable_cold ();
			else
				xmt_enable_warm ();
			break;
	}

	zzv_rdbk->hstat = status;
}

/* ========================================= */

static byte rssi_cnv (void) {
/*
 * Converts the RSSI to a single byte 0-255
 */
	int v;

	v = zzv_rdbk->rssi;

#if RSSI_MIN >= 0x8000
	// RSSI is signed
	if ((v & RSSI_MIN))
		v |= RSSI_MIN;
#endif

#if RSSI_MIN != 0
	v -= RSSI_MIN;
#endif
	// Higher RSSI means lower signal level
	return (byte) ((((int)RSSI_MAX - (int)RSSI_MIN) - v) >> RSSI_SHF);
}

/* ========================================= */

#define	RCV_GETIT	0
#define	RCV_CHECKIT	1

static process (rcvradio, void)

    entry (RCV_GETIT)

	if (zzv_rdbk->rxoff) {
Finish:
		if (zzv_rdbk->txoff) {
			hard_lock;
			if (!xmitter_active)
				chp_hstat (HSTAT_SLEEP);
			hard_drop;
		}
		finish;
	}
	/*
	 * Initialize things for reception. Note that the transmitter may be
	 * running at this time. We block the CHIPCON clock for a tiny little
	 * while to avoid race with the transmitter.
	 */
	hard_lock;
	/* This also resets the receiver if it was running */
	zzr_buffp = zzr_buffer;
	if (!xmitter_active) {
		chp_hstat (HSTAT_RCV);
		irq_start_rcv;
	}
	hard_drop;
	/*
	 * Formally, we have a race here because we are not holding the lock
	 * while issuing the wait. However, it is physically impossible for
	 * any reception to complete while we are finishing this state.
	 */
	wait (rxevent, RCV_CHECKIT);
	release;

    entry (RCV_CHECKIT)

	/* Tell the receiver to stop in case we have been interrupted */
	if (receiver_active) {
		hard_lock;
		zzr_buffp = NULL;
		if (receiver_active) {
			zzv_status = 0;
			zzv_istate = IRQ_OFF;
			gbackoff;
		}
		hard_drop;
	}

	if (zzv_rdbk->rxoff)
		/* rcvevent can be also triggered by this */
		goto Finish;

#if 0
	diag ("RCV: %d %x %x %x %x %x %x", zzr_length,
		zzr_buffer [0],
		zzr_buffer [1],
		zzr_buffer [2],
		zzr_buffer [3],
		zzr_buffer [4],
		zzr_buffer [5]);
#endif
	/* The length is in words and must be at least 2 (4 bytes) */
	if (zzr_length < 2)
		proceed (RCV_GETIT);

	/* Check the station Id */
	if (zzv_rdbk->statid != 0 && zzr_buffer [0] != 0 &&
	    zzr_buffer [0] != zzv_rdbk->statid)
		/* Wrong packet */
		proceed (RCV_GETIT);

	/* Validate checksum */
	if (zzv_rdbk->chks) {
		if (w_chk (zzr_buffer, zzr_length))
			proceed (RCV_GETIT);
		/*
		 * Check if should return RSSI in the last checksum byte
		 */
		if (zzv_rdbk->rssif && zzv_rdbk->chks > 1)
			zzr_buffer [zzr_length - 1] = (word) rssi_cnv ();
	}

	tcvphy_rcv (zzv_rdbk->physid, zzr_buffer, zzr_length << 1);

	proceed (RCV_GETIT);

    nodata;

endprocess (1)

static INLINE void chp_xmt_down (void) {
/*
 * Executed when the transmitter goes down to properly set the chip
 * state
 */
	if (zzr_buffp == NULL) {
		hard_lock;
		chp_hstat (zzv_rdbk->rxoff ? HSTAT_SLEEP : HSTAT_RCV);
		hard_drop;
	}
}
	
#define	XM_LOOP		0
#define XM_TXDONE	1

static process (xmtradio, void)

    int stln;

    entry (XM_LOOP)

	if (zzv_rdbk->txoff) {
		/* We are off */
		if (zzv_rdbk->txoff == 3) {
Drain:
			tcvphy_erase (zzv_rdbk->physid);
			wait (zzv_rdbk->qevent, XM_LOOP);
			release;
		} else if (zzv_rdbk->txoff == 1) {
			/* Queue held, transmitter off */
			chp_xmt_down ();
			zzv_rdbk->backoff = 0;
			finish;
		}
	}

	if ((stln = tcvphy_top (zzv_rdbk->physid)) == 0) {
		/* Packet queue is empty */
		if (zzv_rdbk->txoff == 2) {
			/* Draining; stop xmt if the output queue is empty */
			zzv_rdbk->txoff = 3;
			chp_xmt_down ();
			/* Redo */
			goto Drain;
		}
		wait (zzv_rdbk->qevent, XM_LOOP);
		release;
	}

	if (zzv_rdbk->backoff && stln < 2) {
		/* We have to wait and the packet is not urgent */
		delay (zzv_rdbk->backoff, XM_LOOP);
		zzv_rdbk->backoff = 0;
		wait (zzv_rdbk->qevent, XM_LOOP);
		release;
	}

	hard_lock;
	if (receiver_busy) {
		hard_drop;
		delay (zzv_rdbk->delmnbkf, XM_LOOP);
		wait (zzv_rdbk->qevent, XM_LOOP);
		release;
	}
	if ((zzx_buffp = tcvphy_get (zzv_rdbk->physid, &stln)) != NULL) {
		/*
		 * We are holding the lock while doing this, but that's OK.
		 * We have to do it anyway, and being greedy for the radio
		 * resource doesn't pay.
		 */

		zzx_buffer = zzx_buffp;

		/* This must be even */
		if (stln < 4 || (stln & 1) != 0)
			syserror (EREQPAR, "phys_chipcon/tlength");
		stln >>= 1;

		if (zzv_rdbk->chks) {
			zzx_buffp [stln - 1] =
				w_chk (zzx_buffp, stln - 1);
		}
#if 0
		diag ("SND: %d (%x) %x %x %x %x %x %x", stln, (word) zzx_buffp,
			zzx_buffer [0],
			zzx_buffer [1],
			zzx_buffer [2],
			zzx_buffer [3],
			zzx_buffer [4],
			zzx_buffer [5]);
#endif
		chp_hstat (HSTAT_XMT);
		zzx_buffl = zzx_buffp + stln;
		irq_start_xmt;
		/*
		 * Now, txevent is used exclusively for the end of transmission.
		 * We HAVE to perceive it, as we are responsible for restarting
		 * a pending reception.
		 */
		wait (txevent, XM_TXDONE);
		hard_drop;
		release;
	}
	hard_drop;
	// We should never get here
	proceed (XM_LOOP);

    entry (XM_TXDONE)

#if 0
	diag ("SND: DONE (%x)", (word) zzx_buffp);
#endif

	/* Restart a pending reception */
	hard_lock;
	if (zzr_buffp != NULL) {
		/* Reception queued or active */
		if (!receiver_active) {
			/* Not active, we can keep the lock for a while */
			chp_hstat (HSTAT_RCV);
			irq_start_rcv;
			/* Reception commences asynchronously at hard_drop */
		}
		hard_drop;
		/* The receiver is running at this point */
		tcvphy_end (zzx_buffer);
		/* Delay for a while before next transmission */
		delay (zzv_rdbk->delmnbkf, XM_LOOP);
		release;
	} else {
		/* This is void: receiver is not active */
		hard_drop;
		sysassert (zzv_status == 0, "xmtradio illegal chip status");
		tcvphy_end (zzx_buffer);
		if (tcvphy_top (zzv_rdbk->physid)) {
			/* More to xmit: keep the transmitter up */
			delay (zzv_rdbk->delxmspc, XM_LOOP);
			release;
		}
		/* Shut down the transmitter */
		chp_hstat (HSTAT_RCV);
		proceed (XM_LOOP);
	}

    nodata;

endprocess (1)

void phys_chipcon (int phy, int mod, int mbs, int bau) {
/*
 * phy  - interface number
 * mod  - if nonzero, selects framed mode with 'mod' used as station Id
 * mbs  - maximum packet length (excluding checksum, must be divisible by 4)
 * bau  - baud rate /100 (default assumed if zero)
 */
	if (zzv_rdbk != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_chipcon");

	if (mbs < 8 || mbs & 0x3) {
		if (mbs == 0)
			mbs = RADIO_DEF_BUF_LEN;
		else
			syserror (EREQPAR, "phys_chipcon mbs");
	}

	/* For reading RSSI */
	adc_config;

	if ((zzv_rdbk = (radioblock_t*) umalloc (sizeof (radioblock_t))) ==
	    NULL)
		syserror (EMALLOC, "phys_chipcon (r)");
	if ((zzr_buffer = umalloc (mbs)) == NULL)
		syserror (EMALLOC, "phys_chipcon (b)");

	/* This is static and will never change */
	zzr_buffl = zzr_buffer + (mbs >> 1);
	/* This also indicates that there's no pending reception */
	zzr_buffp = 0;

	zzv_status = 0;

	zzv_rdbk->xpower = RADIO_DEF_XPOWER;
	zzv_rdbk->rssi = 0;
	zzv_rdbk->rssif = 0;	// Flag == collect RSSI

	zzv_rdbk -> statid = mod;
	zzv_rdbk -> physid = phy;
	zzv_rdbk -> backoff = 0;

	zzv_rdbk->seed = 12345;

	zzv_rdbk->delmnbkf = RADIO_DEF_MNBACKOFF;
	zzv_rdbk->delxmspc = RADIO_DEF_XMITSPACE;
	zzv_rdbk->delbsbkf = RADIO_DEF_BSBACKOFF;
	zzv_rdbk->preamble = RADIO_DEF_PREAMBLE;
	/* Checksum used by default */
	zzv_rdbk->chks = RADIO_DEF_CHECKSUM;

	/* Register the phy */
	zzv_rdbk->qevent = tcvphy_reg (phy, option,
		INFO_PHYS_CHIPCON | (mod != 0));

	/* Both parts are initially inactive */
	zzv_rdbk->rxoff = zzv_rdbk->txoff = 1;
	LEDI (0, 0);
	LEDI (1, 0);

	/* Start the device */
	ini_chipcon (bau);

	zzv_rdbk->hstat = HSTAT_SLEEP;
}

static int option (int opt, address val) {
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = ((zzv_rdbk->txoff == 0) << 1) | (zzv_rdbk->rxoff == 0);
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		zzv_rdbk->txoff = 0;
		LEDI (1, 1);
		if (!running (xmtradio))
			fork (xmtradio, NULL);
		trigger (zzv_rdbk->qevent);
		break;

	    case PHYSOPT_RXON:

		zzv_rdbk->rxoff = 0;
		LEDI (0, 1);
		if (!running (rcvradio))
			fork (rcvradio, NULL);
		trigger (rxevent);
		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		zzv_rdbk->txoff = 2;
		LEDI (1, 0);
		trigger (zzv_rdbk->qevent);
		break;

	    case PHYSOPT_TXHOLD:

		zzv_rdbk->txoff = 1;
		LEDI (1, 0);
		trigger (zzv_rdbk->qevent);
		break;

	    case PHYSOPT_RXOFF:

		zzv_rdbk->rxoff = 1;
		LEDI (0, 0);
		trigger (rxevent);
		break;

	    case PHYSOPT_CAV:

		/* Force an explicit backoff */
		if (val == NULL)
			zzv_rdbk->backoff = 0;
		else
			zzv_rdbk->backoff = *val;
		trigger (zzv_rdbk->qevent);
		break;

	    case PHYSOPT_SENSE:

		ret = (zzv_status != 0);
		break;

	    case PHYSOPT_SETPOWER:

		if (val == NULL || *val == 0)
			zzv_rdbk->xpower = RADIO_DEF_XPOWER;
		else
			zzv_rdbk->xpower = (*val) > 0xff ? 0xff : (byte) (*val);
		break;

	    case PHYSOPT_GETPOWER:

		if (zzv_rdbk->rssif)
			ret = ((int) rssi_cnv ()) & 0xff;
		else
			ret = 0;

		if (val != NULL)
			*val = ret;

		break;

	    case PHYSOPT_SETPARAM:

#define	pinx	(*val)
		/*
		 * This is the parameter index. The parameters are numbered:
		 *
		 *    0 - minimum backoff (min = 0 msec)
		 *    1 - backoff mask bits (from 1 to 15)
		 *    2 - xmit packet space (min = 0, max = 256 msec)
		 *    3 - preamble length (min = 32, max = 256)
		 *    4 - checksum mode (0, 1-checksum, 2-checksum+power level
		 *    5 - collect RSSI
		 */
#define pval	(*(val + 1))
		/*
		 * This is the value. We do some checking here and make sure
		 * that the values are within range.
		 */
		switch (pinx) {
			case 0:
				if (pval > 32767)
					pval = 32767;
				zzv_rdbk->delmnbkf = pval;
				break;
			case 1:
				if (pval > 15)
					pval = 15;
				if (pval)
					pval = (1 << pval) - 1;
				zzv_rdbk->delbsbkf = pval;
				break;
			case 2:
				if (pval > 256)
					pval = 256;
				if ((zzv_rdbk->delxmspc = pval) >
				    zzv_rdbk->delmnbkf)
					zzv_rdbk->delmnbkf = pval;
				break;
			case 3:
				if (pval < 32)
					pval = 32;
				else if (pval > 256)
					pval = 256;
				zzv_rdbk->preamble = pval;
				break;
			case 4:
				if (pval > 2)
					pval = 2;
				zzv_rdbk->chks = pval;
				break;
			case 5:
				if (pval != 0) {
					/* Collect RSSI */
					zzv_rdbk->rssif = 1;
				} else {
					zzv_rdbk->rssif = 0;
				}
				break;
			default:
				syserror (EREQPAR, "options radio param index");
		}
#undef	pinx
#undef	pval
		ret = 1;
		break;
	}
	return ret;
}
