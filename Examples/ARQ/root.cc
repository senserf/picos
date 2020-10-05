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

#include "protocol.h"

int		MinFrameLength,		// These are in bits, of course
		MaxFrameLength,
		AckLength,
		WindowSize;

TrGen		*UpperLayer;

Sender		*Snd;
Recipient	*Rcp;

process Root {

	void network ();
	void output ();

	states {Start, Stop};

	perform {
		state Start:

			network ();
			Kernel->wait (DEATH, Stop);

		state Stop:

			output ();
			terminate;
	};
};

void Root::network () {

	double		r, d;
	PLink		*Link1, *Link2;
	TIME		SenderTimeout,
			ReceiverTimeout;
	DISTANCE	LinkLength;
	// Traffic parameters
	double		MINML,		// Minimum message length
			MAXML,		// Maximum message length
			MInt,		// Mean message interarrival time
			LinkErrorRate;	// Probability of a bit error

	BIG		MLimit,		// Number of messages to receive
			TLimit;		// Simulated time limit

	// Create the network and start things up

	// Rate in bits per second
	readIn (r);
	print (r, "Transmission rate:", 15, 50);

	// Set the time unit, i.e., second to the right number of bits
	setEtu (r);

	// This is the link length in meters
	readIn (d);
	print (d, "Link length:", 15, 50);
	LinkLength = (DISTANCE) (d * (r/200000000.0));
	print (LinkLength, "The same in bits:", 15, 50);

	// Error rate
	readIn (LinkErrorRate);
	print (LinkErrorRate, "Link bit error rate:", 15, 50);

	// Frame/acknowledgement length
	readIn (MinFrameLength);
	readIn (MaxFrameLength);
	readIn (AckLength);
	print (MinFrameLength, "Minimum frame length:", 15, 50);
	print (MaxFrameLength, "Maximum frame length:", 15, 50);
	print (AckLength, "Acknowledgement length:", 15, 50);

	// Window size
	readIn (WindowSize);
	print (WindowSize, "Window size:", 15, 50);

	// Timeouts
	readIn (d);
	print (d, "Sender timeout:", 15, 50);
	SenderTimeout = (TIME)(Etu * d);
	print (SenderTimeout, "The same in bits:", 15, 50);
	readIn (d);
	print (d, "Receiver timeout:", 15, 50);
	ReceiverTimeout = (TIME)(Etu * d);
	print (ReceiverTimeout, "The same in bits:", 15, 50);

	// Traffic generator
	readIn (MINML);
	print (MINML, "Minimum message length:", 15, 50);
	readIn (MAXML);
	print (MAXML, "Maximum message length:", 15, 50);
	readIn (MInt);
	print (MInt, "Mean message interarrival time:", 15, 50);

	// When to stop
	readIn (MLimit);
	print (MLimit, "Stop after receiving this many messages:", 15, 50);
	readIn (d);
	print (d, "Stop after this many seconds:", 15, 50);
	TLimit = (TIME)(Etu * d);
	print (TLimit, "Equivalent to this many bits:", 15, 50);

	print ("\n\n");

	// Create all this stuff

	Link1 = create PLink (2);
	Link2 = create PLink (2);

	Snd = create Sender (WindowSize, SenderTimeout);
	Rcp = create Recipient (WindowSize, ReceiverTimeout);

	Snd->Out->connect (Link1);
	Rcp->In->connect (Link1);
	Rcp->Out->connect (Link2);
	Snd->In->connect (Link2);

	Snd->Out->setDTo (Rcp->In, LinkLength);
	Rcp->Out->setDTo (Snd->In, LinkLength);

	Link1->setFaultRate (LinkErrorRate, FT_LEVEL2);
	Link2->setFaultRate (LinkErrorRate, FT_LEVEL2);

	UpperLayer = create TrGen (MIT_exp + MLE_unf, MInt, MINML, MAXML);
	UpperLayer -> addSender (Snd);
	UpperLayer -> addReceiver (Rcp);

	setQSLimit (10000);

	setLimit ((LONG) MLimit, TLimit);
};

void Root::output () {

	print (Snd->TransmissionCounter, "Number of transmissions:", 15,50);
	print (Snd->TransmittedBits, "Number of transmitted bits:", 15,50);
	print (Snd->RetransmissionCounter, "Number of retransmissions:", 15,50);
	print (Snd->RetransmittedBits, "Number of retransmitted bits:", 15,50);
	print ("\n\n");

	Client->printPfm ();

};
