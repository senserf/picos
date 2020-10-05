/*
	Copyright 1995-2020 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __rfmod_raw_cc__
#define __rfmod_raw_cc__

#include "rfmod_raw.h"

#if DEBUGGING
char *_zz_dpk (Packet *p) {
/*
 * Dump a packet to a character string
 */
	return form ("T %1d, S %1d, R %1d, L %1d [%1d]",
		p->TP, p->Sender, p->Receiver, p->TLength, p->ILength);
}
#endif

Xmitter::perform {

    state XM_LOOP:

	if (CP == NULL)
		// Try to acquire a packet to transmit
		CP = RFM->get_packet (XM_LOOP);

	RFM->wait_backoff (XM_LOOP);
	RFM->wait_receiver (XM_LOOP);
	RFM->wait_lbt (XM_LBS);
Xmit:
	RFM->xmit_start (CP, XM_TXDONE);

    state XM_TXDONE:

	RFM->xmit_stop (CP);
	CP = NULL;
	proceed XM_LOOP;

    state XM_LBS:

	RFM->lbt_ok (XM_LOOP);
	goto Xmit;
}

void Collector::setup (RFModule *rfm) {
	RFM = rfm;
	RFM->SigSensor = this;
}

Collector::perform {

    double DT, NA;

    state SSN_WAIT:

	this->wait (SIGNAL, SSN_RESUME);

    state SSN_RESUME:

	assert (ptrToLong (TheSignal) == YES, "Collector: illegal ON signal");
	
	ATime = 0.0;
	Average = 0.0;
	Last = Time;

	CLevel = RFM->sig_monitor (SSN_UPDATE, SSN_STOP);

    state SSN_UPDATE:

	// Calculate the average signal level over the sampling
	// period

	DT = (double)(Time - Last);	// Time increment
	NA = ATime + DT;		// New total sampling time
	Average = ((Average * ATime) / NA) + (CLevel * DT) / NA;
	Last = Time;
	ATime = NA;
	CLevel = RFM->sig_monitor (SSN_UPDATE, SSN_STOP);

    state SSN_STOP:

	Assert (ptrToLong (TheSignal) == NO, "Collector: illegal OFF signal");
	// Done: wait for another request
	proceed SSN_WAIT;
}

Receiver::perform {

    state RCV_GETIT:

	RFM->init_rcv (RCV_START);

    state RCV_START:

	RFM->start_rcv (RCV_GETIT, RCV_RECEIVE);

    state RCV_RECEIVE:

	RFM->wait_rcv (RCV_GOTIT, RCV_GETIT, RCV_START);

    state RCV_GOTIT:

	RFM->receive ();
	proceed RCV_GETIT;
}

RFModule::RFModule (Transceiver *xcv, Long qsize, double lbth, double lbtd) {

	PQ = create PQueue (qsize);
	LBT_delay = etuToItu (lbtd);
	LBT_threshold = lbth;
	TBackoff = TIME_0;
	Receiving = Xmitting = NO;
	SigSensor = NULL;
	Xcv = xcv;

	create Xmitter (this);
	create Receiver (this);
	if (LBT_threshold > 0.0)
		create Collector (this);
}

#endif
