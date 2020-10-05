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

#include "types.h"
#include "rfmodule.h"

void Xmitter::setup (double lbd, double lbt, double mib, double mab) {

	if (lbd <= 0.0) {
		LBT_delay = TIME_0;
	} else {
		LBT_delay = etuToItu (lbd);
		LBT_threshold = dBToLin (lbt);
	}

	MinBackoff = mib;
	MaxBackoff = mab;

	// The companion process

	RSSI = create ADC;
}

TIME Xmitter::backoff (int mode) {

	// Just an illustration. You may want to make it dependent on the
	// "mode". Perhaps the backoff should be different after reception,
	// the end of transmission (packet space), and so on ...
	// For now, it is just a random delay between Min and Max.

	return etuToItu (dRndUniform (MinBackoff, MaxBackoff));
}
		
Xmitter::perform {

    WDPacket *P;

    state XM_LOOP:

	// Wait here until the packet queue becomes non-empty
	if (S->PQ->empty ()) {
		// No packet to transmit
		S->PQ->wait (NONEMPTY, XM_LOOP);
		sleep;
	}

	if (S->RFBusy) {
		// This can only mean that the receiver is currently using the
		// channel, i.e., receiving a packet. Wait for a signal from
		// the receiver: it will tell us when it is done. Then we will
		// back off a little and try again.
		S->Event->wait (EVENT_READY, XM_RBACK);
		sleep;
	}

	if (LBT_delay != TIME_0) {
		// If this is nonzero, it means that we want to listen before
		// transmission, which we do in a rather complicated way. We
		// send a signal to our companion process pointed to by RSSI.
		// That process will carefully calculate the average level of
		// all received signals within the waiting period, i.e.,
		// LBT_delay.
		RSSI->signal ((void*)YES);
		// So we wait for that much time while the companion process is
		// doing its job.
		Timer->wait (LBT_delay, XM_LBS);
	} else
		// LBT disabled, transmit right away
		goto Xmit;

    state XM_LBS:

	// Done measuring RSSI. We get here at the end of the LBT period. This
	// signal will tell the companion process to stop.
	RSSI->signal ((void*)NO);

	// Recheck if the receiver hasn't grabbed the channel while we haven't
	// been looking
	if (S->RFBusy) {
		// It has, so the game starts from scratch
		S->Event->wait (EVENT_READY, XM_RBACK);
		sleep;
	}

	// Now compare the average signal level from the waiting period to
	// the threshold
	if (RSSI->sigLevel () >= LBT_threshold) {
		// It is above the threshold, so back off and try again
		Timer->wait (backoff (BK_LBT), XM_LOOP);
		sleep;
	}
Xmit:
	// Here we are allowed to transmit. Grab the channel first.
	S->RFBusy = YES;

	// Remove the top packet from the queue
	P = (WDPacket*) (S->PQ->get ());
	// A sanity check: this should never happen as only we remove packets
	// from there.
	assert (P != NULL, "Xmitter: NULL packet");

	// Now set the antenna. Each packet put into the transmit queue is
	// tagged with a direction, which is simply the angle from East, which
	// can be up to 2 PI (counterclockwise). A packet that is to be sent
	// without the directional setting, i.e., a brodcast packet, has this
	// angle set to a negative value.
	//
	// Before transmission, we extract the angle from the packet (it isn't
	// really needed there) and plug it into the Transceiver as the Tag
	// (see SMURPH manual). The generic type of Tag is IPointer (which in
	// SMURPH means a general purpose holder for a pointer or integer).
	// We play a simple trick to properly store a float value there.
	//
	// The Tag values is interpreted by function gain in channel.cc, which
	// is invoked by the RFC_att assessment method (see channel.cc).
	//
	S->RFI->setXTag (angle_to_tag (P->Direction));

	// Transmit the packet
	S->RFI->transmit (P, XM_TXDONE);

	// And have to deallocate it. Note that transmit creates a copy of the
	// packet, which sort of goes into the ether. That copy will be
	// deallocated automatically.
	delete P;

    state XM_TXDONE:

	// We have to explicitly terminate the transmission when done
	S->RFI->stop ();

	// Not busy anymore
	S->RFBusy = NO;

	// Signal the "channel available" status
	S->Event->signal (EVENT_READY);

	// Some packet space before trying again?
	Timer->wait (backoff (BK_EOT), XM_LOOP);

    state XM_RBACK:

	// After a ready signal from the receiver
	Timer->wait (backoff (BK_EOR), XM_LOOP);
}
		
ADC::perform {

// This is the transmitter companion process, which calculates the average
// received signal level while waiting for LBT delay

    double DT, NA;

    state ADC_WAIT:

	// Wait for a go signal to start
	this->wait (SIGNAL, ADC_RESUME);

    state ADC_RESUME:

	// Initialize
	ATime = 0.0;	// Accumulated time
	Average = 0.0;	// Average so far
	Last = Time;	// Time of last update (Time == current time)

	// Current signal level
	CLevel = S->RFI->sigLevel ();

	// sigLevel returns the current signal level perceived by the
	// Transceiver. We wait for ANYEVENT (see SMURPH manual) because
	// only then can the signal level change.
	S->RFI->wait (ANYEVENT, ADC_UPDATE);

	// Of course, we also wait for a signal from the transmitter to tell
	// us that we are supposed to stop
	this->wait (SIGNAL, ADC_STOP);

    state ADC_UPDATE:

	// The signal level has (probably) changed. So we add the previous
	// sample weighted by its time to the average.

	DT = (double)(Time - Last);	// Time increment
	NA = ATime + DT;		// Total sampling time
	Average = ((Average * ATime) / NA) + (CLevel * DT) / NA;

	// New current level
	CLevel = S->RFI->sigLevel ();
	Last = Time;			// When taken
	ATime = NA;			// Accumulated time

	// The semantics of ANYEVENT has been set (see initNodes in nodes.cc)
	// to present only new events. This way we won't loop by issuing 
	// ANYEVENT immediately. Whatever has woken us us, will not show up
	// any more.
	S->RFI->wait (ANYEVENT, ADC_UPDATE);

	// And don't forget to wait for the STOP signal from the transmitter
	this->wait (SIGNAL, ADC_STOP);

    state ADC_STOP:

	// Done: wait for another request. See the sigLevel method of ADC (file
	// rfmodule.h) for the way to get hold of the measured signal level.
	proceed ADC_WAIT;
}

Receiver::perform {

    state RCV_GETIT:

	// Do nothing until the BOT assessment method (RFC_bot) tells you that
	// you are hearing a beginning of packet
	S->RFI->wait (BOT, RCV_START);

    state RCV_START:

	if (S->RFBusy) {
		// Ignore if transmitter active and wait for a signal
		S->Event->wait (EVENT_READY, RCV_GETIT);
		sleep;
	}

	// Grab the device
	S->RFBusy = YES;

	// We want to track this packet, i.e., the one that has triggered the
	// BOT
	S->RFI->follow (ThePacket);
	skipto RCV_RECEIVE;

    state RCV_RECEIVE:

	// If we have reached the EOT without a problem, the packet has been
	// received correctly
	S->RFI->wait (EOT, RCV_GOTIT);

	// If a bit error has occurred earlier, the reception is aborted
	S->RFI->wait (BERROR, RCV_ABORT);

	// This one is just in case and I don't think is possible under sane
	// circumstances. If we recognize another BOT while listening to the
	// packet, we start receiving from scratch. If things are sane, we
	// should first have a serious interference and a bit error.
	S->RFI->wait (BOT, RCV_RESTART);

    state RCV_ABORT:

	// Aborted reception
	S->RFBusy = NO;
	S->Event->signal (EVENT_READY);
	proceed RCV_GETIT;

    state RCV_RESTART:

	// Restarted reception (for a new BOT)
	S->RFBusy = NO;
	S->Event->signal (EVENT_READY);
	proceed RCV_START;

    state RCV_GOTIT:

	// Successful reception
	S->RFBusy = NO;
	S->Event->signal (EVENT_READY);

	// Call the Node's method that will determine the packet's fate
	S->receive ((WDPacket*)ThePacket);

	proceed RCV_GETIT;

}
