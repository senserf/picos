/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "tcvphys.h"
#include "phys_ether.h"
#include "ethernet.h"

static int option_raw (int, address);
static int option_cooked (int, address);

typedef	struct {
	int	physid;
	word	qevent;
	byte	txoff, rxoff;
} istat_t;

typedef	struct {
	istat_t	istatus [2];	/* 0 - raw, 1 - cooked */
	int	buflen;
	address buffer;
	byte	summary;
	byte	lastwmode;
} idata_t;

static idata_t	*idata = NULL;

static thread (reader)

    int ln;
    istat_t *s;

    entry (0)

	if ((ln = io (0, ETHERNET, READ, (char*)(idata->buffer), idata->buflen))
  	    > 0) {
		s = &(idata->istatus [idata->summary < 2 ?
			idata->summary : ion (ETHERNET, CONTROL, NULL,
				ETHERNET_CNTRL_GMODE)]);
		/*
		 * Keep receiving even if nobody's listening to reclaim
		 * buffer spce
		 */
		if (s->rxoff == 0)
			tcvphy_rcv (s->physid, idata->buffer, ln);
	}

	proceed (0);

endthread

static void write (int mode) {

	istat_t *s;
	static address buffer = NULL;
	int len;

	s = &(idata->istatus [mode]);

	while (1) {
		if (buffer == NULL)
			buffer = tcvphy_get (s->physid, &len);
		if (s->txoff) {
			switch (s->txoff) {
				case 1:
					/* Off, queue held */
					wait (s->qevent, 0);
					release;
				case 2:
					/* Draining */
					if (buffer != NULL)
						break;
					/* Drained */
					s->txoff = 3;
				default:
					/* Drain */
					if (buffer != NULL) {
						tcvphy_end (buffer);
						buffer = NULL;
					}
					tcvphy_erase (s->physid);
					wait (s->qevent, 0);
					release;
			}
		} else if (buffer == NULL) {
			wait (s->qevent, 0);
			release;
		}

		if (idata->lastwmode != mode) {
			idata->lastwmode = mode;
			ion (ETHERNET, CONTROL, (char*)&mode,
				ETHERNET_CNTRL_WMODE);
		}
		io (0, ETHERNET, WRITE, (char*) buffer, len);
		tcvphy_end (buffer);
		buffer = NULL;
	}
}

static thread (writer)

    entry (0)

	if ((idata->summary & 1) == 0)
		write (0);
	if (idata->summary > 0)
		write (1);

	release;

endthread

void phys_ether (int phy, int cid, int mbs) {
/*
 * Registers the Ethernet interface as a phy. If cid (card Id) is nonzero,
 * the interface is "cooked"; otherwise, the interface is raw and we are
 * sending and receiving Ethernet frames. mbs is the maximum buffer size
 * for a received packet. If zero, the maximum Ethernet frame is assumed.
 * It is possible to have two different interfaces (raw and cooked)
 * being present simultaneously under different phy numbers.
 */
	int np;
	istat_t *s;
	char c;

	if (idata == NULL) {
		if ((idata = (idata_t*) umalloc (sizeof (idata_t))) == NULL)
			syserror (EMALLOC, "phys_ether (1)");
		for (np = 0; np < 2; np++) {
			s = &idata->istatus [np];
			s->physid = -1;
			s->txoff = s->rxoff = 0;
		}
		idata->buflen = 0;
		idata->summary = (byte) -1;
		/* First time around - reset the interface to a decent state */
		c = 0;
		ion (ETHERNET, CONTROL, &c, ETHERNET_CNTRL_PROMISC);
		ion (ETHERNET, CONTROL, &c, ETHERNET_CNTRL_MULTI);
	}

	/* 0 == RAW, 1 == COOKED */
	s = &(idata->istatus [(cid != 0)]);

	if (s->physid != NONE)
		/* We have done it already */
		syserror (ETOOMANY, "phys_ether");

	s->physid = phy;

	/* 0 == RAW, 1 == COOKED, 2 == BOTH */
	idata->summary += (1 + (cid != 0));

	np = cid ? ETH_MXLEN - 14 - 4 : ETH_MXLEN;

	if (mbs == 0)
		mbs = np;
	else if (mbs < 0 || mbs > np)
		syserror (EREQPAR, "phys_ether");

	if (cid)
		ion (ETHERNET, CONTROL, (char*)&cid, ETHERNET_CNTRL_SETID);

	/* Check if we should re-allocate the buffer */
	if (idata->buflen < mbs) {
		if (idata->buflen)
			ufree (idata->buffer);
		idata->buflen = mbs;
		idata->buffer = (address) umalloc (mbs < 14 ? 14 : mbs);
		if (idata->buffer == NULL)
			syserror (EMALLOC, "phys_ether (2)");
	}

	/* Register the phy */
	s->qevent = tcvphy_reg (phy, cid ? option_cooked : option_raw,
		INFO_PHYS_ETHER | ((cid != 0) << 4));

	/* Start the driver processes, unless they are present already */
	if (idata->summary < 2) {
		runthread (reader);
		runthread (writer);
	}

	ion (ETHERNET, CONTROL, (char*)&(idata->summary), ETHERNET_CNTRL_RMODE);

	idata->lastwmode = (cid != 0);
	/* This will be reset dynamically if summary == 2 */
	ion (ETHERNET, CONTROL, (char*)&(idata->lastwmode),
		ETHERNET_CNTRL_WMODE);
}

static int option (istat_t *s, int opt, address val) {

	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = ((s->txoff == 0) << 1) | (s->rxoff == 0);
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		s->txoff = 0;
		trigger (s->qevent);
		break;

	    case PHYSOPT_RXON:

		s->rxoff = 0;
		trigger (s->qevent);
		break;

	    case PHYSOPT_TXOFF:

		/* This means switch off when the output queue drains */
		s->txoff = 2;
		trigger (s->qevent);
		break;

	    case PHYSOPT_TXHOLD:

		s->txoff = 1;
		trigger (s->qevent);
		break;

	    case PHYSOPT_RXOFF:

		s->rxoff = 0;
		trigger (s->qevent);
		break;

	    case PHYSOPT_ERROR:

		/* This just returns the error status */
		ion (ETHERNET, CONTROL, (char*)&ret, ETHERNET_CNTRL_ERROR);
		break;
	}

	return ret;
}

static int option_raw (int opt, address val) {
/*
 * Option processing
 */
	return option (&idata->istatus [0], opt, val);
}

static int option_cooked (int opt, address val) {
/*
 * Option processing
 */
	return option (&idata->istatus [1], opt, val);
}
