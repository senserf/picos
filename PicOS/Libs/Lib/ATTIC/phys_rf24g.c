#include <ecog.h>
#include <ecog1.h>
#include "kernel.h"
#include "tcvphys.h"
#include "rf24g.h"

static int option (int, address);
static void rf24g_hstat (byte);

radioblock_t *zzv_rdbk = NULL;

#define	RCV_GETIT	0
#define	RCV_CHECKIT	1

static process (rcvradio, void)

    int nw, nb;

    entry (RCV_GETIT)

	if (zzv_rdbk->rxoff) {
Finish:
		clr_rcv_int;	
		// Transmission is synchronous, so if we are here at all,
		// it means that the transmitter is not running. Thus we
		// can put the chip into a standby state.
		rf24g_hstat (HSTAT_IDLE);
		finish;
	}

	// Set to receive mode
	
	wait (rxevent, RCV_CHECKIT);
	rf24g_hstat (HSTAT_RCV);
	zzv_rdbk->rxwait = 1;
	release;

    entry (RCV_CHECKIT)

	if (zzv_rdbk->rxwait == 0)
		// This means actual reception
		gbackoff;
	else
		zzv_rdbk->rxwait = 0;

	if (zzv_rdbk->rxoff)
		// We have been switched off
		goto Finish;

	// Extract the packet
	for (nw = 0; nw < RADIO_DEF_BUF_LEN/2+1; nw++)
		// Include the length byte
		zzv_rdbk->rbuffer [nw] = 0;
	nw = 0;
	// Start from the less significant byte of the first word
	nb = 7;
	while (nw < RADIO_DEF_BUF_LEN/2) {
		// packet length is fixed at this stage, so we know exactly
		// how many bits to expect
		udelay (TINY_DELAY);
		if (rf24g_getdbit)
			zzv_rdbk->rbuffer [nw] |= (1 << nb);
		rf24g_clk0up;
		udelay (TINY_DELAY);
		rf24g_clk0down;
		if (nb == 0) {
			nw++;
			nb = 15;
		} else
			nb--;
	}

#if 0
	diag ("RCV: %d %x %x %x %x %x %x",
		zzv_rdbk->rbuffer [0],
		zzv_rdbk->rbuffer [1],
		zzv_rdbk->rbuffer [2],
		zzv_rdbk->rbuffer [3],
		zzv_rdbk->rbuffer [4],
		zzv_rdbk->rbuffer [5]);
#endif

	// Find out the actual length of the packet
	nw = zzv_rdbk->rbuffer [0];
	if (nw > RADIO_DEF_BUF_LEN || nw == 0)
		// This is crap
		proceed (RCV_GETIT);

	/* Check the station Id */
	if (zzv_rdbk->statid != 0 && zzv_rdbk->rbuffer [1] != 0 &&
	    zzv_rdbk->rbuffer [1] != zzv_rdbk->statid)
		/* Wrong packet */
		proceed (RCV_GETIT);

	// Receive it
	tcvphy_rcv (zzv_rdbk->physid, zzv_rdbk->rbuffer+1, nw);

	proceed (RCV_GETIT);

    nodata;

endprocess (1)

#define	XM_LOOP		0
#define XM_TXDONE	1

static process (xmtradio, void)

    int stln, nw, nb;
    word *xbuffp;

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
			zzv_rdbk->backoff = 0;
			finish;
		}
	}

	if ((stln = tcvphy_top (zzv_rdbk->physid)) == 0) {
		/* Packet queue is empty */
		if (zzv_rdbk->txoff == 2) {
			/* Draining; stop xmt if the output queue is empty */
			zzv_rdbk->txoff = 3;
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

	if ((xbuffp = tcvphy_get (zzv_rdbk->physid, &stln)) != NULL) {

		if (stln < 1 || stln > RADIO_DEF_BUF_LEN)
			syserror (EREQPAR, "phys_rf24g/tlength");
#if 0
		diag ("SND: %d (%x) %x %x %x %x %x %x", stln, (word) xbuffp,
			xbuffp [0],
			xbuffp [1],
			xbuffp [2],
			xbuffp [3],
			xbuffp [4],
			xbuffp [5]);
#endif
		// Make sure the receiver won't interfere
		clr_rcv_int;
		rf24g_hstat (HSTAT_XMT);

		// Clock in the address byte
		for (nb = 7; nb >= 0; nb--) {
			if (((zzv_rdbk->group >> nb) & 1) != 0)
				rf24g_dataup;
			else
				rf24g_datadown;
			udelay (TINY_DELAY);
			rf24g_clk0up;
			udelay (TINY_DELAY);
			rf24g_clk0down;
udelay (TINY_DELAY);
		}

		// Clock in the length byte
		for (nb = 7; nb >= 0; nb--) {
			if (((stln >> nb) & 1) != 0)
				rf24g_dataup;
			else
				rf24g_datadown;
			udelay (TINY_DELAY);
			rf24g_clk0up;
			udelay (TINY_DELAY);
			rf24g_clk0down;
udelay (TINY_DELAY);
		}

		// Clock in the buffer
		stln = (stln + 1) >> 1;

		nw = 0;
		nb = 15;

		while (nw < stln) {
			if (((xbuffp [nw] >> nb) & 1) != 0)
				rf24g_dataup;
			else
				rf24g_datadown;
			udelay (TINY_DELAY);
			rf24g_clk0up;
			udelay (TINY_DELAY);
			rf24g_clk0down;
			if (nb == 0) {
				nb = 15;
				nw++;
			} else 
				nb--;
udelay (TINY_DELAY);
		}

		// Complete the packet up to the full length
		while (nw < RADIO_DEF_BUF_LEN) {
			rf24g_datadown;
			udelay (TINY_DELAY);
			rf24g_clk0up;
			udelay (TINY_DELAY);
			rf24g_clk0down;
			if (nb == 0) {
				nb = 15;
				nw++;
			} else 
				nb--;
udelay (TINY_DELAY);
		}

		// Done, set CE low and the packet will be sent
		rf24g_cedown;
		// Release the buffer
#if 0
		diag ("SND: DONE (%x)", (word) xbuffp);
#endif
		tcvphy_end (xbuffp);
		// Give it a chance to transmit
		if (zzv_rdbk->bitrate)
			delay (1, XM_TXDONE);
		else
			delay (3, XM_TXDONE);
		release;
	}

	// We should never get here
	proceed (XM_LOOP);

    entry (XM_TXDONE)


	clr_rcv_int;	// Just in case
	if (zzv_rdbk->rxwait) {
		rf24g_hstat (HSTAT_RCV);
		/* Delay for a while before next transmission */
		delay (zzv_rdbk->delmnbkf, XM_LOOP);
		release;
	}

	delay (zzv_rdbk->delxmspc, XM_LOOP);
	release;

    nodata;

endprocess (1)

static void cnfg (const byte *par, int nbits) {
/*
 * Write a configuration sequence to the device
 */
	int bn;
#if 1
	if (nbits > 1) {
		diag ("CNFG: %d bits", nbits);
		for (bn = 0; bn < nbits/16; bn++)
			diag ("%d -- %d -> %x",
				nbits - (bn + 1) * 16, nbits - bn*16 - 1,
					((word) par [bn * 2] << 8) |
						par [bn * 2 + 1] );
		if ((nbits % 16) != 0)
			diag ("%d -- %d -> %x", 0, 7,
				(word) par [nbits/8 - 1]);

		
	}
#endif
	rf24g_clk0down;
	rf24g_dataout;
	rf25g_cedown;
	rf24g_csup;

	bn = 7;
	while (nbits--) {
		if (bn < 0) {
			par++;
			bn = 7;
		}
		if ((((*par) >> bn) & 1) == 0)
			rf24g_datadown;
		else
			rf24g_dataup;
		udelay (TINY_DELAY);	// > 500nsec
		rf24g_clk0up;
		udelay (TINY_DELAY);	// > 500nsec
		rf24g_clk0down;
udelay (TINY_DELAY);
		bn--;
	}

	udelay (TINY_DELAY);
	rf24g_csdown;
	rf25g_cedown;
}

static void rf24g_hstat (byte status) {
/*
 * This is all about the most significant bit
 */
	rf24g_cedown;
	rf24g_csdown;
	rf24g_clk0down;
	if (status == HSTAT_IDLE)
		return;
	if (status == HSTAT_RCV)
		set_rcv_int;
	cnfg (&status, 1);
	if (status == HSTAT_RCV) {
		rf24g_datain;
		rf24g_ceup;
	} else {
		rf24g_dataout;
	}
}

static void rf24g_ini (void) {
/*
 * Initialize the device
 */
	byte *par;

	ini_regs;

	if ((par = (byte*) umalloc (PARBLOCK_SIZE)) == NULL)
		syserror (EMALLOC, "phys_rf24g (p)");

	memset (par, 0, PARBLOCK_SIZE);

	par [PARBLOCK_SIZE-1 - 0] = (byte) (
		  (1 << 0) 	/* Receive mode */
		| (zzv_rdbk->channel << 1)
	);
	par [PARBLOCK_SIZE-1 - 1] = (byte) (
		  (RADIO_DEF_XPOWER << 0)
		| (3 << 2) 	/* Crystal freq 16 MHZ */
		| (zzv_rdbk->bitrate << 5)
		| (1 << 6) 	/* ShockBurst mode */
		| (0 << 7) 	/* One channel receive */
	) ;
	par [PARBLOCK_SIZE-1 - 2] = (byte) (
		  (1 << 0) 	/* CRC enabled */
		| (1 << 1) 	/* 16 bit CRC */
		| (16 << 2) 	/* 8 address bits ????? */
	);
	par [PARBLOCK_SIZE-1 - 3] = (byte) (
		  zzv_rdbk->group		/* Group address */
	);
	// 3-7	ADDR1
	// 8-12	ADDR2 (unused)
	par [PARBLOCK_SIZE-1 - 13] = (byte) (	/* Payload length */
		  RADIO_DEF_BUF_LEN * 8
	);
	// 14	payload length 2, unused

	// Wait for >3 msec -> power up to standby
	mdelay (4);
	cnfg (par, PARBLOCK_SIZE * 8);
	ufree (par);
}

static void rf24g_setpower (int pow) {

	byte par [2];

	// Move the receiver out of the way
	clr_rcv_int;
	rf24g_hstat (HSTAT_IDLE);

	par [0] = (byte) (
		((pow & 0x3) << 0)
		| (3 << 2) 	/* Crystal freq 16 MHZ */
		| (zzv_rdbk->bitrate << 5)
		| (1 << 6) 	/* ShockBurst mode */
		| (0 << 7) 	/* One channel receive */
	);

	par [1] = (byte) (
		  (1 << 0) 	/* Receive mode */
		| (zzv_rdbk->channel << 1)
	);
	cnfg (par, 2 * 8);
	udelay (TINY_DELAY);

	// Restore receiver status
	if (zzv_rdbk->rxwait)
		rf24g_hstat (HSTAT_RCV);
}

void phys_rf24g (int phy, int grp, int mod, int chan, int bau) {
/*
 * phy  - interface number
 * mod  - if nonzero, selects framed mode with 'mod' used as station Id
 * bau  - physical transmission speed (250 or 1000)
 */
	if (zzv_rdbk != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_rf24g");

	if ((zzv_rdbk = (radioblock_t*) umalloc (sizeof (radioblock_t))) ==
	    NULL)
		syserror (EMALLOC, "phys_rf24g (r)");
	// We need one extra word in front to accommodate the length field.
	// In fact, we are only using the less significant byte of it.
	if ((zzv_rdbk->rbuffer = umalloc (RADIO_DEF_BUF_LEN+2)) == NULL)
		syserror (EMALLOC, "phys_rf24g (b)");

	zzv_rdbk -> statid = mod;
	zzv_rdbk -> physid = phy;
	zzv_rdbk -> group = (byte) grp;
	zzv_rdbk -> backoff = 0;
	zzv_rdbk -> channel = (byte) chan;
	zzv_rdbk -> rxwait = 0;

	if (bau == 0)
		bau = RADIO_DEF_BITRATE;
	else if (bau == 250)
		bau = 0;
	else if (bau == 1000)
		bau = 1;
	else
		syserror (EMALLOC, "phys_rf24g bitrate");

	zzv_rdbk->bitrate = (byte) bau;

	zzv_rdbk->seed = 12345;

	zzv_rdbk->delmnbkf = RADIO_DEF_MNBACKOFF;
	zzv_rdbk->delxmspc = RADIO_DEF_XMITSPACE;
	zzv_rdbk->delbsbkf = RADIO_DEF_BSBACKOFF;

	/* Register the phy */
	zzv_rdbk->qevent = tcvphy_reg (phy, option,
		INFO_PHYS_RF24G | (mod != 0));

	/* Both parts are initially active */
	zzv_rdbk->rxoff = zzv_rdbk->txoff = 0;
	LEDI (2, 1);
	LEDI (3, 1);

	/* Start the device */
	rf24g_ini ();

	fork (xmtradio, NULL);
	fork (rcvradio, NULL);
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
		LEDI (3, 1);
		if (!running (xmtradio))
			fork (xmtradio, NULL);
		trigger (zzv_rdbk->qevent);
		break;

	    case PHYSOPT_RXON:

		zzv_rdbk->rxoff = 0;
		LEDI (2, 1);
		if (!running (rcvradio))
			fork (rcvradio, NULL);
		trigger (rxevent);
		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		zzv_rdbk->txoff = 2;
		LEDI (3, 0);
		trigger (zzv_rdbk->qevent);
		break;

	    case PHYSOPT_TXHOLD:

		zzv_rdbk->txoff = 1;
		LEDI (3, 0);
		trigger (zzv_rdbk->qevent);
		break;

	    case PHYSOPT_RXOFF:

		zzv_rdbk->rxoff = 1;
		LEDI (2, 0);
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

		// This one is for compatibility
		ret =  0;
		break;

	    case PHYSOPT_SETPOWER:

		if (val == NULL || *val == 0)
			rf24g_setpower (RADIO_DEF_XPOWER);
		else
			rf24g_setpower (*val > 3 ? 3 : *val);
		break;

	    case PHYSOPT_GETPOWER:

		// For compatibility
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
