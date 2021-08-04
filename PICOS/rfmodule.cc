/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#ifndef	__rfmodule_cc__
#define	__rfmodule_cc__

#include "board.h"

#define	rfi	(rf->RFInterface)
#define	mxpl	(rf->MaxPL)
#define	xbf	(rf->__pi_x_buffer)
#define	rbf	(rf->__pi_r_buffer)
#define	obf	(rf->OBuffer)
#define	bkf	(rf->backoff)
#define	txe	(rf->tx_event)
#define	lastp	(rf->LastPower)
#define	rxe	rf
#define	rxoff	(rf->RXOFF)
#define	sid	(rf->statid)
#define	minbkf	(rf->cpars->min_backoff)
#define	maxbkf	(rf->cpars->max_backoff)
#define	lbtth	(rf->cpars->lbt_threshold)
#define	lbtdl	(rf->cpars->lbt_delay)
#define	lbtries	(rf->cpars->lbt_tries)
#define	lbtnths (rf->cpars->lbt_nthrs)
#define	xmtg	(rf->Xmitting)
#define	rcvg	(rf->Receiving)
#define	defxp	(rf->cpars->DefXPower)
#define	defrt	(rf->cpars->DefRate)
#define	defch	(rf->cpars->DefChannel)
#define	physid	(rf->phys_id)
#define	rerr	(rf->rerror)

#define	upd_try_count	do { \
			    if (ntry < (lbtries ? lbtries : MAX_WORD)) ntry++; \
			} while (0)

#include "stdattr.h"
#include "tcvphys.h"
#include "tcv.cc"
#include "rfleds.h"

static int rfm_option (int, address);

// ============================================================================

int rfm_intd_t::revoke (revkfun_t qual) {
//
// Revokes a packet from the transmit queue as qualified by the provided
// function
//
	int nc = 0;

	if (__pi_x_buffer != NULL) {
		// Apply the function to the current packet
		if (qual (__pi_x_buffer)) {
			if (!Xmitting) {
				tcvphy_end (__pi_x_buffer);
				__pi_x_buffer = NULL;
				nc++;
			}
		}
	}

	// Scan the transmit queue
	return nc + tcvphy_erase (phys_id, qual);
}

void RM_Receiver::setup () {

	rf = TheNode->RFInt;
}

byte RM_Receiver::get_rssi (byte &qual) {

	word wr;

	// Let us ignore this for a while
	qual = 0;

	if (Ether->RSSIC == NULL)
		// No RSSI calculator present
		return 0;

	wr = Ether->RSSIC->getvalue (rfi->sigLevel (ThePckt, SIGL_OWN));

	if (wr > 255)
		wr = 255;

	return (byte) wr;
}

void RM_ADC::setup () {

	rf = TheNode->RFInt;
};

double RM_ADC::sigLevel () {

#ifdef LBT_THRESHOLD_IS_AVERAGE
	double DT, NA, res;

	DT = (double)(Time - Last);
	NA = ATime + DT;
	res = ((Average * ATime) / NA) + (CLevel * DT) / NA;
	return res;
#else
	return Maximum;
#endif
}

void RM_ADC::start () {

	if (!On) {
		On = YES;
		// This one can be legitimately rejected on a race: the signal
		// repo may be pending with the previous "off" signal. This is
		// exactly what the On flag is for.
		this->signal ((void*)YES);
	}
}

void RM_ADC::stop () {

	if (On) {
		On = NO;
		if (this->signal ((void*)NO) == REJECTED)
			excptn ("RM_ADC: rejected stop signal");
	} else
		excptn ("RM_ADC: unexpected stop signal");
}

RM_ADC::perform {

    state ADC_WAIT:

	if (!On) {
		this->wait (SIGNAL, ADC_WAIT);
		sleep;
	}

    transient ADC_RESUME:

#ifdef LBT_THRESHOLD_IS_AVERAGE
	ATime = 0.0;
	Average = 0.0;
	Last = Time;
	CLevel = rfi->sigLevel ();
#else
	Maximum = rfi->sigLevel ();
#endif
	// In case something is already pending
	rfi->wait (ANYEVENT, ADC_UPDATE);
	this->wait (SIGNAL, ADC_STOP);

    state ADC_UPDATE:

	// Calculate the average signal level over the sampling
	// period

#ifdef LBT_THRESHOLD_IS_AVERAGE
	double DT, NA;
	DT = (double)(Time - Last);	// Time increment
	NA = ATime + DT;		// New total sampling time
	CLevel = rfi->sigLevel ();
	Average = ((Average * ATime) / NA) + (CLevel * DT) / NA;
	Last = Time;
	ATime = NA;
#else
	double DT;
	if ((DT = rfi->sigLevel ()) > Maximum)
		Maximum = DT;
#endif
	// Only new events, no looping!
	rfi->wait (ANYEVENT, ADC_UPDATE);
	this->wait (SIGNAL, ADC_STOP);

    state ADC_STOP:

	Assert (ptrtoint (TheSignal) == NO, "ADC: illegal OFF signal");
	// Done: wait for another request
	sameas ADC_WAIT;
}

void RM_Xmitter::setup () {

	rf = TheNode->RFInt;
	xbf = NULL;
	RSSI = lbtdl ? create RM_ADC : NULL;
}

void RM_Xmitter::gbackoff () {
	// This is only called when backoff is defined, so maxbkf is > 0 and
	// has been set to the proper argument for toss
	bkf = minbkf + toss (maxbkf);
	// trace ("bkf = %1d", bkf);
}

void RM_Xmitter::pwr_on () {

	TheNode->pwrt_change (PWRT_RADIO, 
		rxoff ? PWRT_RADIO_XMT : PWRT_RADIO_XCV);
}

void RM_Xmitter::pwr_off () {

	TheNode->pwrt_change (PWRT_RADIO, 
		rxoff ? PWRT_RADIO_OFF : PWRT_RADIO_RCV);
}

// Copied almost directly from PICOS; will be optimized out if the body
// is empty
void RM_Xmitter::set_congestion_indicator (word v) {

#if (RADIO_OPTIONS & RADIO_OPTION_STATS)

	if ((rerr [RERR_CONG] = (rerr [RERR_CONG] * 3 + v) >> 2) > 0x0fff)
		rerr [RERR_CONG] = 0xfff;

	if (v) {
		if (rerr [RERR_CURB] + v < rerr [RERR_CURB])
			// Overflow
			rerr [RERR_CURB] = 0xffff;
		else
			rerr [RERR_CURB] += v;
	} else {
		// Update max
		if (rerr [RERR_MAXB] < rerr [RERR_CURB])
			rerr [RERR_MAXB] = rerr [RERR_CURB];
		rerr [RERR_CURB] = 0;
	}
#endif
}

RM_Xmitter::perform {

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
    address ppm;
    byte w, ppw;
#endif

    _pp_enter_ ();

    state XM_LOOP:

	double thr;

	if (xbf == NULL) {
		if ((xbf = tcvphy_get (physid, &buflen)) == NULL) {
			// Nothing to transmit
			when (txe, XM_LOOP);
			release;
		}
		// Reset the number of tries
		ntry = 0;
	}

	assert (buflen <= mxpl, "RM_Xmitter: packet too long, %1d > %1d",
		buflen, mxpl);
	assert (buflen >= MINIMUM_PACKET_LENGTH && (buflen & 1) == 0,
		"RM_Xmitter: illegal packet length %1d", buflen);
	if (sid != 0xffff)
		// otherwise, honor the packet's statid
		xbf [0] = sid;
		
	if (bkf && !tcv_isurgent (xbf)) {
		// backing off and the packet is not urgent
#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
Bkf:
#endif
		delay (bkf, XM_LOOP);
		bkf = 0;
		when (txe, XM_LOOP);
		release;
	}

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)

	// Check for CAV requested in the packet
	ppm = xbf + (buflen >> 1) - 1;

	if ((bkf = (*ppm & 0x0fff))) {
		*ppm &= ~0x0fff;
		goto Bkf;
	}

#if 0
	// This transmitter is not used any more with a neutrion-type channel
	if (__pi_channel_type != CTYPE_NEUTRINO) {
		// Xmit power setting?
		ppw = (*ppm >> 12) & 0x7;
		if (ppw != lastp) {
			if (ppw > (w = Ether->PS->upper ()))
				ppw = w;
			else if (ppw < (w = Ether->PS->lower ()))
				ppw = w;
			rf->setrfpowr (lastp = ppw);
		}
		// trace ("PXOPT pl: %1d", ppw);
	}
#endif

	if (*ppm & 0x8000)
		goto Force;	// What an ugly jump!
#endif

	// trace ("TR: %1d %1d %1d", lbtries, ntry, lbtnths);
	if (!lbtries || ntry < lbtries) {
Force:
		if (rcvg) {
#if 0
			// need this for safety?
			delay (minbkf, XM_LOOP);
			ntry++;
#endif
			when (txe, XM_LOOP);
			release;
		}

		if (lbtnths && !rxoff) {
			// LBT enabled and receiver turned on
			if (lbtdl) {
				// Have to listen for a while, use the RSSI
				// process
				// trace ("wsi %1d %1d", lbtdl, ntry);
				RSSI->start ();
				delay (lbtdl, XM_LBS);
				release;
			}
			// A single shot, this is the threshold level
			thr = lbtth [(ntry >= lbtnths) ? lbtnths - 1 : ntry];
			// trace ("SS: %g", linTodB (thr));
			if (rfi->sigLevel () > thr) {
				// Have to back off
				set_congestion_indicator (1);
				if (maxbkf) {
					// Normal backoff
					gbackoff ();
					upd_try_count;
					proceed XM_LOOP;
				}
				// Wait for signal low
				rfi->setSigThresholdLow (thr);
				rfi->wait (SIGLOW, XM_LOOP);
				release;
			}
		}
	}
Xmit:
	set_congestion_indicator (0);
	// Last check
	if (xbf == NULL)
		sameas XM_LOOP;
	xmtg = YES;
	obf.load (xbf, buflen);
	pwr_on ();
	LEDI (1, 1);
	rfi->transmit (&obf, XM_TXDONE);

#ifdef HIGHLIGHT_XMT
	// if (buflen > 14 && (((byte*)xbf) [2] & 0x1f) == 1 && 
		// xbf [5] == 0 && xbf [6] == 1)
	// PGTMP: DISP message with ref 1
	// highlight_set ((xbf [4] & 0x7f) == 0 ? 2 : 1, HIGHLIGHT_XMT, "XMT");
	highlight_set (1, HIGHLIGHT_XMT, "XMT");
#endif

    state XM_TXDONE:

	rfi->stop ();
	LEDI (1, 0);
	if (lbtnths == 0)
		// To reduce the risk of livelocks; not needed with LBT
		gbackoff ();

	tcvphy_end (xbf);
	xbf = NULL;
	xmtg = NO;
	pwr_off ();
	trigger (rxe);
	delay (minbkf, XM_LOOP);

    state XM_LBS:

	RSSI->stop ();

	if (rcvg) {
#if 0
		set_congestion_indicator (1);
		upd_try_count;
		delay (minbkf, XM_LOOP);
#endif
		// trace ("rvg");
		when (txe, XM_LOOP);
		release;
	}

	if (rxoff)
		goto Xmit;

	if (RSSI->sigLevel () < lbtth [ntry]) {
		// trace ("lbt+ %1d %g %g", ntry, RSSI->sigLevel (), lbtth [ntry]);
		goto Xmit;
	}
	// trace ("lbt- %1d %g %g", ntry, RSSI->sigLevel (), lbtth [ntry]);

#if ((RADIO_OPTIONS & (RADIO_OPTION_RBKF | RADIO_OPTION_STATS)) == (RADIO_OPTION_RBKF | RADIO_OPTION_STATS))
	if (rerr [RERR_CONG] >= 0x0fff)
		diag ("RF driver: LBT congestion!!");
#endif
	gbackoff ();
	upd_try_count;
	set_congestion_indicator (1);
	proceed XM_LOOP;

}

RM_Receiver::perform {

    address packet;
    int     pktlen;
    byte    rssi, qual;

    _pp_enter_ ();

    state RCV_GETIT:

	if (rcvg) {
		rcvg = NO;
		trigger (txe);
	}

	LEDI (2, 0);

	if (rxoff) {
		when (rxe, RCV_GETIT);
		release;
	}
	this->signal ((void*)NONE);
	rfi->wait (BOT, RCV_START);
	when (rxe, RCV_GETIT);

    state RCV_START:

	if (xmtg) {
		when (rxe, RCV_GETIT);
		release;
	}

	LEDI (2, 1);
	rcvg = YES;
	rfi->follow (ThePckt);
	skipto RCV_RECEIVE;

    state RCV_RECEIVE:

#if (RADIO_OPTIONS & RADIO_OPTION_STATS)
	if (rerr [RERR_RCPA] == MAX_WORD)
		// Zero out slots 0 and 1
		memset (rerr + RERR_RCPA, 0, sizeof (word) * 2);
	rerr [RERR_RCPA] ++;
#endif
	rfi->wait (EOT, RCV_GOTIT);
	rfi->wait (BERROR, RCV_GETIT);
	rfi->wait (BOT, RCV_START);

    state RCV_GOTIT:

	rssi = get_rssi (qual);

	packet = ThePckt -> Payload;
	pktlen = ThePckt -> PaySize;

	assert (pktlen >= MINIMUM_PACKET_LENGTH,
		"RM_Receiver: packet too short: %d < %d", pktlen,
			MINIMUM_PACKET_LENGTH);
	assert (pktlen <= mxpl, "RM_Receiver: packet too long: %d > %d",
		pktlen, mxpl);

	if (sid != 0 && sid != 0xffff) {
		// Admit only packets with agreeable statid
		if (packet [0] != 0 && packet [0] != sid)
			// Ignore
			proceed RCV_GETIT;
	}

	memcpy (rbf, packet, pktlen);
		
	// Fake the RSSI for now. FIXME: do it right! Include add_entropy.
	rbf [(pktlen - 1) >> 1] = ((word) rssi << 8) | qual;
	TheNode->_na_add_entropy (rssi);

#if (RADIO_OPTIONS & RADIO_OPTION_STATS)
	rerr [RERR_RCPS] ++;
#endif
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
	diag ("RF driver: %u RX OK %04x %04x %04x", (word) seconds (),
		rbf [0],
		rbf [1],
		rbf [2]);
#endif
	tcvphy_rcv (physid, rbf, pktlen);

	proceed RCV_GETIT;
}

// ============================================================================

void RN_Receiver::setup () {

	rf = TheNode->RFInt;
}

void RN_Xmitter::setup () {

	rf = TheNode->RFInt;
}

void RN_Xmitter::pwr_on () {

	TheNode->pwrt_change (PWRT_RADIO, 
		rxoff ? PWRT_RADIO_XMT : PWRT_RADIO_XCV);
}

void RN_Xmitter::pwr_off () {

	TheNode->pwrt_change (PWRT_RADIO, 
		rxoff ? PWRT_RADIO_OFF : PWRT_RADIO_RCV);
}

RN_Receiver::perform {

	address	packet;
	int	pktlen;

	_pp_enter_ ();

	state RCV_GETIT:

		LEDI (2, 0);

		if (rxoff) {
			when (rxe, RCV_GETIT);
			release;
		}

		rfi->wait (EOT, RCV_GO);
		when (rxe, RCV_GETIT);

	state RCV_GO:

		LEDI (2, 1);

		while (rfi->anotherActivity () != NONE) {
			if (ThePckt->Sender == TheNode->getId ())
				continue;
			packet = ThePckt -> Payload;
			pktlen = ThePckt -> PaySize;

			assert (pktlen >= MINIMUM_PACKET_LENGTH,
				"RN_Receiver: packet too short: %d < %d",
				pktlen, MINIMUM_PACKET_LENGTH);

			assert (pktlen <= mxpl,
				"RN_Receiver: packet too long: %d > %d",
				pktlen, mxpl);

			if (sid != 0 && sid != 0xffff) {
				// Admit only packets with agreeable statid
				if (packet [0] != 0 && packet [0] != sid)
					// Ignore
					continue;
			}

			memcpy (rbf, packet, pktlen);
			// Dummy RSSI/qual
			rbf [(pktlen - 1) >> 1] = 0xFF00;
			tcvphy_rcv (physid, rbf, pktlen);
		}

		skipto RCV_GETIT;
}

RN_Xmitter::perform {

	int buflen;

	_pp_enter_ ();

	state XM_LOOP:

		if ((xbf = tcvphy_get (physid, &buflen)) == NULL) {
			when (txe, XM_LOOP);
			release;
		}

		assert (buflen <= mxpl,
			"RN_Xmitter: packet too long, %1d > %1d",
			buflen, mxpl);

		assert (buflen >= MINIMUM_PACKET_LENGTH && (buflen & 1) == 0,
			"RN_Xmitter: illegal packet length %1d", buflen);

		if (sid != 0xffff)
			// otherwise, honor the packet's statid
			xbf [0] = sid;

		obf.load (xbf, buflen);
		pwr_on ();
		LEDI (1, 1);
		xmtg = YES;
		rfi->transmit (&obf, XM_TXDONE);

	state XM_TXDONE:

		rfi->stop ();
		LEDI (1, 0);
		tcvphy_end (xbf);
		xbf = NULL;
		xmtg = NO;
		pwr_off ();
		proceed XM_LOOP;
}

// ============================================================================

__PUBLF (PicOSNode, void, phys_cc1100) (int phy, int mbs) {
/*
 * phy  - interface number
 * mbs  - maximum packet length (including checksum, must be divisible by 4)
 */
	rfm_intd_t *rf;

	rf = TheNode->RFInt;

	if (mbs < 6 || mbs > CC1100_MAXPLEN) {
		if (mbs == 0)
			mbs = CC1100_MAXPLEN;
		else
			syserror (EREQPAR, "phys_cc1100");
	}

	rbf = (address) memAlloc (mbs, (word) (mbs + 2));
	if (rbf == NULL)
		syserror (EMALLOC, "phys_cc1100");

	phys_rfmodule_init (phy, mbs, INFO_PHYS_CC1100);
}

__PUBLF (PicOSNode, void, phys_cc1350) (int phy, int mbs) {
/*
 * phy  - interface number
 * mbs  - maximum packet length (including checksum, must be divisible by 4)
 */
	rfm_intd_t *rf;

	rf = TheNode->RFInt;

	if (mbs < 6 || mbs > CC1350_MAXPLEN) {
		if (mbs == 0)
			mbs = CC1350_MAXPLEN;
		else
			syserror (EREQPAR, "phys_cc1350");
	}

	rbf = (address) memAlloc (mbs, (word) (mbs + 2));
	if (rbf == NULL)
		syserror (EMALLOC, "phys_cc1350");

	phys_rfmodule_init (phy, mbs, INFO_PHYS_CC1350);
}

__PUBLF (PicOSNode, void, phys_cc2420) (int phy, int mbs) {
/*
 * phy  - interface number
 * mbs  - maximum packet length (including checksum, must be divisible by 4)
 */
	rfm_intd_t *rf;

	rf = TheNode->RFInt;

	if (mbs < 6 || mbs > CC2420_MAXPLEN) {
		if (mbs == 0)
			mbs = CC2420_MAXPLEN;
		else
			syserror (EREQPAR, "phys_cc2420");
	}

	rbf = (address) memAlloc (mbs, (word) (mbs + 2));
	if (rbf == NULL)
		syserror (EMALLOC, "phys_cc2420");

	phys_rfmodule_init (phy, mbs, INFO_PHYS_CC2420);
}

__PUBLF (PicOSNode, void, phys_dm2200) (int phy, int mbs) {
/*
 * phy  - interface number
 * mbs  - maximum packet length (including checksum, must be divisible by 4)
 */
	rfm_intd_t *rf;

	rf = TheNode->RFInt;

	if (mbs < 8 || mbs & 0x3) {
		if (mbs == 0)
			mbs = RADIO_DEF_BUF_LEN;
		else
			syserror (EREQPAR, "phys_dm2200 mbs");
	}

	rbf = (address) memAlloc (mbs, (word) mbs);
	if (rbf == NULL)
		syserror (EMALLOC, "phys_dm2200");

	phys_rfmodule_init (phy, mbs, INFO_PHYS_DM2200);
}

void PicOSNode::phys_rfmodule_init (int phy, int rbs, int inf) {

	rfm_intd_t *rf;
	Boolean su;

	rf = TheNode->RFInt;

	sid = 0;
	bkf = 0;

	physid = phy;

	/* Register the phy */
	txe = tcvphy_reg (phy, rfm_option, inf);
	mxpl = rbs;

	/* Both parts are initially active */
	rxoff = YES;
	pwrt_change (PWRT_RADIO, PWRT_RADIO_OFF);
	LEDI (0, 0);
	LEDI (1, 0);
	LEDI (2, 0);

	if (__pi_channel_type == CTYPE_NEUTRINO)
		su = runthread (RN_Xmitter) != 0 &&
			runthread (RN_Receiver) != 0;
	else
		su = runthread (RM_Xmitter) != 0 &&
			runthread (RM_Receiver) != 0;
	if (!su)
		syserror (ERESOURCE, "phys_rf");

	TheNode->_na_add_entropy ((lword)(lRndUniform ((LONG)0, MAX_LONG) ^
		getEffectiveTimeOfDay ()));

#if 0
	// Dump the Transceiver status
	{
		Transceiver *t;
		t = rf->RFInterface;
		diag ("TRANSCEIVER %1d, %1d, %g, %g, %1d, %1d, %g, %1d",
			(Long)(t->getTXate ()),
			t->getPreamble (),
			t->getXPower (),
			t->getRPower (),
			(Long)(t->getTag ()),
			t->getErrorRun (),
			t->getMinDistance (),
			t->getAevMode ());
	}
#endif
}

static int rfm_option (int opt, address val) {
/*
 * Option processing
 */
	int ret = 0;
	word w, v;
	rfm_intd_t *rf;

	rf = TheNode->RFInt;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = (2 | !rxoff);
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_RXON:

		if (rxoff) {
			if (!xmtg) {
#if (RADIO_OPTIONS & RADIO_OPTION_DIAG)
			    diag ("RF driver: %u POWER UP",
				(word) seconds ());
#endif
			    TheNode->pwrt_change (PWRT_RADIO, PWRT_RADIO_RCV);
			    trigger (rxe);
			}

			LEDI (0, 1);
			rxoff = NO;
		}

	    case PHYSOPT_TXON:
	    case PHYSOPT_TXOFF:
	    case PHYSOPT_TXHOLD:

		break;

	    case PHYSOPT_RXOFF:

		if (!rxoff) {
			rxoff = YES;
			if (!xmtg) {
			    trigger (rxe);
			    TheNode->pwrt_change (PWRT_RADIO, PWRT_RADIO_OFF);
			}
			LEDI (0, 0);
		}

		break;

	    case PHYSOPT_CAV:

		/* Force an explicit backoff */
		if (val == NULL)
			bkf = 0;
		else
			bkf = *val;
		trigger (txe);
		break;

#if ((RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS) == 0)

	    case PHYSOPT_SETPOWER:

		if (__pi_channel_type == CTYPE_NEUTRINO)
			// Ignore
			break;

		// Make sure the argument is decent
		if (val != NULL) {
			v = *val;
			if (v > (w = Ether->PS->upper ()))
				v = w;
			else if (v < (w = Ether->PS->lower ()))
				v = w;
		} else
			v = defxp;

		if (v != lastp)
			rf->setrfpowr (lastp = v);
		break;
#endif
	    case PHYSOPT_GETPOWER:

		ret = (__pi_channel_type == CTYPE_NEUTRINO) ? 0 :
			(word) (Ether->PS->getvalue (rfi->getXPower ()));

		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_SETRATE:

		if (val != NULL) {
			v = *val;
			if (v > (w = Ether->Rates->upper ()))
				v = w;
			else if (v < (w = Ether->Rates->lower ()))
				v = w;
		} else
			v = defrt;
		rf->setrfrate (v);
		break;

	    case PHYSOPT_GETRATE:

		ret = Ether->tagToRI (rfi->getTag ());
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_SETCHANNEL:

		if (val != NULL) {
			v = *val;
			if (v > (w = Ether->Channels->max ()))
				v = w;
		} else
			v = defch;

		rf->setrfchan (v);
		break;

	    case PHYSOPT_GETCHANNEL:

		ret = Ether->tagToCh (rfi->getTag ());
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_SETSID:

		sid = (val == NULL) ? 0 : *val;
		break;

            case PHYSOPT_GETSID:

		ret = (int) sid;
		if (val != NULL)
			*val = ret;
		break;

#if (RADIO_OPTIONS & RADIO_OPTION_STATS)
	    case PHYSOPT_ERROR:

		if (val != NULL) {
			if (rerr [RERR_CURB] > rerr [3])
				rerr [RERR_MAXB] = rerr [RERR_CURB];
			memcpy (val, rerr, RERR_SIZE);
		} else
			memset (rerr, 0, sizeof (rerr));

		break;
#endif
	    case PHYSOPT_RESET:
		// Void
		return 0;

	    case PHYSOPT_REVOKE:

		return rf->revoke ((revkfun_t)val);

	    default:

		excptn ("SYSERROR [%1d]: %1d, unimplemented PHYSOPT %1d",
			TheStation->getId (), EREQPAR, opt);

	}
	return ret;
}

#undef	rfi
#undef	xbf
#undef	rbf
#undef	obf
#undef	bkf
#undef	txe
#undef	rxe
#undef	rxoff
#undef	sid
#undef	minbkf
#undef	maxbkf
#undef	lbtth	
#undef	lbtdl
#undef	lbtries
#undef	xmtg
#undef	rcvg
#undef	defxp
#undef	defrt
#undef	defch
#undef	rerr
#undef	retr
#undef	lastp
#undef	upd_try_count

#include "stdattr_undef.h"

#endif
