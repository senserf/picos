#ifndef	__rfmodule_cc__
#define	__rfmodule_cc__

#include "board.h"
#include "stdattr.h"
#include "rfmodule.h"
#include "tcvphys.h"
#include "tcv.cc"
#include "rfleds.h"

static int rfm_option (int, address);

byte Receiver::get_rssi (byte &qual) {

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

Xmitter::perform {

    state XM_LOOP:

	if (xbf == NULL) {
		if (txoff) {
			if (txoff == 3) {
Drain:
				tcvphy_erase (physid);
				when (txe, XM_LOOP);
				release;
			} else if (txoff == 1) {
				bkf = 0;
				when (txe, XM_LOOP);
				release;
			}
		}

		if ((xbf = tcvphy_get (physid, &buflen)) != NULL) {
			assert (buflen >= 4 && (buflen & 1) == 0,
				"Xmitter: illegal packet length %1d", buflen);
			if (sid != 0xffff)
				// otherwise, honor the packet's statid
				xbf [0] = sid;
		} else {
			// Nothing to transmit
			if (txoff == 2) {
				// Draining, stop if the queue is empty
				txoff = 3;
				goto Drain;
			}
			when (txe, XM_LOOP);
			release;
		}
	}

	if (bkf && !tcv_isurgent (xbf)) {
		// backing off and the packet is not urgent
		delay (bkf, XM_LOOP);
		bkf = 0;
		when (txe, XM_LOOP);
		release;
	}

	if (rcvg) {
		delay (minbkf, XM_LOOP);
		when (txe, XM_LOOP);
		release;
	}

	if (lbtdl && !rxoff) {
		// Start the ADC
		if (RSSI->signal ((void*)YES) == REJECTED)
			// Race with ADC
			proceed XM_LOOP;

		delay (lbtdl, XM_LBS);
		release;
	}

Xmit:
	set_congestion_indicator (0);
	obf.load (xbf, buflen);
	xmtg = YES;
	pwr_on ();
	LEDI (1, 1);
	rfi->transmit (&obf, XM_TXDONE);

    state XM_TXDONE:

	rfi->stop ();
	LEDI (1, 0);
	if (lbtdl == 0)
		// To reduce the risk of livelocks; not needed with LBT
		gbackoff ();

	tcvphy_end (xbf);
	xbf = NULL;
	xmtg = NO;
	pwr_off ();
	trigger (rxe);
	delay (minbkf, XM_LOOP);

    state XM_LBS:

	if (RSSI->signal ((void*)NO) == REJECTED)
		// Race with ADC
		proceed XM_LBS;

	if (rcvg) {
		set_congestion_indicator (minbkf);
		delay (minbkf, XM_LOOP);
		when (txe, XM_LOOP);
		release;
	}

	if (rxoff)
		goto Xmit;

	if (RSSI->sigLevel () < lbtth)
		goto Xmit;

#if ((RADIO_OPTIONS & 0x05) == 0x05)
	if (rerr [RERR_CONG] >= 0x0fff)
		diag ("RF driver: LBT congestion!!");
#endif
	gbackoff ();
	set_congestion_indicator (minbkf);
	proceed XM_LOOP;

}

ADC::perform {

    double DT, NA;

    state ADC_WAIT:

	this->wait (SIGNAL, ADC_RESUME);

    state ADC_RESUME:

	assert (ptrtoint (TheSignal) == YES, "ADC: illegal ON signal");
	
	ATime = 0.0;
	Average = 0.0;
	Last = Time;
	CLevel = rfi->sigLevel ();

	// In case something is already pending
	rfi->wait (ANYEVENT, ADC_UPDATE);
	this->wait (SIGNAL, ADC_STOP);

    state ADC_UPDATE:

	// Calculate the average signal level over the sampling
	// period

	DT = (double)(Time - Last);	// Time increment
	NA = ATime + DT;		// New total sampling time
	Average = ((Average * ATime) / NA) + (CLevel * DT) / NA;
	CLevel = rfi->sigLevel ();
	Last = Time;
	ATime = NA;
	// Only new events, no looping!
	rfi->wait (ANYEVENT, ADC_UPDATE);
	this->wait (SIGNAL, ADC_STOP);

    state ADC_STOP:

	Assert (ptrtoint (TheSignal) == NO, "ADC: illegal OFF signal");
	// Done: wait for another request
	proceed ADC_WAIT;
}

Receiver::perform {

    address packet;
    int     pktlen;
    byte    rssi, qual;

    state RCV_GETIT:

	rcvg = NO;
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

#if (RADIO_OPTIONS & 0x04)
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
		"Receiver: packet too short: %d < %d", pktlen,
			MINIMUM_PACKET_LENGTH);
	assert (pktlen <= RBS, "Receiver: packet too long: %d > %d",
		pktlen, RBS);

	if (sid != 0 && sid != 0xffff) {
		// Admit only packets with agreeable statid
		if (packet [0] != 0 && packet [0] != sid)
			// Ignore
			proceed RCV_GETIT;
	}

	memcpy (rbf, packet, pktlen);
		
	// Fake the RSSI for now. FIXME: do it right! Include add_entropy.
	rbf [(pktlen - 1) >> 1] = ((word) rssi << 8) | qual;

#if (RADIO_OPTIONS & 0x04)
	rerr [RERR_RCPS] ++;
#endif
#if (RADIO_OPTIONS & 0x02)
	diag ("RF driver: %u RX OK %04x %04x %04x", (word) seconds (),
		(word*)(rbf) [0],
		(word*)(rbf) [1],
		(word*)(rbf) [2]);
#endif
	tcvphy_rcv (physid, rbf, pktlen);
	proceed RCV_GETIT;
}

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

	phys_rfmodule_init (phy, mbs);
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

	phys_rfmodule_init (phy, mbs);
}

void PicOSNode::phys_rfmodule_init (int phy, int rbs) {

	rfm_intd_t *rf;

	rf = TheNode->RFInt;

	sid = 0;
	bkf = 0;

	physid = phy;

	/* Register the phy */
	txe = tcvphy_reg (phy, rfm_option, INFO_PHYS_DM2200);

	/* Both parts are initially active */
	rxoff = txoff = 1;
	pwrt_change (PWRT_RADIO, PWRT_RADIO_OFF);
	LEDI (0, 0);
	LEDI (1, 0);
	LEDI (2, 0);

	if (runthread (Xmitter) == 0 || runstrand (Receiver, rbs) == 0)
		syserror (ERESOURCE, "phys_rf");
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

		if (txoff == 0)
			ret = 2;
		if ((txoff & 1) == 0) {
			// On or draining
			if (xbf != NULL || tcvphy_top (physid) != NULL)
				ret |= 4;
		}
		if (rxoff == 0)
			ret++;
	
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		if (txoff) {
			txoff = NO;
			trigger (txe);
		}

		if (rxoff)
			LEDI (0, 1);
		else
			LEDI (0, 2);

		break;

	    case PHYSOPT_RXON:

		if (rxoff) {
			rxoff = NO;
			if (!xmtg) {
#if (RADIO_OPTIONS & 0x02)
			    diag ("RF driver: %u POWER DOWN",
				(word) seconds ());
#endif
			    TheNode->pwrt_change (PWRT_RADIO, PWRT_RADIO_RCV);
			}
			trigger (rxe);
		}

		if (txoff)
			LEDI (0, 1);
		else
			LEDI (0, 2);

		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		txoff = 2;
		trigger (txe);

		if (rxoff)
			LEDI (0, 0);
		else
			LEDI (0, 1);
		break;

	    case PHYSOPT_TXHOLD:

		txoff = 1;
		trigger (txe);

		if (rxoff)
			LEDI (0, 0);
		else
			LEDI (0, 1);
		break;

	    case PHYSOPT_RXOFF:

		if (!rxoff) {
			rxoff = 1;
			if (!xmtg)
			    TheNode->pwrt_change (PWRT_RADIO, PWRT_RADIO_OFF);
		}
		trigger (rxe);

		if (txoff)
			LEDI (0, 0);
		else
			LEDI (0, 1);
		break;

	    case PHYSOPT_CAV:

		/* Force an explicit backoff */
		if (val == NULL)
			bkf = 0;
		else
			bkf = *val;
		trigger (txe);
		break;

	    case PHYSOPT_SETPOWER:

		// Make sure the argument is decent
		if (val != NULL) {
			v = *val;
			if (v > (w = Ether->PS->upper ()))
				v = w;
			else if (v < (w = Ether->PS->lower ()))
				v = w;
		} else
			v = defxp;
		rf->setrfpowr (v);
		break;

	    case PHYSOPT_GETPOWER:

		ret = (word) (Ether->PS->getvalue (rfi->getXPower ()));
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

		ret = SEther->tagToRI (rfi->getTag ());
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

		ret = SEther->tagToCh (rfi->getTag ());
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

#if (RADIO_OPTIONS & 0x04)
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
#undef	txoff
#undef	rxoff
#undef	sid
#undef	minbkf
#undef	maxbkf
#undef	lbtth	
#undef	lbtdl
#undef	xmtg
#undef	rcvg
#undef	defxp
#undef	defrt
#undef	defch
#undef	rerr
#undef	retr

#include "stdattr_undef.h"

#endif
