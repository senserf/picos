#include "kernel.h"
#include "tcvphys.h"
#include "dm2100.h"

static int option (int, address);

radioblock_t *zzv_rdbk = NULL;

/*
 * We are wasting precious RAM words, but we really want to make it efficient,
 * because interrupts will have to access this AFAP.
 */
word		*zzr_buffer,	// Pointer to static reception buffer
		*zzr_buffp,	// Pointer to next buffer word; also used to
				// indicate that a reception is pending
		*zzr_buffl,	// Pointer to LWA+1 of buffer area
		*zzx_buffer,	// Pointer to dynamic transmission buffer
		*zzx_buffp,	// Next buffer word
		*zzx_buffl,	// LWA+1 of xmit buffer
		zzv_tmaux;	// To store previous value of signal timer

byte		zzv_curbit,	// Current bit index
		zzv_prmble,	// Preamble counter
		zzv_status,
		zzr_length,	// Length of received packet in words; this
				// is set after a complete reception
		zzv_curnib,	// Current nibble
		zzv_cursym,	// Symbol buffer

 		zzv_istate = IRQ_OFF;

const byte zzv_symtable [] = {
		255,		//  000000
		255,		//  000001
		255,		//  000010
		255,		//  000011
		255,		//  000100
		255,		//  000101
		255,		//  000110
		255,		//  000111
		255,		//  001000
		255,		//  001001
		255,		//  001010
		128,		//  001011	(SV2, EOP)
		255,		//  001100
		  0,		//  001101
		  1,		//  001110
		255,		//  001111
		255,		//  010000
		255,		//  010001
		255,		//  010010
		  2,		//  010011
		255,		//  010100
		  3,            //  010101
		  4,            //  010110
		255,		//  010111
		255,		//  011000
		  5,		//  011001
		  6,		//  011010
		255,		//  011011
		  7,		//  011100
		255,		//  011101
		255,		//  011110
		255,		//  011111
		255,		//  100000
		255,		//  100001
		129,		//  100010	(SV3)
		  8,		//  100011
		255,		//  100100
		  9,		//  100101
		 10,		//  100110
		255,		//  100111
		255,		//  101000
		 11,		//  101001
		 12,		//  101010
		255,		//  101011
		 13,		//  101100
		255,		//  101101
		130,		//  101110	(SV1)
		255,		//  101111
		255,		//  110000
		255,		//  110001
		 14,		//  110010
		255,		//  110011
		 15,		//  110100
		255,		//  110101
		255,		//  110110
		255,		//  110111
		255,		//  111000
		255,		//  111001
		255,		//  111010
		255,		//  111011
		255,		//  111100
		255,		//  111101
		255,		//  111110
		255		//  111111
	};

const byte zzv_nibtable [] = {
		0x05,		// 001101 ->   1100 -> 000000000101
		0x09,		// 001110 ->    120 -> 000000001001
		0x50,		// 010011 ->   0011 -> 000001010000
		0x00,		// 010101 -> 000000 -> 000000000000
		0x40,		// 010110 ->  00010 -> 000001000000
		0x14,		// 011001 ->   0110 -> 000000010100
		0x04,		// 011010 ->  01000 -> 000000000100
		0x18,		// 011100 ->    021 -> 000000011000
		0x18,		// 100011 ->    021 -> 000000011000
		0x04,		// 100101 ->  01000 -> 000000000100
		0x14,		// 100110 ->   0110 -> 000000010100
		0x40,		// 101001 ->  00010 -> 000001000000
		0x00,		// 101010 -> 000000 -> 000000000000
		0x50,		// 101100 ->   0011 -> 000001010000
		0x05,		// 110010 ->   1100 -> 000000000101
		0x41		// 110100 ->   1001 -> 000001000001
	};

const byte zzv_srntable [] = {
		3, 2, 3, 5, 4, 3, 4, 2, 2, 4, 3, 4, 5, 3, 3, 3
	};

/* ========================================= */

#include "checksum.h"

static INLINE void xmt_enable (void) {

	rssi_on;
	adc_disable;
	c0up;
	c1down;
	udelay (30);
}

static INLINE void rcv_enable (void) {

	rssi_off;
	adc_enable;
	c0up;
	c1up;
}

static INLINE void xcv_disable (void) {
/*
 * Power down
 */
	rssi_off;
	adc_disable;
	c0down;
	c1down;
}

static void ini_dm2100 (void) {
/*
 * Initialize the device
 */
	ini_regs;
	timer_init;
	diag ("DM2100 initialized: %d, %d, %d / %d, %d, %d (%d)",
		DM_RATE_X1,
		DM_RATE_X2,
		DM_RATE_X3,
		SH1,
		SH2,
		SH3,
		SH4
	);
}

static void hstat (word status) {
/*
 * Change chip status: xmt, rcv, off
 */
	switch (status) {

		case HSTAT_SLEEP:
			xcv_disable ();
			break;

		case HSTAT_RCV:
			rcv_enable ();
			break;

		default:
			xmt_enable ();
	}
}

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
	return (byte) ((v - (int) RSSI_MIN) >> RSSI_SHF);
}

#include "xcvcommon.h"

void phys_dm2100 (int phy, int mbs) {
/*
 * phy  - interface number
 * mbs  - maximum packet length (excluding checksum, must be divisible by 4)
 */
	if (zzv_rdbk != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_dm2100");

	if (mbs < 8 || mbs & 0x3) {
		if (mbs == 0)
			mbs = RADIO_DEF_BUF_LEN;
		else
			syserror (EREQPAR, "phys_dm2100 mbs");
	}

	/* For reading RSSI */
	adc_config_rssi;

	if ((zzv_rdbk = (radioblock_t*) umalloc (sizeof (radioblock_t))) ==
	    NULL)
		syserror (EMALLOC, "phys_dm2100 (r)");
	if ((zzr_buffer = umalloc (mbs)) == NULL)
		syserror (EMALLOC, "phys_dm2100 (b)");

	/* This is static and will never change */
	zzr_buffl = zzr_buffer + (mbs >> 1);
	/* This also indicates that there's no pending reception */
	zzr_buffp = 0;

	zzv_status = 0;

	zzv_rdbk->rssi = 0;

	zzv_rdbk -> statid = 0;
	zzv_rdbk -> physid = phy;
	zzv_rdbk -> backoff = 0;

	zzv_rdbk->seed = 12345;

	zzv_rdbk->delmnbkf = RADIO_DEF_MNBACKOFF;
	zzv_rdbk->delxmspc = RADIO_DEF_XMITSPACE;
	zzv_rdbk->delbsbkf = RADIO_DEF_BSBACKOFF;

	/* Register the phy */
	zzv_rdbk->qevent = tcvphy_reg (phy, option, INFO_PHYS_DM2100);

	/* Both parts are initially active */
	zzv_rdbk->rxoff = zzv_rdbk->txoff = 1;
	LEDI (0, 0);
	LEDI (1, 0);
	LEDI (2, 0);
	LEDI (3, 0);

	/* Initialize the device */
	ini_dm2100 ();
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

		ret = 0;
		break;

	    case PHYSOPT_SETPOWER:

		// Not implemented on TR1000
		break;

	    case PHYSOPT_GETPOWER:

		ret = ((int) rssi_cnv ()) & 0xff;

		if (val != NULL)
			*val = ret;

		break;

	    case PHYSOPT_SETSID:

		zzv_rdbk->statid = (val == NULL) ? 0 : *val;
		break;

	    case PHYSOPT_SETPARAM:

#define	pinx	(*val)
		/*
		 * This is the parameter index. The parameters are numbered:
		 *
		 *    0 - minimum backoff (min = 0 msec)
		 *    1 - backoff mask bits (from 1 to 15)
		 *    2 - xmit packet space (min = 0, max = 256 msec)
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

/*
 * Pin access operations
 */
int pin_get_adc (word pin, word ref, word smpt) {
/*
 * pin == 0-7 (corresponds to to P6.0-P6.7)
 * ref == 0 (1.5V) or 1 (2.5V)
 * smpt == sample time in msec (0 == 1)
 */
	int res;

	if (pin == 0 || pin > 7)
		syserror (EREQPAR, "phys_dm2100 pin_get_adc");

	if (adc_inuse)
		// We never interfere with the receiver
		return -1;

	// Take over and reset the ADC
	adc_config_read (pin, ref, smpt);
	adc_enable;
	udelay (12);
	adc_start;
	udelay (12);
	adc_wait;
	res = (int) adc_value;

	// Restore RSSI setting
	adc_config_rssi;

	return res;
}

word pin_get (word pin) {

	if (pin == 0 || pin > 11)
		syserror (EREQPAR, "phys_dm2100 pin_get");

	return pin_value (pin) != 0;
}

word pin_set (word pin, word val) {

	if (pin == 0 || pin > 11)
		syserror (EREQPAR, "phys_dm2100 pin_set");

	if (val)
		pin_sethigh (pin);
	else
		pin_setlow (pin);
}
