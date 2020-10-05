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
#include "rwpmm.cc"

static char *pkt_dump (WDPacket *p) {

// For debugging

	if (p->TP == PKT_TYPE_HELLO) 
		return form ("PKT HELLO S %1d [%1d], TTL: %1d, SN %1d",
			p->Sender, p->DCFP_S, p->TTL, p->SN);
	else
		return form ("PKT DATA S %1d [%1d], R %1d [%1d], "
			"L %1d (%1d), TTL %1d, SN %1d",
			(p)->Sender, (p)->DCFP_S, (p)->Receiver,
			(p)->DCFP_R, (p)->ILength,
			(p)->TLength, (p)->TTL, (p)->SN);
}

Boolean PCache::add (WDPacket *P) {

// Checks if the packet has been seen previously and if not, adds the packet
// information to the cache. The info consists of two values: the "transport"
// Sender + the serial number assigned by the sender.

	int n;
	PCache_i *ce;

	n = MaxSize;

	// SMURPH offers simple tools for handling "pools", which are
	// doubly-linked lists of some items (see the manual)
	for_pool (ce, Head) {
		if (ce->Sender == P->Sender & ce->SN == P->SN)
			// Found
			return YES;
		n--;
	}

	// Not found, add at front

	if (n) {
		// Room available
		ce = new PCache_i;
		if (n == MaxSize)
			// First addition: initialize Tail
			Tail = ce;
	} else {
		// Remove the tail. We follow the FIFO policy for removing
		// old entries from the cache.
		ce = Tail;
		pool_out (ce);
		Tail = ce->prev;
	}
	ce->Sender = P->Sender;
	ce->SN = P->SN;

	pool_in (ce, Head);

	return NO;
}

Boolean RTable::getCoords (Long N, double &X, double &Y) {

// This one is a method of RTable (routing table). It locates the specified
// node and returns its coordinates, if found. What we call a "routing table"
// is in fact a "pool" of nodes with their coordinates.

	RTE_i *re;

	for_pool (re, Head) {
		if (re->A == N) {
			// Found
			X = re->X;
			Y = re->Y;
			return YES;
		}
	}
	// Not found
	return NO;
}

void RTable::update (Long N, double X, double Y) {

// This one updates the "routing table" with the coordinates of node number
// N.

	RTE_i *re;

	for_pool (re, Head) {
		if (re->A == N)
			break;
	}

	if (re) {
		// The node already exists
#if TRACE_ROUTES
		if (re->X == X && re->Y == Y)
			trace ("ROUTING table, location unchanged");
		else
			trace ("ROUTING table, location for %1d changed: "
				"[%f,%f]", N, X, Y);
#endif
		re->X = X;
		re->Y = Y;
	} else {
		re = new RTE_i;
		re->A = N;
		re->X = X;
		re->Y = Y;
		pool_in (re, Head);
#if TRACE_ROUTES
		trace ("ROUTING table, new entry: <%1d,%f,%f>", N, X, Y);
#endif
	}
}

void NTable::update (Long N, double X, double Y, double rssi) {

// This one updates the neighbor table

	NTE_i *re, *rf;
	double delta;

	for_pool (re, Head) {
		if (re->A == N)
			break;
	}

	if (re) {
		if (re->X == X && re->Y == Y) {
			// Location unchanged: average RSSI; allowing for RSSI
			// fluctuations, we calculate the "current" RSSI of the
			// neighbors as an exponential moving average; this is
			// stupid, of course, as the RSSI's are themselves
			// random numbers coming from some distribution (with
			// a well-defined average); just for illustration
			rssi = ema (re->RSSI, rssi, 0.25);
#if TRACE_ROUTES
			trace ("NEIGHBOR table, location unchanged, rssi = %g",
				rssi);
#endif
		} else {
			// Location changed, RSSI starts from scratch
#if TRACE_ROUTES
			trace ("NEIGHBOR table, location for %1d changed: "
				"[%f,%f], rssi = %g", N, X, Y, rssi);
#endif
		}
		// Update the average RSSI; AvgRSSI gives you the average RSSI
		// over all your neighbors
		AvgRSSI += (rssi - re->RSSI) / NN;

		re->X = X;
		re->Y = Y;
		re->TStamp = Time;

	} else {

		// This is a new neighbor

		re = new NTE_i;
		re->A = N;
		re->X = X;
		re->Y = Y;
		pool_in (re, Head);
#if TRACE_ROUTES
		trace ("NEIGHBOR table, new entry: <%1d,%f,%f>, rssi = %g",
			N, X, Y, rssi);
#endif
		if (NN == 0)
			RelRSSI = rssi;
		NN++;
		// Update the average RSSI over all neighbors
		AvgRSSI = (AvgRSSI * (NN - 1) + rssi) / NN;
	}

	re->RSSI = rssi;

	if (rssi < MinRSSI)
		// Calculate the minimum RSSI over your neighbors
		MinRSSI = rssi;

	if (rssi > MaxRSSI)
		// ... and the maximum
		MaxRSSI = rssi;

	if (rssi < RelRSSI)
		// ... and tentatively push the threshold towards the new
		// RSSI as to allow the node to explore the neighbor
		RelRSSI = ema (RelRSSI, rssi, 0.9);

#if TRACE_ROUTES
	trace ("AVG: %g, MIN: %g, MAX: %g, REL: %g", AvgRSSI, MinRSSI, MaxRSSI,
		RelRSSI);
#endif
	re->TStamp = Time;
}

void NTable::unreliable (Long n) {

// Called after neighbor n fails to receive our packet

	NTE_i *re;

	for_pool (re, Head) {
		if (re->A == n) {
			// The neighbor has failed to receive the last packet;
			// we set the reliable RSSI threshold to the average
			// of this node's and the MAX; in other words, we say
			// that we will prefer nodes whose RSSI is no weaker
			// than half way between the current node's and	the
			// closest node's (trying to use neighbors that are
			// closer to us than the failed one)
			RelRSSI = (MaxRSSI + re->RSSI) / 2.0;
#if TRACE_ROUTES
			trace ("NEIGHBOR table, %1d marked as unreliable", n);
			trace ("AVG: %g, MIN: %g, MAX: %g, REL: %g",
				AvgRSSI, MinRSSI, MaxRSSI, RelRSSI);
#endif
			return;
		}
	}
}

Long Node::route (Long d) {

// Called to find the neighbor to forward our packet to destination d

	double X_own, Y_own, X_dest, Y_dest;
	double CD, MD, DD, RT;
	Long N;
	int i;
	NTE_i *re;

	if (Neighbors->NN == 0) {
		// We have no neighbors
		trace ("destination %d unreachable, no neighbors", d);
		return NONE;
	}

	// Destination coordinates
	if (!NetMap->getCoords (d, X_dest, Y_dest)) {
		// Destination unknown
#if TRACE_ROUTES
		trace ("destination %d, coordinates unknown");
#endif
		return NONE;
	}

	// Own location
	TheNode->RFI->getLocation (X_own, Y_own);

	// Distance from here to the destinaton
	CD = dist (X_own, Y_own, X_dest, Y_dest);
	MD = HUGE;
	N = NONE;

	// We execute two turns to find the next hop node. The first time
	// around, we only consider nodes with RSSI >= the current reliability
	// threshold. If that fails to find a neighbor getting us closer to the
	// destination, we redo the same with all neighbors. In each turn we
	// try to locate the neighbor located closest to the destination
	// (but within the RSSI threshold in the first turn).

	for (i = 0; i < 2; i++) {
		// Two turns: 1. RSSI above threshold, 2. Any RSSI

		RT = i ? -HUGE : Neighbors->RelRSSI;

		for_pool (re, Neighbors->Head) {

			if (re->RSSI < RT)
				continue;

			if (re->A == d) {
#if TRACE_ROUTES
				trace ("destination reachable in one hop at "
					"rssi %g", re->RSSI);
#endif
				// Destination available in one hop
				return d;
			} else {

				DD = dist (re->X, re->Y, X_dest, Y_dest);
				if (DD + TINY_DISTANCE > CD) {
					// No distance improvement
					continue;
				}
			}
	
			if (DD < MD) {
				MD = DD;
				N = re->A;
			}
		}

		if (N != NONE) {
			// Go there
#if TRACE_ROUTES
			trace ("ROUTE to %1d [%f], via %1d [%f] %g", d, CD, N,
				MD, RT);
#endif
			return N;
		}

		if (i == 0 && RT <= Neighbors->MinRSSI)
			// No need to go for a second round; current threshold
			// already low
			break;

		RT = Neighbors->MinRSSI;
	}

#if TRACE_ROUTES
	trace ("ROUTE to %1d [%f], NOT FOUND, packet dropped, %g", d, CD, RT);
#endif
	return NONE;
}

void NTable::deleteOld (TIME et) {

// Deletes obsolete entries from the neighbor table

	NTE_i *pe;
	int nd;
	double mind, maxd, sumd;

	nd = 0;
	sumd = 0.0;
	maxd = -HUGE;
	mind = HUGE;
	for_pool (pe, Head) {
		if (pe->TStamp + et < Time) {
			nd++;
			sumd += pe->RSSI;
			if (pe->RSSI < mind)
				mind = pe->RSSI;
			if (pe->RSSI > maxd);
				maxd = pe->RSSI;
#if TRACE_ROUTE
			trace ("NEIGHBOR table, entry for %1d timed out",
				pe->A);
#endif
		}
	}

	// Re-calculate statistics: only if we have deleted some neighbors
	if (nd) {
		if (nd == NN) {
			// Empty pool
			AvgRSSI = 0.0;
			MinRSSI = HUGE;
			RelRSSI = MaxRSSI = -HUGE;
			trim_pool (pe, Head, pe->TStamp + et < Time);
		} else {
			AvgRSSI = (AvgRSSI * NN - sumd) / (NN - nd);
			trim_pool (pe, Head, pe->TStamp + et < Time);
			if (mind == MinRSSI || maxd == MaxRSSI) {
				for_pool (pe, Head) {
					if (pe->RSSI < MinRSSI)
						MinRSSI = pe->RSSI;
					if (pe->RSSI > MaxRSSI);
						MaxRSSI = pe->RSSI;
				}
			}
		}
		NN -= nd;
	}
}
		
void Node::receive (WDPacket *P, double rssi) {

// This method determines what to do with a received packet

	Long NE;

	// Use RSSI in decibells
	rssi = linTodB (rssi);

#if TRACE_PACKETS
	trace ("ARRIVED: %s, RSSI = %gdBm", pkt_dump (P), rssi);
#endif
	if (P->Sender == getId ()) {
		// This packet was sent by us - ignore it no matter
		// what
#if TRACE_PACKETS
		trace ("OWN packet, ignored");
#endif
		return;
	}

	// Duplicate check
	if (PC->add (P)) {
		// The packet has been seen recently - drop it
#if TRACE_PACKETS
		trace ("DUPLICATE packet, ignored");
#endif
		return;
	}

	if (P->TP == PKT_TYPE_HELLO) {
		// This is a HELLO packet: update your tables
		if (P->Sender == P->DCFP_S)
			// This means that the packet was sent by a neighbor
			// (its MAC-level sender is the same as the "transport"
			// sender). Add the coordinates to the Neighbor Pool.
			Neighbors->update (P->Sender,
					((HelloPacket*)P)->X,
					((HelloPacket*)P)->Y, rssi);
		// Always update the global routing table. It stores the
		// coordinates of all nodes in the network.
		NetMap->update (P->Sender,
				((HelloPacket*)P)->X,
				((HelloPacket*)P)->Y);
	} else {
		// This is a data packet: check if addressed to us
		if (P->isMy ()) {
			// This is SMURPH's way to check if you are the
			// "transport" recipient of the packet. You are, so
			// pass the packet to "upper layers". This is done
			// by the (standard) receive method of Client.
#if TRACE_PACKETS
			trace ("PACKET RECEIVED");
#endif
			Client->receive (P, RFI);
			return;
		}
	}

	// Check if should retransmit the packet
	if (P->TTL <= 1) {
		// The packet has made its last allowed hop
#if TRACE_PACKETS
		trace ("TTL LIMIT, packet dropped");
#endif
		return;
	}

	// Note that you are not allowed to modify P, as this is the "Ether"
	// copy that may be seen by other nodes. Thus, for example, I
	// didn't decrement the TTL yet. We can only do it when we create
	// a new copy of the packet to be retransmitted. We want to postpone
	// that until we know for a fact that the packet will actually be
	// retransmitted.

	if (P->TP != PKT_TYPE_HELLO) {

		// It is a data packet, and it wasn't addressed to us. Thus,
		// we are supposed to forward it towards the destination. So
		// we invoke the routing function.
		
		if ((NE = route ((Long)(P->Receiver))) == NONE)
			// Sorry, no way
			return;

		// Now is the time to create a copy of the packet. The best
		// way to copy a packet is to call its clone method.
		P = (WDPacket*) (P->clone ());
		// And now we can decrement the TTL
		--(P->TTL);
		// Initialize the number of routing attempts
		P->Retries = 0;
		// New next-hop node
		P->DCFP_R = NE;
	} else {
		// Here we handle the case of a forwarded HELLO packet
		P = (WDPacket*) (P->clone ());
#if TRACE_ROUTES
		trace ("HELLO REBROADCAST");
#endif
		--(P->TTL);
	}

	// We are the new MAC-level sender
	P->DCFP_S = getId ();
	// Queue the packet for (re) transmission

	if (RFM->send (P)) {
		// Packet queued for transmission, we shall learn when it
		// is done, so we can deallocate it
#if TRACE_PACKETS
		trace ("PACKET QUEUED for forwarding");
#endif
		NOP;
	} else {
#if TRACE_PACKETS
		trace ("QUEUE FULL, packet dropped");
#endif
		// No room in queue: drop the packet
		delete P;
	}
}

void Node::dispatch () {

// This one is called to send a data packet that originates at this node

	Long NE;
	WDPacket *P;

#if TRACE_PACKETS
	trace ("DISPATCH: %s", pkt_dump (&Buffer));
#endif

	// This is essentially the same as for routing (in receive). A bit
	// simpler because we know that this is is a data packet and that it
	// just begins its life.

	if ((NE = route (Buffer.Receiver)) == NONE)
		// No way
		return;

	P = (WDPacket*) (Buffer.clone ());

	P->DCFP_R = NE;

	P->Retries = 0;

	if (RFM->send (P)) {
#if TRACE_PACKETS
		trace ("PACKET QUEUED for transmission");
#endif
		NOP;
	} else {
#if TRACE_PACKETS
		trace ("QUEUE FULL, packet dropped");
#endif
		delete P;
	}
}

void HelloPacket::setup () {

// This is the initialization of a HELLO packet

	Node *TN = TheNode;

	// Insert this node's location
	TN->RFI->getLocation (X, Y);

	// The packet type
	TP = PKT_TYPE_HELLO;

	// No formal payload
	ILength = 0;
	TLength = PKT_LENGTH_HELLO;

	// MAC-level sender = "transport" sender = US
	DCFP_S = Sender = TN->getId ();

	// No explicit transport receiver
	Receiver = NONE;
	// For the record
	DCFP_R = NONE;

	// Serial nmber
	SN = TN->Serial++;

	// Time to live
	TTL = PKT_TTL_HELLO;

	// Flags
	DCFP_Flags = (1 << DCFP_FLAG_BCST);
}

void Node::dropped (WDPacket *P) {

// Called for every packet dropped by the Xmitter because of retransmission
// number limit (see UPPER_fai_data below)

	Long NE;

	if (P->Retries >= MAX_ROUTE_TRIES) {
		// We have tried to reroute it several times, and it has
		// failed: drop it
#if TRACE_ROUTES
		trace ("PACKET DROPPED by RFM and by Node: %s", pkt_dump (P));
#endif
		delete P;
		return;
	}
#if TRACE_ROUTES
	trace ("PACKET FAILED, try %1d: %s", P->Retries, pkt_dump (P));
#endif

	// Increment retry counter
	P->Retries++;

	// Mark the neighbor as unreliable
	Neighbors->unreliable (P->DCFP_R);

	// And try again
	if ((NE = route ((Long)(P->Receiver))) == NONE)
		return;

	// New MAC-level recipient
	P->DCFP_R = NE;

#if TRACE_ROUTES
	trace ("PACKET REROUTED VIA %1d", NE);
#endif

	if (RFM->send (P)) {
		// Packet queued for transmission, as for "receive"
#if TRACE_ROUTES
		trace ("PACKET QUEUED for %1d re-forwarding", P->Retries);
#endif
		NOP;
	} else {
#if TRACE_ROUTES
		trace ("QUEUE FULL, packet not re-forwarded %1d", P->Retries);
#endif
		// Must delete explicitly
		delete P;
	}
}

void Node::sent (WDPacket *P, int rets, int retl) {

// Called for every packet successfully sent by the Xmitter (see UPPER_suc_data
// below)

#if TRACE_PACKETS
	trace ("PACKET SENT [NRetr = %1d,%1d]: %s", rets, retl, pkt_dump (P));
#endif
	delete P;
}

HelloSender::perform {

// This process sends HELLO at some intervals

	HelloPacket *P;

	state Delay:

		// This is how we determine the interval (see the interval
		// method in types.h)
		Timer->wait (interval (), SendIt);

	state SendIt:

		// Ignore the packet if the output queue is full
		if (S->RFM->room ()) {
#if TRACE_PACKETS
			trace ("NEW HELLO");
#endif
			P = create HelloPacket ();
			S->RFM->send (P);
		} else {
#if TRACE_PACKETS
			trace ("NEW HELLO dropped, QUEUE FULL");
#endif
			NOP;
		}
		proceed Delay;
}

NeighborCleaner::perform {

// This simple process runs at regular Intervals and cleans the Neighbors
// pool of obsolete entries.

	RTE_i *pe;

	state Delay:

		Timer->wait (Interval, CleanIt);

	state CleanIt:

		S->Neighbors->deleteOld (ExpirationTime);
		proceed Delay;
}

DataSender::perform {

// Simplicity itself. This process waits for message (packet) arrivals from
// the Client and dispatches those packets out.

	state WMess:

		// One message == one packet
		if (Client->getPacket (S->Buffer, 8, 0, PKT_LENGTH_DATA)) {
			// Submit it for transmission 
			S->dispatch ();
#if TRACE_PACKETS
			trace ("RELEASED: %s", pkt_dump (&(S->Buffer)));
#endif
			Client->release (S->Buffer);
			proceed WMess;
		}

		Client->wait (ARRIVAL, WMess);
}

// ============================================================================

void MY_RFModule::UPPER_rcv_data (DCFPacket *p, double rssi) {

	TheNode -> receive ((WDPacket*)p, rssi);
};

void MY_RFModule::UPPER_suc_data (DCFPacket *p, int rets, int retl) {

	TheNode -> sent ((WDPacket*)p, rets, retl);
};

void MY_RFModule::UPPER_fai_data (DCFPacket *p, int sh, int ln) {

	TheNode -> dropped ((WDPacket*)p);
};

// ============================================================================

void Node::setup (RATE xr, Long Pre) {

// Compare the sequence of readIn's to the input data file

	double X, Y, xp, lbd, lbt, minb, maxb;
	Long pr, q, c;

	// Read the coordinates
	readIn (X);
	readIn (Y);

	// Transceiver parameters
	readIn (xp);	// Xmit power in dBm

	RFI = create Transceiver (xr, Pre, dBToLin (xp), 1.0, X, Y);

	Ether->connect (RFI);

	// Minimum and maximum Hello interval
	readIn (minb);
	readIn (maxb);

	create HelloSender (minb, maxb);

	// Neighbor cleanup interval + expiration time
	readIn (minb);
	readIn (maxb);

	create NeighborCleaner (minb, maxb);

	// Packet queue size
	readIn (q);
	// Packet cache size
	readIn (c);

	RFM = new MY_RFModule (RFI, q);

	Neighbors = new NTable;
	NetMap = new RTable;

	PC = new PCache (c);

	Serial = 0;
	// The high-level transmitter
	create DataSender ();
}

void initNodes (Long N, Long P) {

	double  d;
	Long n, nret;
	RATE XmitRate;

	// Number of ITUs in one second
	d = (double) etuToItu (1.0);
	// Number of ITUs per bit
	XmitRate = (RATE) round (d / BitRate);

	print (form ("Bit rate (same for all nodes): %1d bps\n\n", BitRate));

	for (n = 0; n < N; n++)
		create Node (XmitRate, P);

	// Global parameters for transceivers
}

// ============================================================================

PositionReporter::perform {

// This process runs every virtual minute and dumps node positions to the
// output file. It only runs when there is at least one mobile node.

	double X, Y;
	Long n;

	state Report:

		print (form ("Node positions at time %1.2f seconds:\n",
			ituToEtu (Time)));
		for (n = 0; n < NStations; n++) {
			((Node*)(idToStation (n)))->RFI->getLocation (X, Y);
			print (form ("  %3d: [%8.2f, %8.2f]\n", n, X, Y));
		}
		print ("\n");

		Timer->delay (60.0, Report);
}

void initMobility () {

// Set up random waypoint mobility for selected nodes (see the data file)

	double X0, Y0, X1, Y1, Mns, Mxs, Mnp, Mxp;
	Long NN;
	int i, nn;

	// The number of mobile nodes
	readIn (nn);

	if (nn) {
		for (i = 0; i < nn; i++) {

			// The way the model works (Examples/IncLib/rwpmm.cc)
			// is that the mobility process of a node is described
			// by 9 floating point numbers. The first four specify
			// the bounding rectangle, the next two the minimum
			// and maximum speed (in m/s), the next two the minimum
			// and maximum pause time (in s), and the last one
			// the total duration of the mobility episode (in s).
			// If the duration is < 0, the apisode lasts forever.

			readIn (NN);		// Node number

			readIn (X0);
			readIn (Y0);		// The bounding rectangle
			readIn (X1);
			readIn (Y1);

			readIn (Mns);		// Minimum speed (m/s)
			readIn (Mxs);		// Maximum speed

			readIn (Mnp);		// Minimum pause (s)
			readIn (Mxp);		// Maximum pause (s)

			rwpmmStart (NN, ((Node*)idToStation(NN))->RFI,
				X0, Y0, X1, Y1,
				Mns, Mxs,
				Mnp, Mxp,
				-1.0		// Move forever
			);
		}

		// Start a process to report node positions every minute

		create PositionReporter ();
	}
}
