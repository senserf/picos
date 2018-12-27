/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

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

#include "protocol.h"

// This is the constructor for the sender station
void Sender::setup (int ws, TIME tm) {
	Transmitter *xm;
	// This is the output port - used for transmission; the parameter
	// gives the transmission rate in time units per bit; thus, our
	// internal time units are simply bits
	Out = create Port (1);
	// This one is used exclusively for reception and needs no transmission
	// rate
	In = create Port ();
	// The station runs two processes: the transmitter sends out frames ...
	xm = create Transmitter (ws, tm);
	// ... and the ack monitor receives acknowledgements
	create AckMonitor (xm);
	// These are counters for calculating some obvious statistics
	TransmissionCounter = RetransmissionCounter =
		TransmittedBits = RetransmittedBits = 0;
};

// This is the constructor for the transmitter process
void Transmitter::setup (int ws, TIME tm) {
	// Store the window size
	WS = ws;
	// Create and initialize the sliding window; one entry in that
	// structure stores a Frame and its time stamp; the time stamp
	// gives the time when the frame's last bit was transmitted
	SW = new SlidingWindow (WS, sizeof (Frame), sizeof (TIME));
	NFrames = Current = 0;
	// Make this anything that cannot be viewed as a valid Sequence Number
	OldAck = LastAck = ws;
	Timeout = tm;
	Outgoing = 0;
	// Copy some attributes from your station for easier access
	OF = &(S->OF);
	Out = S->Out;
};

int Transmitter::nextSN () {
	// Return and update the serial number for next outgoing frame; the
	// range of those numbers is 0...WS
	int	ns = Outgoing;
	Outgoing = (Outgoing + 1) % (WS + 1);
	// trace ("Outgoing SN %1d", Outgoing);
	return ns;
};

// This is the "code" of the transmitter process
Transmitter::perform {

	TIME ts;

	state WakeUp:

		// We are restarted at this state whenever there is
		// something (anything) that should be taken care of.
		// We are the only process allowed to fool around with
		// the sliding window, so we do not have to worry about
		// synchronization.

		erase ();	// Erase the signal from AckMonitor, if any

		// trace ("Transmitter wakeup %1d", NFrames);

		if (NFrames > 0) {
			// There are some outstanding frames in the window
			if (LastAck != OldAck) {
				// A new acknowledgement has arrived - advance
				// the window
				do {
					assert (NFrames > 0,
						"illegal acknowledgement");
					// Extract next frame from the front
					SW->get (OF, 0);
					// Tell the upper layers that we are
					// done with this frame for good
					UpperLayer->release (OF);
					// The window shrinks
					NFrames--;
					// ... and slides one frame forward
					SW->slide ();
					// Have to backspace Current because it
					// counts positions from the beginning
					if (Current > 0)
						Current--;
					// Keep going until you hit the frame
					// that has been acknowledged
				} while (OF->SN != LastAck);
				// Mark the acknowledgement at processed
				OldAck = LastAck;
				// Redo your stuff for the updated window
				proceed WakeUp;
			}
			// No new ACK; check if there's a timeout for the
			// first frame
			SW->getStatus (&ts, 0);
			if (ts + Timeout <= Time) {
				// Timeout - reset the window pointer to the
				// front
				Current = 0;
			}
			// If Current points into the window, retransmit
			// frames from there
			if (Current >= 0 && Current < NFrames) {
				// Get current frame ...
				SW->get (OF, Current);
				// ... start transmitting it ...
				Out->transmit (OF, XDone);
				// ... update statistics ...
				++(S->RetransmissionCounter);
				S->RetransmittedBits += OF->TLength;
				// trace ("ReTransmitting %1d", OF->SN);
				// ... and wait until transmitted
				sleep;
			}
			// We are behind the window
		}
		// Check if we can accommodate a new frame into the window
		if (NFrames < WS) {
			// Mark the frame buffer as empty; otherwise, getPacket
			// will refuse to overwrite it
			clearFlag (OF->Flags, PF_full);
			// Attempt to acquire a frame from the "upper layers"
			if (!UpperLayer->getPacket (OF, MinFrameLength,
				MaxFrameLength, 0)) {
				// There isn't any at the moment - must wait
				// for an arrival event
				UpperLayer->wait (ARRIVAL, WakeUp);
				// But we are not blocked at this point - just
				// indicating one more event to wake us up
			} else {
				// Acquired a new frame - add it to the window
				// with the right sequence number
				OF->SN = nextSN ();
				// Point to it
				Current = NFrames;
				SW->put (OF, Current);
				// Update window occupancy count ...
				NFrames++;
				// ... and transmit the new frame
				Out->transmit (OF, XDone);
				// Update statistics ...
				++(S->TransmissionCounter);
				S->TransmittedBits += OF->TLength;
				// ... and wait until transmission completed
				// trace ("XXTransmitting %1d", OF->SN);
				sleep;
			}
		}
		// Now we have to wait for whatever is going to happen next
		if (NFrames > 0) {
			// There is at least one frame in the window; set up
			// an alarm clock for the timeout of the first one
			SW->getStatus (&ts, 0);
			// ts == time when transmitted + Timeout
			ts += Timeout;
			// trace ("New timeout %1lld", ts);
			assert (ts > Time, "missing timeout");
			// Will go off then if nothing happens earlier
			Timer->wait (ts - Time, WakeUp);
			// Wait for the ack monitor as well
			this->wait (SIGNAL, WakeUp);
		}
		// If we get here and the preceding 'if' was void (i.e., the
		// window is empty), then we are waiting for the Upper Layer
		// to deliver us a new frame; thus, no matter what, when we
		// get here we are expecting at least one event

	state XDone:

		// This state handles the end of transmission
		// trace ("Transmitter EOT");
		// Terminate the transmission
		Out->stop ();
		// Set the "transmitted" time for the current frame to "now"
		SW->setStatus (&Time, Current);
		// And advance the frame pointer
		Current++;
		// Keep going
		proceed WakeUp;
};

// This is the constructor of the acknowledgement monitor
void AckMonitor::setup (Transmitter *xm) {

	// Save the pointer to your Transmitter; we are acting as a helper
	// for that process
	XM = xm;
	// The port of our station that we listen to
	In = S->In;
};

// This is the "code" of the acknowledgement monitor
AckMonitor::perform {

	int sn;

	state WaitAck:

		// Wait for a frame reception on your port; a frame is
		// assumed to be received completely, if it triggers the
		// EOT (end of transmission) event; note that this event
		// will not be triggered if the frame has been corrupted;
		// this is taken care of by SMURPH
		In->wait (EOT, NextAck);

	state NextAck:

		// Extract the sequence number of the acknowledgement; ThePacket
		// points to the received frame; this is a standard variable
		// of SMURPH
		sn = ((Ack*) ThePacket) -> SN;
		// trace ("Received ACK %1d, last was %1d", sn, XM->OldAck);
		if (sn != XM->OldAck) {
			// This is a new acknowledgement (otherwise it is
			// ignored); by setting Transmitter's LastAck, we tell
			// it to advance the window
			XM->LastAck = sn;
			// Send a signal to the Transmitter to wake it up
			XM->signal ();
		}
		// Ignore the EOT and wait for another scknowledgment
		skipto WaitAck;
};

// This is the constructor for the Recipient station
void Recipient::setup (int ws, TIME tm) {
	Acknowledger *ac;
	// Output port - for sending acknowledgements
	Out = create Port (1);
	// Input port - for receiving frames
	In = create Port ();
	ACK = create Ack;
	// The acknowledgement frame is fixed and we build it by hand
	ACK -> fill (this, ALL, AckLength);
	// Create the two processes: one sends out acknowledgements, the
	// other receives frames from the Sender
	ac = create Acknowledger (ws, tm);
	create Receiver (ac);
};

void Receiver::setup (Acknowledger *ac) {

	AC = ac;
	In = S->In;
};

Receiver::perform {

	Frame *IF;

	state WaitFrame:

		// Wait for an end of a correctly received frame
		In->wait (EOT, NewFrame);

	state NewFrame:

		// This is the frame that we have just received
		IF = (Frame*) ThePacket;
		// trace ("Received %1d, expected %1d", IF->SN, AC->Expected);
		if (IF->SN == AC->Expected) {
			// We have been expecting it; this method is going to
			// advance Expected and wake up the Acknowledger - to
			// send an acknowledgement
			AC->nextSN ();
			// Pass the frame to the Upper Layer and forget about
			// it
			UpperLayer->receive (IF, ThePort);
		}
		// Skip the EOT event and wait for another frame
		skipto WaitFrame;
};

void Acknowledger::setup (int ws, TIME tm) {

	Timeout = tm;
	Out = S->Out;
	LastReceived = WS = ws;
	Expected = 0;
	ACK = S->ACK;
};

void Acknowledger::nextSN () {

	LastReceived = Expected;
	Expected = (Expected + 1) % (WS + 1);
	this->signal ();
};

Acknowledger::perform {

	state WaitSignal:

		// Wait for a signal from the Receiver or timeout
		this->wait (SIGNAL, SendAck);
		Timer->wait (Timeout, SendAck);

	state SendAck:

		// Whatever happens, acknowledge the last received frame
		erase ();
		// trace ("Acknowledging %1d", LastReceived);
		ACK->SN = LastReceived;
		Out->transmit (ACK, XDone);

	state XDone:

		Out->stop ();
		proceed WaitSignal;
};
