#ifndef	__rfmodule_dm2200_cc__
#define	__rfmodule_dm2200_cc__

#include "board.h"
#include "rfmodule_dm2200.h"
#include "tcvphys.h"

#include "tcv.cc"

#define	TX_EVENT		((int)(TheNode->tx_event))
#define	RX_EVENT		((int)(TheNode->RFInterface))

static int option (int, address);

byte Receiver::get_rssi (byte &qual) {

	IHist *ih;
	double sl, sd, bn, ra;
	byte res;

	ih = RFC->iHist (ThePckt);
	assert (ih != NULL, "Receiver->get_rssi: no IHist");

	sl = RFC->sigLevel (ThePckt, SIGL_OWN);
	assert (sl >= 0.0, "Receiver->get_rssi: no signal");

	// Weight it between background noise and 10dBm, which is the default
	// transmit power
	sd = linTodB (sl);
	bn = linTodB (Ether->BNoise);

	// This should be precomputed 
	ra = 10.0 - bn;

	ra = ((sd - bn) / ra) * 127.0;
	if (ra < 0.0)
		res = 0;
	else if (ra > 127.0)
		res = 127;
	else
		res = (byte) ra;

	// By quality, we mean the S/N ratio normalized between 10.0/bn and
	// 0dB

	ra = RFC->sigLevel (ThePckt, SIGL_IFA) + Ether->BNoise;
	sl = (linTodB (sl/ra) + 5.0) / 55.0;
	if (sl >= 1.0)
		qual = 127;
	else if (sl < 0.0)
		qual = 0;
	else
		qual = (byte) (128.0 * sl);

	return res;
}

Xmitter::perform {

    state XM_LOOP:

	if (buffer == NULL) {
		if (S->TXOFF) {
			if (S->TXOFF == 3) {
Drain:
				S->tcvphy_erase (PHYSID);
				S->wait (TX_EVENT, XM_LOOP);
				sleep;
			} else if (S->TXOFF == 1) {
				S->backoff = 0;
				S->wait (TX_EVENT, XM_LOOP);
				sleep;
			}
		}

		if ((buffer = S->tcvphy_get (PHYSID, &buflen)) != NULL) {
			assert (buflen >= 4 && (buflen & 1) == 0,
				"Xmitter: illegal packet length");
			buffer [0] = S->statid;
		} else {
			// Nothing to transmit
			if (S->TXOFF == 2) {
				// Draining, stop if the queue is empty
				S->TXOFF = 3;
				goto Drain;
			}
			S->wait (TX_EVENT, XM_LOOP);
			sleep;
		}
	}

	if (S->backoff && !S->tcv_isurgent (buffer)) {
		// backing off and the packet is not urgent
		S->delay (S->backoff, XM_LOOP);
		S->backoff = 0;
		S->wait (TX_EVENT, XM_LOOP);
		sleep;
	}

	if (S->Receiving) {
		S->delay (S->min_backoff, XM_LOOP);
		S->wait (TX_EVENT, XM_LOOP);
		sleep;
	}

	if (S->lbt_delay && !S->RXOFF) {
		// Start the ADC
		RSSI->signal ((void*)YES);
		S->delay (S->lbt_delay, XM_LBS);
		sleep;
	}

Xmit:
	S->OBuffer.load (buffer, buflen);
	S->Xmitting = YES;
	RFC->transmit (&(S->OBuffer), XM_TXDONE);

    state XM_TXDONE:

	RFC->stop ();
	if (S->lbt_delay == 0)
		// To reduce the risk of livelocks; not needed with LBT
		S->gbackoff ();

	S->tcvphy_end (buffer);
	buffer = NULL;
	S->Xmitting = NO;
	trigger (RX_EVENT);
	S->delay (S->min_backoff, XM_LOOP);

    state XM_LBS:

	RSSI->signal ((void*)NO);
	if (S->Receiving) {
		S->delay (S->min_backoff, XM_LOOP);
		S->wait (TX_EVENT, XM_LOOP);
		sleep;
	}

	if (S->RXOFF)
		goto Xmit;

	if (RSSI->sigLevel () < S->lbt_threshold)
		goto Xmit;

	S->gbackoff ();
	proceed XM_LOOP;

}

ADC::perform {

    double DT, NA;

    state ADC_WAIT:

	this->wait (SIGNAL, ADC_RESUME);

    state ADC_RESUME:

	assert ((int)TheSignal == YES, "ADC: illegal ON signal");
	
	ATime = 0.0;
	Average = 0.0;
	Last = Time;
	CLevel = RFC->sigLevel ();

	// In case something is already pending
	RFC->wait (ANYEVENT, ADC_UPDATE);
	this->wait (SIGNAL, ADC_STOP);

    state ADC_UPDATE:

	// Calculate the average signal level over the sampling
	// period

	DT = (double)(Time - Last);	// Time increment
	NA = ATime + DT;		// New total sampling time
	Average = ((Average * ATime) / NA) + (CLevel * DT) / NA;
	CLevel = RFC->sigLevel ();
	Last = Time;
	ATime = NA;
	// Only new events, no looping!
	RFC->wait (ANYEVENT, ADC_UPDATE);
	this->wait (SIGNAL, ADC_STOP);

    state ADC_STOP:

	Assert ((int)TheSignal == NO, "ADC: illegal OFF signal");
	// Done: wait for another request
	proceed ADC_WAIT;
}

Receiver::perform {

    address packet;
    int     pktlen;
    byte    rssi, qual;

    state RCV_GETIT:

	S->Receiving = NO;

	if (S->RXOFF) {
Finidh:
		S->wait (RX_EVENT, RCV_GETIT);
		sleep;
	}
	this->signal ((void*)NONE);
	RFC->wait (BOT, RCV_START);

    state RCV_START:

	if (S->Xmitting) {
		S->wait (RX_EVENT, RCV_GETIT);
		sleep;
	}

	S->Receiving = YES;
	RFC->follow (ThePckt);
	skipto RCV_RECEIVE;

    state RCV_RECEIVE:

	RFC->wait (EOT, RCV_GOTIT);
	RFC->wait (BERROR, RCV_GETIT);
	RFC->wait (BOT, RCV_START);

    state RCV_GOTIT:

	rssi = get_rssi (qual);

	packet = ThePckt -> Payload;
	pktlen = ThePckt -> PaySize;

	assert (pktlen > MINIMUM_PACKET_LENGTH, "Receiver: packet too short");
	if (S->statid != 0 && packet [0] != 0 && packet [0] != S->statid)
		// Ignore
		proceed RCV_GETIT;

	// Fake the RSSI for now. FIXME: do it right! Include add_entropy.
	packet [(pktlen - 1) >> 1] = ((word) rssi << 8) | qual;

	S->tcvphy_rcv (PHYSID, packet, pktlen);
	proceed RCV_GETIT;
}

void PicOSNode::phys_dm2200 (int phy, int mbs) {
/*
 * phy  - interface number
 * mbs  - maximum packet length (including checksum, must be divisible by 4)
 */
	if (mbs < 8 || mbs & 0x3) {
		if (mbs == 0)
			mbs = RADIO_DEF_BUF_LEN;
		else
			syserror (EREQPAR, "phys_dm2200 mbs");
	}

	if (memBook (mbs) == NO)
		syserror (EMALLOC, "phys_dm2200");

	statid = 0;
	backoff = 0;

	/* Register the phy */
	tx_event = tcvphy_reg (phy, option, INFO_PHYS_DM2200);

	/* Both parts are initially active */
	RXOFF = TXOFF = 1;

	create Xmitter;
	create Receiver;
}

static int option (int opt, address val) {
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = ((TheNode->TXOFF == 0) << 1) | (TheNode->RXOFF == 0);
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		if (TheNode->TXOFF) {
			TheNode->TXOFF = NO;
			trigger (TX_EVENT);
		}
		break;

	    case PHYSOPT_RXON:

		if (TheNode->RXOFF) {
			TheNode->RXOFF = NO;
			trigger (RX_EVENT);
		}
		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		TheNode->TXOFF = 2;
		trigger (TX_EVENT);
		break;

	    case PHYSOPT_TXHOLD:

		TheNode->TXOFF = 1;
		trigger (TX_EVENT);
		break;

	    case PHYSOPT_RXOFF:

		TheNode->RXOFF = 1;
		trigger (RX_EVENT);
		break;

	    case PHYSOPT_CAV:

		/* Force an explicit backoff */
		if (val == NULL)
			TheNode->backoff = 0;
		else
			TheNode->backoff = *val;
		trigger (TX_EVENT);
		break;

	    case PHYSOPT_SENSE:

		excptn ("PHYSOPT_SENSE unimplemented");
		break;

	    case PHYSOPT_SETPOWER:

		// Void: power not settable on DM2200
		break;

	    case PHYSOPT_GETPOWER:

		excptn ("PHYSOPT_GETPOWER unimplemented");
		break;

	    case PHYSOPT_SETSID:

		TheNode->statid = (val == NULL) ? 0 : *val;
		break;

            case PHYSOPT_GETSID:

		ret = (int) TheNode->statid;
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_SETMODE:

		excptn ("PHYSOPT_SETMODE unimplemented");
		break;

	    case PHYSOPT_GETMODE:

		excptn ("PHYSOPT_GETMODE unimplemented");
		break;

	    default:

		syserror (EREQPAR, "phys_dm2200 option");

	}
	return ret;
}

#endif
