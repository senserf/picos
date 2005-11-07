/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "tcvphys.h"
#include "rf24g.h"

static int option (int, address);

/*
 * We are wasting precious RAM words, but we really want to make it efficient,
 * because interrupts will have to access this AFAP.
 */
				// Pointer to static reception buffer
word		*zzr_buffer = NULL,
		zzv_qevent,
		zzv_physid,
		zzv_statid,
		zzx_seed,		// For the random number generator
		zzx_backoff;		// Calculated backoff for xmitter

byte		zzv_rxoff,		// Transmitter on/off flags
		zzv_txoff,
		zzv_paylen,		// Formal payload length in bytes
		zzx_power,		// Transmit power (0-3)
		zzv_group,
		zzv_channel;		// Channel

/* ========================================= */

static void cnfg (const byte *par, int nbits) {
/*
 * Write a configuration sequence to the chip
 */
	int bn;
#if 0
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
	clk1_down;
	data_out;
	ce_down;
	cs_up;

	bn = 7;
	while (nbits--) {
		if (bn < 0) {
			par++;
			bn = 7;
		}
		if ((((*par) >> bn) & 1) == 0)
			data_down;
		else
			data_up;
		udelay (1);		// > 500nsec
		clk1_up;
		udelay (1);		// > 500nsec
		clk1_down;
		udelay (1);		// > 500nsec
		bn--;
	}
	udelay (1);
	ce_down;
	cs_down;
}

static void boot () {
/*
 * Complete reset
 */
#define	par	((byte*)zzr_buffer)

	par [0] = (zzv_paylen + 3) << 3;	// Packet length in bits

	par  [1] = 0;	
	par  [2] = 0;
	par  [3] = 0;
	par  [4] = 0;
	par  [5] = 200;				// Chan 2 address (irrelevant)

	par  [6] = 0;	
	par  [7] = 0;
	par  [8] = 0;
	par  [9] = 0;
	par [10] = zzv_group;			// Chan 1 address (group)

	// par [11] = (8 << 2) | 3;		// Addr width + CRC
	par [11] = (8 << 2) | 1;		// Addr width + CRC
	// 1 channel, Burst, 256 kbps, 16MHz
	par [12] = (1 << 6) | (3 << 2) | (zzx_power & 0x3);
	par [13] = (zzv_channel << 1) | 0;	// XMT enable

	cnfg (par, 14 * 8);

#undef par
}

static INLINE void xmt_enable (void) {

	byte p;

	if (stat_get (FLG_XMTE))
		// Already enabled
		return;
	p = 0;
	cnfg (&p, 1);
	stat_set (FLG_XMTE);
}

static void rcv_enable (void) {

	byte p;

	if (stat_get (FLG_XMTE)) {
		cnfg (&p, 1);
		stat_clr (FLG_XMTE);
	}
}

static void rcv_disable (void) {

	ce_down;
	xmt_enable ();
}

static void snd (const byte *p, int len) {

#define	sndbit(b)	do { \
				if ((*p & (b))) \
					data_up; \
				else \
					data_down; \
				udelay (1); \
				clk1_up; \
				udelay (1); \
				clk1_down; \
				udelay (1); \
			} while (0)

	while (len--) {
		sndbit (0x80);
		sndbit (0x40);
		sndbit (0x20);
		sndbit (0x10);
		sndbit (0x08);
		sndbit (0x04);
		sndbit (0x02);
		sndbit (0x01);
		p++;
	}

#undef	sndbit
}

static void rcv (byte *p, int len) {

#define	rcvbit(b)	do { \
				if (data_val) \
					*p |= (b); \
				udelay (1); \
				clk1_up; \
				udelay (1); \
				clk1_down; \
				udelay (1); \
			} while (0)

	while (len--) {
		*p = 0;
		rcvbit (0x80);
		rcvbit (0x40);
		rcvbit (0x20);
		rcvbit (0x10);
		rcvbit (0x08);
		rcvbit (0x04);
		rcvbit (0x02);
		rcvbit (0x01);
		p++;
	}
					
#undef rcvbit
}

#define	RCV_GETIT		0
#define	RCV_CHECKIT		1

static process (rcvradio, void)

    byte nb;

    entry (RCV_GETIT)

	if (zzv_rxoff) {
Finish:
		rcv_disable ();
		finish;
	}

	wait (rxevent, RCV_CHECKIT);
	rcv_enable ();
	stat_set (FLG_RCVA + FLG_RCVI);
	ce_up;
	set_rcv_int;
	release;
	
    entry (RCV_CHECKIT)

	// Just in case, should have been cleared by the interrupt
	ce_down;
	stat_clr (FLG_RCVA);

	if (zzv_rxoff)
		goto Finish;

	gbackoff;

	data_in;

	/* Strobe in the payload length */
	rcv (&nb, 1);

	/* Receive the packet */
	rcv ((byte*)zzr_buffer, zzv_paylen);
#if 1
	diag ("RCV: %d %x %x %x %x %x %x", nb,
		zzr_buffer [0],
		zzr_buffer [1],
		zzr_buffer [2],
		zzr_buffer [3],
		zzr_buffer [4],
		zzr_buffer [5]);
#endif
	if (nb > zzv_paylen || nb == 0)
		/* Ignore as garbage */
		proceed (RCV_GETIT);

	/* Check the station Id */
	if (zzv_statid != 0 && zzr_buffer [0] != 0 &&
	    zzr_buffer [0] != zzv_statid)
		/* Wrong packet */
		proceed (RCV_GETIT);

	tcvphy_rcv (zzv_physid, zzr_buffer, nb);

	proceed (RCV_GETIT);

    nodata;

endprocess (1)

#define	XM_LOOP		0

static process (xmtradio, void)

    int stln;
    address buffp;
    byte bb;

    entry (XM_LOOP)

	if (zzv_txoff) {
		/* We are off */
		if (zzv_txoff == 3) {
Drain:
			tcvphy_erase (zzv_physid);
			wait (zzv_qevent, XM_LOOP);
			release;
		} else if (zzv_txoff == 1) {
			/* Queue held, transmitter off */
			zzx_backoff = 0;
			finish;
		}
	}

	if ((stln = tcvphy_top (zzv_physid)) == 0) {
		/* Packet queue is empty */
		if (zzv_txoff == 2) {
			/* Draining; stop xmt if the output queue is empty */
			zzv_txoff = 3;
			/* Redo */
			goto Drain;
		}
		wait (zzv_qevent, XM_LOOP);
		release;
	}

	if (zzx_backoff && stln < 2) {
		/* We have to wait and the packet is not urgent */
		delay (zzx_backoff, XM_LOOP);
		zzx_backoff = 0;
		wait (zzv_qevent, XM_LOOP);
		release;
	}

	if ((buffp = tcvphy_get (zzv_physid, &stln)) != NULL) {

		LEDI (2, 1);

		clr_rcv_int;
		xmt_enable ();

		data_out;
	
		ce_up;

		snd (&zzv_group, 1);
		if (stln > zzv_paylen)
			syserror (EREQPAR, "rxmt/tlength");

		bb = (byte) stln;
		snd (&bb, 1);
		snd ((byte*) buffp, stln);

		bb = 0xaa;
		while (stln++ < zzv_paylen)
			snd (&bb, 1);

		ce_down;

		LEDI (2, 0);
		tcvphy_end (buffp);

		if (stat_get (FLG_RCVA)) {
			rcv_enable ();
			ce_up;
			if (stat_get (FLG_RCVI))
				set_rcv_int;
		}

		delay (MIN_BACKOFF, XM_LOOP);
	}

	proceed (XM_LOOP);
		
    nodata;

endprocess (1)

void phys_rf24g (int phy, int grp, int chn) {
/*
 * phy  - interface number
 * grp  - group (ipart of RF24G addressing scheme)
 * chn  - channel
 */
	if (zzr_buffer != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_rf24g");

	/*
	 * We book the maximum-size buffer, which, for reception, is 28 bytes
	 * (== 32 bytes - 4 bytes of overhead). The complete packet, as
	 * submitted for transmission, consists of:
	 *
	 *	1 byte RF24G address (this is what we mean by the group)
	 *	1 byte length (the actual length of the packet, excluding
	 *	  the RF24G address byte and CRC) denoted by L
	 *	2 bytes logical station address (counts to length)
	 *	L bytes of data
	 *
	 * Thus, the maximum length of a reception buffer (which never includes
	 * the first two bytes) is 32 - 2 - 2 == 28. Note that the CRC is 16
	 * bits long.
	 */

	if ((zzr_buffer = umalloc (28)) == NULL)
		syserror (EMALLOC, "phys_dm2100");

	zzv_statid = 0;
	zzv_physid = phy;
	zzv_paylen = 28;
	zzx_backoff = 0;
	zzx_power = 3;			// Maximum power
	zzv_group = (byte) grp;
	zzv_channel = (byte) chn;

	zzx_seed = 12345;

	/* Register the phy */
	zzv_qevent = tcvphy_reg (phy, option, INFO_PHYS_DM2100);

	/* Both parts are initially active */
	LEDI (0, 0);
	LEDI (1, 0);
	LEDI (2, 0);
	LEDI (3, 0);
	zzv_rxoff = zzv_txoff = 1;
	/* Initialize the device */
	ini_regs;
	boot ();
	stat_set (FLG_XMTE);
}

static void reboot () {

	clr_rcv_int;
	boot ();
	if (!stat_get (FLG_XMTE)) {
		// Transmitter should be disabled, i.e., receiver is active
		stat_set (FLG_XMTE);
		rcv_enable ();
	}
	if (stat_get (FLG_RCVA)) {
		ce_up;
		if (stat_get (FLG_RCVI))
			set_rcv_int;
	}
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

	    case PHYSOPT_SETPOWER:

		zzx_power &= ~0x3;
		if (val == NULL || *val >= (256/4)*3)
			zzx_power |= 3;	// Max
		else if (*val >= (256/4)*2)
			zzx_power |= 2;
		else if (*val >= (256/4)*1)
			zzx_power |= 1;
		reboot ();
		break;

	    case PHYSOPT_SETPAYLEN:

		if (val == NULL)
			zzv_paylen = 28;	// Default and max
		else if (*val > 0 && *val < 29)
			zzv_paylen = (byte) *val;
		else
			syserror (EREQPAR, "phys_rf24g paylen");
		reboot ();
		break;

	    case PHYSOPT_SETGROUP:

		if (val == NULL)
			zzv_group = 0;
		else
			zzv_group = (byte) *val;
		reboot ();
		break;

	    case PHYSOPT_SETCHANNEL:

		if (val == NULL)
			zzv_channel = 0;
		else
			zzv_channel = (byte) *val;
		reboot ();
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

		syserror (EREQPAR, "phys_rf24g option");

	}
	return ret;
}
