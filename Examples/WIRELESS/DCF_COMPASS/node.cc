#include "types.h"
#include "rwpmm.cc"

static char *pkt_dump (WDPacket *p) {

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

	for_pool (re, Head)
		if (re->A == N)
			break;

	if (re) {
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
	re->TStamp = Time;
}

void NTable::update (Long N, double X, double Y, double rssi) {

// This one updates the neighbor table

	NTE_i *re;

	for_pool (re, Head)
		if (re->A == N)
			break;

	if (re) {
		if (re->X == X && re->Y == Y) {
			// Location unchanged: average RSSI
			if (rssi >= 0.0)
				re->RSSI = ema (re->RSSI, rssi, 0.25);
			if ((re->Reliability += RELIABILITY_INC) >
			    RELIABILITY_MAX)
				re->Reliability = RELIABILITY_MAX;
#if TRACE_ROUTES
			trace ("NEIGHBOR table, location unchanged, rssi = %g"
				", rel = %g",
					linTodB (re->RSSI), re->Reliability);
#endif
		} else {
			// Location changed, RSSI starts from scratch
			re->RSSI = rssi;
			re->Reliability = RELIABILITY_MAX;
#if TRACE_ROUTES
			trace ("NEIGHBOR table, location for %1d changed: "
				"[%f,%f], rssi = %g", N, X, Y,
					linTodB (re->RSSI));
#endif
		}
		re->X = X;
		re->Y = Y;
		re->TStamp = Time;
	} else {
		re = new NTE_i;
		re->A = N;
		re->X = X;
		re->Y = Y;
		re->RSSI = rssi;
		re->Reliability = RELIABILITY_MAX;
		pool_in (re, Head);
#if TRACE_ROUTES
		trace ("NEIGHBOR table, new entry: <%1d,%f,%f>, rssi = %g",
			N, X, Y, linTodB (re->RSSI));
#endif
	}
	re->TStamp = Time;
}

void NTable::unreliable (Long n) {

	NTE_i *re;

	for_pool (re, Head) {
		if (re->A == n) {
			re->Reliability = RELIABILITY_MIN;
#if TRACE_ROUTES
			trace ("NEIGHBOR table, %1d marked as unreliable", n);
#endif
			return;
		}
	}
}

Long NTable::route (Long d, double cd, double X, double Y) {

// This method is called to locate in the routing table a node which is
// either d, or whose distance to (X,Y) is the smallest and less than cd.

	double CD, MD, DD;
	Long N;
	NTE_i *re;

	N = NONE;
	MD = HUGE;

	for_pool (re, Head) {
		if (linTodB (re->RSSI) < -84.0)
			continue;
		if (re->A == d) {
#if TRACE_ROUTES
			trace ("ROUTE to %1d <%f> [IMMED] %g", d, cd,
				linTodB (re->RSSI));
#endif
			return d;
		}
		DD = dist (re->X, re->Y, X, Y);

		if (DD + TINY_DISTANCE >= cd)
			continue;

		if (DD < MD) {
			MD = DD;
			N = re->A;
		}
	}
#if TRACE_ROUTES
	if (N == NONE) 
		trace ("ROUTE to %1d [%f], NOT FOUND, packet dropped",
			d, cd);
	else
		trace ("ROUTE to %1d [%f], via %1d [%f]", d, cd, N, MD);
#endif
	return N;

#if 0

	double CD, MD, DD, RT;

	Long N;
	NTE_i *re;
	N = NONE;	// The best neighbor so far
	MD = HUGE;


	// Try reliable routes first
	for (RT = RELIABILITY_THS; RT >= RELIABILITY_ACC; RT -=
	    RELIABILITY_STP) {

		for_pool (re, Head) {

			if (re->Reliability < RT)
				continue;

			if (re->A == d) {
				DD = 0.0;
			} else {
				DD = dist (re->X, re->Y, X, Y);
				if (DD + TINY_DISTANCE >= cd) {
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
			if (cd == HUGE)
				cd = 0.0;
			trace ("ROUTE to %1d [%f], via %1d [%f] %g", d, cd, N,
				MD, RT);
#endif
			return N;
		}
	}

#if TRACE_ROUTES
	if (cd == HUGE)
		cd = 0.0;
	trace ("ROUTE to %1d [%f], NOT FOUND, packet dropped, %g", d, cd, RT);
#endif
	return NONE;

#endif	/* 0 */


}

void NTable::deleteOld (TIME et) {

// Deletes obsolete entries from the table. We only run it for the neighbor
// pool to remove those entries that have not been updated for a time longer
// than the threshold (specified as the argument).

	NTE_i *pe;

#if TRACE_ROUTES
	for_pool (pe, Head) {
		if (pe->TStamp + et < Time)
			trace ("NEIGHBOR table, entry for %1d timed out",
				pe->A);
	}
#endif
	// This operation scans the pool and removes from it all items that
	// fulfill the indicated condition.
	trim_pool (pe, Head, pe->TStamp + et < Time);
}
		
void Node::receive (WDPacket *P, double rssi) {

// This method determines what to do with a received packet

	double XO, YO, XD, YD, DA, TA;
	Long NE;

#if TRACE_PACKETS
	trace ("ARRIVED: %s, RSSI = %gdBm", pkt_dump (P), linTodB (rssi));
#endif
	if (P->Sender == getId ()) {
		// This packet was sent by you - ignore it no matter
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
		// This is a data packet: check if addressed to you
		if (P->isMy ()) {
			// This is SMURPH's way to check if you are the
			// "transport" recipient of the packet. You are, so
			// pass the packet to "higher layers". This is done
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
		// The packet has made its one last hop
#if TRACE_PACKETS
		trace ("TTL LIMIT, packet dropped");
#endif
		return;
	}

	// Note that you are not allowed to modify P, as this is the "Ether"
	// copy that is usually seen by othe nodes. Thus, for example, I
	// didn't decrement the TTL yet. We can only do it when we create
	// a new copy of the packet to be retransmitted. We want to postpone
	// that until we know for a fact that the packet will actually be
	// retransmitted.

	if (P->TP != PKT_TYPE_HELLO) {

		// It is a data packet, and it wasn't addressed to us. Thus,
		// we are supposed to forward it towards the destination. Our
		// naive idea is to select the single neighbor being closest
		// to the destination. We drop the packet if nobody is closer
		// than us, and the destination is not our neighbor

		if (!NetMap->getCoords (P->Receiver, XD, YD)) {
			// Sorry, we don't know about the destination, drop
			// the packet
#if TRACE_ROUTES
			trace ("NO ROUTE, packet dropped");
#endif
			return;
		}

		// Our own coordinates
		RFI->getLocation (XO, YO);
		DA = dist (XO, YO, XD, YD);

		if ((NE = Neighbors->route ((Long)(P->Receiver), DA, XD, YD)) ==
		    NONE)
			// Sorry, no way. This means we have no neighbors.
			return;

		// Now is the time to create a copy of the packet. You must
		// call the clone method as the operation is a bit trickier
		// than it seems at first sight.
		P = (WDPacket*) (P->clone ());
		// And now we can decrement the TTL
		--(P->TTL);
		// Initialize the number of routing attempts
		P->Retries = 0;

		// New MAC sender and next-hop node
		P->DCFP_R = NE;
		P->DCFP_S = getId ();
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
		// is done
#if TRACE_PACKETS
		trace ("PACKET QUEUED for forwarding");
#endif
		NOP;
	} else {
#if TRACE_PACKETS
		trace ("QUEUE FULL, packet dropped");
#endif
		// Must delete explicitly
		delete P;
	}
}

void Node::dispatch () {

// This one is called to send a data packet that originates at this node

	double XO, YO, XD, YD, DA, TA;
	Long D, NE;
	WDPacket *P;

#if TRACE_PACKETS
	trace ("DISPATCH: %s", pkt_dump (&Buffer));
#endif

	// This is essentially the same as for routing (in receive). A bit
	// simpler because we know that this is is a data packet and that it
	// just begins its life.

	D = Buffer . Receiver;

	if (!NetMap->getCoords (D, XD, YD)) {
#if TRACE_RUTES
		trace ("NO ROUTE, packet dropped");
#endif
		return;
	}

	RFI->getLocation (XO, YO);
	DA = dist (XO, YO, XD, YD);

	if ((NE = Neighbors->route (D, DA, XD, YD)) == NONE) {
		// Sorry, no way
		return;
	}

	P = (WDPacket*) (Buffer.clone ());

	P -> DCFP_R = NE;

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

	Node *TN = (Node*)TheStation;

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
	DCFP_Type = DCFP_TYPE_DATA;
}

void Node::dropped (WDPacket *P) {

// Called for every packet dropped by the Xmitter

	double DA, XO, YO, XD, YD;
	Long NE;

	if (P->Retries >= MAX_ROUTE_TRIES) {
#if TRACE_PACKETS
		trace ("PACKET DROPPED by RFM and by Node: %s", pkt_dump (P));
#endif
		delete P;
		return;
	}
	P->Retries++;
	Neighbors->unreliable (P->DCFP_R);

	// Re-route
	RFI->getLocation (XO, YO);
	DA = dist (XO, YO, XD, YD);

	if ((NE = Neighbors->route ((Long)(P->Receiver), DA, XD, YD)) == NONE)
		return;

	P->DCFP_R = NE;

	if (RFM->send (P)) {
		// Packet queued for transmission, we shall learn when it
		// is done
#if TRACE_PACKETS
		trace ("PACKET QUEUED for %1d re-forwarding", P->Retries);
#endif
		NOP;
	} else {
#if TRACE_PACKETS
		trace ("QUEUE FULL, packet not re-forwarded %1d", P->Retries);
#endif
		// Must delete explicitly
		delete P;
	}
}

void Node::sent (WDPacket *P, Long nret) {

// Called for every packet actually sent by the Xmitter

#if TRACE_PACKETS
	trace ("PACKET SENT [NRetr = %1d]: %s", nret, pkt_dump (P));
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

void MY_RFModule::rcv_data (DCFPacket *p, double rssi) {

	((Node*)TheStation)->receive ((WDPacket*)p, rssi);
};

void MY_RFModule::suc_data (DCFPacket *p, Long retr) {

	((Node*)TheStation) -> sent ((WDPacket*)p, retr);
};

void MY_RFModule::fai_data (DCFPacket *p) {

	((Node*)TheStation) -> dropped ((WDPacket*)p);
};

void MY_RFModule::col_data () {

	trace ("DATA PACKET LOST");
};

// ============================================================================

void Node::setup (RATE xr, Long nr) {

// Compare the sequence of readIn's to the input data file

	double X, Y, xp, lbd, lbt, minb, maxb;
	Long pr, q, c;

	// Read the coordinates
	readIn (X);
	readIn (Y);

	// Transceiver parameters
	readIn (xp);	// Xmit power in dBm
	readIn (pr);	// Preamble length in physical bits

	RFI = create Transceiver (xr, pr, dBToLin (xp), 1.0, X, Y);

	SEther->connect (RFI);

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

	RFM = new MY_RFModule (RFI, q, nr);

	Neighbors = new NTable;
	NetMap = new RTable;

	PC = new PCache (c);

	Serial = 0;
	// The high-level transmitter
	create DataSender ();
}

void initNodes (Long N) {

	double  d;
	Long n, nret;
	RATE XmitRate;

	// Number of ITUs in one second
	d = (double) etuToItu (1.0);
	// Number of ITUs per bit
	XmitRate = (RATE) round (d / SEther->BitRate);

	// Number of retransmissions
	readIn (nret);

	for (n = 0; n < N; n++)
		create Node (XmitRate, nret);

	// Global parameters for transceivers

	// Minimum distance
	SEther->setMinDistance (SEther->RDist);
	// We need this for ANYEVENT
	SEther->setAevMode (NO);
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
