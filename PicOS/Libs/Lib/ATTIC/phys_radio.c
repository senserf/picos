/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "tcvphys.h"
#include "phys_radio.h"

/*
 * MAC header format (for cooked frames):
 *
 *     ----------------------
 *     | 16 bits |  16 bits |
 *     ----------------------
 *         RCV     ACK/SEQ?
 *                   LEN?
 */

#if 	RADIO_INTERRUPTS == 0
#error	"THIS VERSION OF phys_radio REQUIRES RADIO_INTERRUPTS > 0"
#endif

typedef	struct {

	byte 	rxoff, txoff;
	word	qevent, physid, statid;
	address packet;
	int	tlen;

	/* ================== */
	/* Control parameters */
	/* ================== */
	word 	delmnbkf,
		delbsbkf,
		delxmsen,
		backoff;

	int	buflen;
	/* This will be dynamic */
	word	buffer [1];

} radioblock_t;

static int option (int, address);

static	radioblock_t *radio = NULL;

#define	rxevent	((word)&(radio->buffer [0]))
#define	txevent	((word)&(radio->buffer [1]))

static word gbackoff () {
/* ============================================ */
/* Generate a backoff after sensing an activity */
/* ============================================ */
	return radio->delmnbkf + (rnd () & radio->delbsbkf);
}

static	int rx () {

	int len;
	word dst;

	if ((len = io (NONE, RADIO, READ, (char*)(radio->buffer),
	    radio->buflen)) == 0) {
		return 0;
	}

	/* We seem to have a packet */
	if (radio->statid == 0 ||
	    (dst = *((int*)(radio->buffer))) == 0 ||
		dst == radio->statid) {
		    /* Receive */
		    tcvphy_rcv (radio->physid, radio->buffer, len);
		    return 1;
	}

	return 0;
}

#define	tx(p,l)	io (NONE, RADIO, WRITE, (char*)(p), l)

#define	XM_LOOP	0
#define XM_URGN	1

static process (xmtradio, void)

    char c;

    entry (XM_LOOP)

	if (radio->txoff) {
		/* We are off */
		if (radio->txoff == 3) {
			/* Drain */
			tcvphy_erase (radio->physid);
			wait (radio->qevent, XM_LOOP);
			release;
		} else if (radio->txoff == 1) {
			/* Queue held, transmitter off */
			radio->backoff = 0;
			finish;
		}
	}

	if (radio->backoff) {
		if (tcvphy_top (radio->physid) > 1) {
			/* Urgent packet already pending, transmit it */
			radio->backoff = 0;
		} else  {
			delay (radio->backoff, XM_LOOP);
			radio->backoff = 0;
			/* Transmit an urgent packet when it shows up */
			wait (radio->qevent, XM_URGN);
			release;
		}
	}

    ForceXmt:

	if (radio->packet ||
	  (radio->packet = tcvphy_get (radio->physid, &(radio->tlen))) !=
	    NULL) {
		if (rcvlast () <= radio->delxmsen) {
			/* Activity, we backoff even if the packet was urgent */
			delay (gbackoff (), XM_LOOP);
			release;
		}
		/* Transmit */
		c = 1;
		io (NONE, RADIO, CONTROL, &c, RADIO_CNTRL_XMTCTRL);
		tx (radio->packet, radio->tlen);
		c = 0;
		io (NONE, RADIO, CONTROL, &c, RADIO_CNTRL_XMTCTRL);
		tcvphy_end (radio->packet);
		radio->packet = NULL;
		delay (RADIO_POST_SPACE, XM_LOOP);
		release;
	}

	if (radio->txoff == 2) {
		/* Draining; stop xmt if the output queue is empty */
		radio->txoff = 3;
		proceed (0);
	}

	wait (radio->qevent, 0);
	wait (txevent, 0);
	release;

    entry (XM_URGN)

	/* Urgent packet while obeying CAV */
	if (tcvphy_top (radio->physid) > 1)
		goto ForceXmt;
	wait (radio->qevent, XM_URGN);
	snooze (XM_LOOP);
	release;

    nodata;

endprocess (1)

static process (rcvradio, void)

    entry (0)

	if (rx ()) {
		/* For the transmitter */
		radio->backoff = gbackoff ();
		trigger (txevent);
		if (radio->rxoff)
			finish;
		proceed (1);
	}
	if (radio->rxoff)
		finish;

	wait (rxevent, 1);
	rcvwait (0);
	release;

    entry (1)

	rcvcancel ();
	proceed (0);

    nodata;

endprocess (1)

void phys_radio (int phy, int mod, int mbs) {
/*
 * phy  - interface number
 * mod  - if nonzero, selects framed mode with 'mod' used as station Id
 * mbs  - maximum packet length (excluding checksum, must be divisible by 4)
 */
	char c;

	if (radio != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_radio");

	if (mbs < 8 || mbs & 0x3) {
		if (mbs == 0)
			mbs = RADIO_DEF_BUF_LEN;
		else
			syserror (EREQPAR, "phys_radio mbs");
	}

	if ((radio = (radioblock_t*) umalloc (sizeof (radioblock_t) +
		mbs - 2)) == NULL)
			syserror (EMALLOC, "phys_radio");

	radio -> buflen = mbs;	// This is total length
	radio -> statid = mod;
	radio -> physid = phy;
	radio -> backoff = 0;
	radio -> packet = NULL;

	radio->delmnbkf = MIN_BACKOFF;
	radio->delbsbkf = MSK_BACKOFF;
	radio->delxmsen = RADIO_DEF_TCVSENSE;

	/* Register the phy */
	radio->qevent = tcvphy_reg (phy, option, INFO_PHYS_RADIO | (mod != 0));

	/* Start the driver processes */
	radio->rxoff = radio->txoff = 0;

	/* Make sure physical receiver is on */
	c = 1;
	io (NONE, RADIO, CONTROL, &c, RADIO_CNTRL_RCVCTRL);

	fork (xmtradio, NULL);
	fork (rcvradio, NULL);
}

static int option (int opt, address val) {
/*
 * Option processing
 */
	int ret = 0;
	char c;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = ((radio->txoff == 0) << 1) | (radio->rxoff == 0);
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		radio->txoff = 0;
		if (!running (xmtradio))
			fork (xmtradio, NULL);
		trigger (txevent);
		break;

	    case PHYSOPT_RXON:

		radio->rxoff = 0;
		c = 1;
		io (NONE, RADIO, CONTROL, &c, RADIO_CNTRL_RCVCTRL);
		if (!running (rcvradio))
			fork (rcvradio, NULL);
		trigger (rxevent);
		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		radio->txoff = 2;
TxOff:
		trigger (txevent);
		trigger (radio->qevent);
		break;

	    case PHYSOPT_TXHOLD:

		radio->txoff = 1;
		goto TxOff;

	    case PHYSOPT_RXOFF:

		radio->rxoff = 1;
		c = 0;
		io (NONE, RADIO, CONTROL, &c, RADIO_CNTRL_RCVCTRL);
		trigger (rxevent);
		break;

	    case PHYSOPT_CAV:

		/* Force an explicit backoff */
		if (val == NULL)
			radio->backoff = 0;
		else
			radio->backoff = *val;
		trigger (txevent);
		break;

	    case PHYSOPT_SENSE:

		ret = rcvlast ();
		break;

	    case PHYSOPT_SETPARAM:

#define	pinx	(*val)
		/*
		 * This is the parameter index. The parameters are numbered:
		 *
		 *    0 - minimum backoff (min = 0 msec)
		 *    1 - backoff mask bits (from 1 to 15)
		 *    2 - sense time before xmit (min = 0, max = 1024 msec)
		 *    3 - preamble length (min = 2, max = 128)
		 *    4 - wait preamble high (min = 1, max = 64 msec)
		 *    5 - preamble resync tries (min = 1, max = 8)
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
				if (pval < radio->delxmsen)
					pval = radio->delxmsen;
				radio->delmnbkf = pval;
				break;
			case 1:
				if (pval > 15)
					pval = 15;
				if (pval)
					pval = (1 << pval) - 1;
				radio->delbsbkf = pval;
				break;
			case 2:
				if (pval > 1024)
					pval = 1024;
				if ((radio->delxmsen = pval) > radio->delmnbkf)
					radio->delmnbkf = pval;
				break;
			case 3:
				if (pval < 2)
					pval = 2;
				else if (pval > 128)
					pval = 128;
				io (NONE, RADIO, CONTROL, (char*) pval,
					RADIO_CNTRL_SETPRLEN);
				break;
			case 4:
				if (pval < 1)
					pval = 1;
				else if (pval > 64)
					pval = 64;
				io (NONE, RADIO, CONTROL, (char*) pval,
					RADIO_CNTRL_SETPRWAIT);
				break;
			case 5:
				if (pval < 1)
					pval = 1;
				else if (pval > 8)
					pval = 8;
				io (NONE, RADIO, CONTROL, (char*) pval,
					RADIO_CNTRL_SETPRTRIES);
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
