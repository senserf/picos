#ifndef	__rfmodule_cc__
#define	__rfmodule_cc__

#include "board.h"
#include "rfmodule.h"
#include "tcvphys.h"
#include "stdattr.h"
#include "tcv.cc"
#include "rfmattr.h"
#include "rfleds.h"

static int option (int, address);

byte Receiver::get_rssi (byte &qual) {

	word wr;

	// Let us ignore this for a while
	qual = 0;

	if (Ether->RSSIC == NULL)
		// No RSSI calculator present
		return 0;

	wr = Ether->RSSIC->calculate (RFInterface->sigLevel (ThePckt,
		SIGL_OWN));

	if (wr > 255)
		wr = 255;

	return (byte) wr;
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
				"Xmitter: illegal packet length %1d", buflen);
			if (statid != 0xffff)
				// otherwise, honor the packet's statid
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
	LEDI (1, 1);
	RFInterface->transmit (&OBuffer, XM_TXDONE);

    state XM_TXDONE:

	RFInterface->stop ();
	LEDI (1, 0);
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

	assert (ptrtoint (TheSignal) == YES, "ADC: illegal ON signal");
	
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

	Assert (ptrtoint (TheSignal) == NO, "ADC: illegal OFF signal");
	// Done: wait for another request
	proceed ADC_WAIT;
}

Receiver::perform {

    address packet;
    int     pktlen;
    byte    rssi, qual;

    state RCV_GETIT:

	Receiving = NO;
	LEDI (2, 0);

	if (RXOFF) {
Finidh:
		when (rx_event, RCV_GETIT);
		release;
	}
	this->signal ((void*)NONE);
	RFInterface->wait (BOT, RCV_START);
	when (rx_event, RCV_GETIT);

    state RCV_START:

	if (Xmitting) {
		when (rx_event, RCV_GETIT);
		release;
	}
	LEDI (2, 1);
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

	if (statid != 0 && statid != 0xffff) {
		// Admit only packets with agreeable statid
		if (packet [0] != 0 && packet [0] != statid)
			// Ignore
			proceed RCV_GETIT;
	}
		
	// Fake the RSSI for now. FIXME: do it right! Include add_entropy.
	packet [(pktlen - 1) >> 1] = ((word) rssi << 8) | qual;

	tcvphy_rcv (PHYSID, packet, pktlen);
	proceed RCV_GETIT;
}

__PUBLF (PicOSNode, void, phys_cc1100) (int phy, int mbs) {
/*
 * phy  - interface number
 * mbs  - maximum packet length (including checksum, must be divisible by 4)
 */
	if (mbs < 6 || mbs > CC1100_MAXPLEN) {
		if (mbs == 0)
			mbs = CC1100_MAXPLEN;
		else
			syserror (EREQPAR, "phys_cc1100");
	}

	if (memBook (mbs+2) == NO)
		syserror (EMALLOC, "phys_cc1100");

	phys_rfmodule_init (phy);
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

	phys_rfmodule_init (phy);
}

void PicOSNode::phys_rfmodule_init (int phy) {

	statid = 0;
	backoff = 0;

	/* Register the phy */
	tx_event = tcvphy_reg (phy, option, INFO_PHYS_DM2200);

	/* Both parts are initially active */
	RXOFF = TXOFF = 1;
	LEDI (0, 2);
	LEDI (1, 0);
	LEDI (2, 0);

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

		if (RXOFF)
			LEDI (0, 1);
		else
			LEDI (0, 2);

		break;

	    case PHYSOPT_RXON:

		if (RXOFF) {
			RXOFF = NO;
			trigger (rx_event);
		}

		if (TXOFF)
			LEDI (0, 1);
		else
			LEDI (0, 2);

		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		TXOFF = 2;
		trigger (tx_event);

		if (RXOFF)
			LEDI (0, 0);
		else
			LEDI (0, 1);
		break;

	    case PHYSOPT_TXHOLD:

		TXOFF = 1;
		trigger (tx_event);

		if (TXOFF)
			LEDI (0, 1);
		else
			LEDI (0, 2);
		break;

	    case PHYSOPT_RXOFF:

		RXOFF = 1;
		trigger (rx_event);

		if (TXOFF)
			LEDI (0, 1);
		else
			LEDI (0, 2);
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

		if (Ether->PS != NULL)
			RFInterface->setXPower ((val == NULL) ? 
				DefXPower : Ether->PS->setvalue (*val));
		break;

	    case PHYSOPT_GETPOWER:

		if (Ether->PS == NULL)
			excptn ("PHYSOPT_GETPOWER unimplemented");

		ret = Ether->PS->getvalue (RFInterface->getXPower ());

		if (val != NULL)
			*val = ret;
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
