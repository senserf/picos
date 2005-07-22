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

	if (zzv_rdbk->rssif)
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
	udelay (30);
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
	// Higher RSSI means lower signal level
	return (byte) ((v - (int) RSSI_MIN) >> RSSI_SHF);
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
				hstat (HSTAT_SLEEP);
			hard_drop;
		}
		finish;
	}
	/*
	 * Initialize things for reception. Note that the transmitter may be
	 * running at this time. We block the capture interrupts for a little
	 * while to avoid race with the transmitter.
	 */
	hard_lock;
	/* This also resets the receiver if it was running */
	zzr_buffp = zzr_buffer;
	if (!xmitter_active) {
		hstat (HSTAT_RCV);
		start_rcv;
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
			disable_xcv_timer;
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
		 * Check if should return RSSI overwriting the checksum
		 */
		if (zzv_rdbk->rssif && zzv_rdbk->chks > 1)
			zzr_buffer [zzr_length - 1] = (word) rssi_cnv ();
	}

	tcvphy_rcv (zzv_rdbk->physid, zzr_buffer, zzr_length << 1);

	proceed (RCV_GETIT);

    nodata;

endprocess (1)

static INLINE void xmt_down (void) {
/*
 * Executed when the transmitter goes down to properly set the chip
 * state
 */
	if (zzr_buffp == NULL) {
		hard_lock;
		if (zzv_rdbk->rxoff)
			hstat (HSTAT_SLEEP);
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
			xmt_down ();
			zzv_rdbk->backoff = 0;
			finish;
		}
	}

	if ((stln = tcvphy_top (zzv_rdbk->physid)) == 0) {
		/* Packet queue is empty */
		if (zzv_rdbk->txoff == 2) {
			/* Draining; stop xmt if the output queue is empty */
			zzv_rdbk->txoff = 3;
			xmt_down ();
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
	LEDI (2, 0);
	if ((zzx_buffp = tcvphy_get (zzv_rdbk->physid, &stln)) != NULL) {

		// Holding the lock

		zzx_buffer = zzx_buffp;

		/* This must be even */
		if (stln < 4 || (stln & 1) != 0)
			syserror (EREQPAR, "phys_dm2100/tlength");
		// To words
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
		hstat (HSTAT_XMT);
		zzx_buffl = zzx_buffp + stln;
		start_xmt;
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
			hstat (HSTAT_RCV);
			start_rcv;
			/* Reception commences asynchronously at hard_drop */
		}
		hard_drop;
		/* The receiver is running at this point */
		tcvphy_end (zzx_buffer);
		/* Delay for a while before next transmission */
		delay (zzv_rdbk->delmnbkf, XM_LOOP);
	} else {
		/* This is void: receiver is not active */
		hard_drop;
		sysassert (zzv_status == 0, "xmtradio illegal chip status");
		tcvphy_end (zzx_buffer);
		hstat (HSTAT_SLEEP);
		delay (zzv_rdbk->delxmspc, XM_LOOP);
	}
	release;

    nodata;

endprocess (1)

void phys_dm2100 (int phy, int mod, int mbs) {
/*
 * phy  - interface number
 * mod  - if nonzero, selects framed mode with 'mod' used as station Id
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
	adc_config;

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
		INFO_PHYS_DM2100 | (mod != 0));

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

		ret = (zzv_status != 0);
		break;

	    case PHYSOPT_SETPOWER:

		// Not implemented on TR1000
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
		 *    3 - preamble length (min = 32, max = 255)
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
				else if (pval > 255)
					pval = 255;
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
