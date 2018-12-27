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

#include "types.h"
#include "traffic.h"

void DataPacket::setup (Message *m) {

// Sets up the nonstandard attributes of a data packet.

	TTL = PKT_TTL_DATA;
	DCFP_S = Sender;
	DCFP_Flags = 0;
	SN = TheNode -> Serial++;
}

void initTraffic () {

// Here is a very simple traffic generator for data packets. The arrival process
// is Poisson with the mean of MINT seconds, and the message (payload) length is
// uniformly distributed between MinML and MaxML. The length is always rounded
// to a multiple of 8 (i.e., full bytes).

	double MinML, MaxML, MINT;
	UTraffic *U;

	// This is a very simple uniform traffic pattern with uniformly
	// distributed message (packet) length and exponential inter-arrival
	// time.

	readIn (MinML);		// Minumum message length in bytes
	readIn (MaxML);		// Maximum message length in bytes
	readIn (MINT);		// Mean interarrival time in seconds

	//                                          ... must be in bits ...
	U = create UTraffic (MIT_exp + MLE_unf, MINT, MinML * 8, MaxML * 8);

	// All node are senders
	U->addSender (ALL);
	// ... and receivers
	U->addReceiver (ALL);
}
