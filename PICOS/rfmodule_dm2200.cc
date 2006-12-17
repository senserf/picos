#ifndef	__rfmodule_dm2200_cc__
#define	__rfmodule_dm2200_cc__

#include "board.h"
#include "rfmodule_dm2200.h"
#include "tcvphys.h"

#include "tcv.cc"

#include "stdattr.h"
#include "rfmattr.h"

static int option (int, address);

byte Receiver::get_rssi (byte &qual) {

	IHist *ih;
	double sl, sd, bn, ra;
	byte res;

	ih = RFInterface->iHist (ThePckt);
	assert (ih != NULL, "Receiver->get_rssi: no IHist");

	sl = RFInterface->sigLevel (ThePckt, SIGL_OWN);
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

	ra = RFInterface->sigLevel (ThePckt, SIGL_IFA) + Ether->BNoise;
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
		if (TXOFF) {
			if (TXOFF == 3) {
Drain:
				tcvphy_erase (PHYSID);
				when (tx_event, XM_LOOP);
				release;
			} else if (TXOFF == 1) {
				backoff = 0;
				when (tx_event, XM_LOOP);
				release;
			}
		}

		if ((buffer = tcvphy_get (PHYSID, &buflen)) != NULL) {
			assert (buflen >= 4 && (buflen & 1) == 0,
				"Xmitter: illegal packet length");
			buffer [0] = statid;
		} else {
			// Nothing to transmit
			if (TXOFF == 2) {
				// Draining, stop if the queue is empty
				TXOFF = 3;
				goto Drain;
			}
			when (tx_event, XM_LOOP);
			release;
		}
	}

	if (backoff && !tcv_isurgent (buffer)) {
		// backing off and the packet is not urgent
		delay (backoff, XM_LOOP);
		backoff = 0;
		when (tx_event, XM_LOOP);
		release;
	}

	if (Receiving) {
		delay (min_backoff, XM_LOOP);
		when (tx_event, XM_LOOP);
		release;
	}

	if (lbt_delay && !RXOFF) {
		// Start the ADC
		RSSI->signal ((void*)YES);
		delay (lbt_delay, XM_LBS);
		release;
	}

Xmit:
	OBuffer.load (buffer, buflen);
	Xmitting = YES;
	RFInterface->transmit (&OBuffer, XM_TXDONE);

    state XM_TXDONE:

	RFInterface->stop ();
	if (lbt_delay == 0)
		// To reduce the risk of livelocks; not needed with LBT
		gbackoff ();

	tcvphy_end (buffer);
	buffer = NULL;
	Xmitting = NO;
	trigger (rx_event);
	delay (min_backoff, XM_LOOP);

    state XM_LBS:

	RSSI->signal ((void*)NO);
	if (Receiving) {
		delay (min_backoff, XM_LOOP);
		when (tx_event, XM_LOOP);
		release;
	}

	if (RXOFF)
		goto Xmit;

	if (RSSI->sigLevel () < lbt_threshold)
		goto Xmit;

	gbackoff ();
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
	CLevel = RFInterface->sigLevel ();

	// In case something is already pending
	RFInterface->wait (ANYEVENT, ADC_UPDATE);
	this->wait (SIGNAL, ADC_STOP);

    state ADC_UPDATE:

	// Calculate the average signal level over the sampling
	// period

	DT = (double)(Time - Last);	// Time increment
	NA = ATime + DT;		// New total sampling time
	Average = ((Average * ATime) / NA) + (CLevel * DT) / NA;
	CLevel = RFInterface->sigLevel ();
	Last = Time;
	ATime = NA;
	// Only new events, no looping!
	RFInterface->wait (ANYEVENT, ADC_UPDATE);
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

	Receiving = NO;

	if (RXOFF) {
Finidh:
		when (rx_event, RCV_GETIT);
		release;
	}
	this->signal ((void*)NONE);
	RFInterface->wait (BOT, RCV_START);

    state RCV_START:

	if (Xmitting) {
		when (rx_event, RCV_GETIT);
		release;
	}

	Receiving = YES;
	RFInterface->follow (ThePckt);
	skipto RCV_RECEIVE;

    state RCV_RECEIVE:

	RFInterface->wait (EOT, RCV_GOTIT);
	RFInterface->wait (BERROR, RCV_GETIT);
	RFInterface->wait (BOT, RCV_START);

    state RCV_GOTIT:

	rssi = get_rssi (qual);

	packet = ThePckt -> Payload;
	pktlen = ThePckt -> PaySize;

	assert (pktlen > MINIMUM_PACKET_LENGTH, "Receiver: packet too short");
	if (statid != 0 && packet [0] != 0 && packet [0] != statid)
		// Ignore
		proceed RCV_GETIT;

	// Fake the RSSI for now. FIXME: do it right! Include add_entropy.
	packet [(pktlen - 1) >> 1] = ((word) rssi << 8) | qual;

	tcvphy_rcv (PHYSID, packet, pktlen);
	proceed RCV_GETIT;
}

__PUBLF (PicOSNode, void, phys_dm2200) (int phy, int mbs) {
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

		ret = ((TXOFF == 0) << 1) | (RXOFF == 0);
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		if (TXOFF) {
			TXOFF = NO;
			trigger (tx_event);
		}
		break;

	    case PHYSOPT_RXON:

		if (RXOFF) {
			RXOFF = NO;
			trigger (rx_event);
		}
		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		TXOFF = 2;
		trigger (tx_event);
		break;

	    case PHYSOPT_TXHOLD:

		TXOFF = 1;
		trigger (tx_event);
		break;

	    case PHYSOPT_RXOFF:

		RXOFF = 1;
		trigger (rx_event);
		break;

	    case PHYSOPT_CAV:

		/* Force an explicit backoff */
		if (val == NULL)
			backoff = 0;
		else
			backoff = *val;
		trigger (tx_event);
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

		statid = (val == NULL) ? 0 : *val;
		break;

            case PHYSOPT_GETSID:

		ret = (int) statid;
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

#include "rfmattr_undef.h"
#include "stdattr_undef.h"

#endif
