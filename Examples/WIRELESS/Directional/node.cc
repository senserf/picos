#include "types.h"
#include "rfmodule.h"

#include "rwpmm.cc"

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
		re->X = X;
		re->Y = Y;
		re->TStamp = Time;
	} else {
		re = new RTE_i;
		re->A = N;
		re->X = X;
		re->Y = Y;
		re->TStamp = Time;
		pool_in (re, Head);
	}
}

Long RTable::route (double X, double Y, double a, double &ta) {

// This method is called to locate in the routing table a node with the
// property that its angle from location (X,Y) is the closest to the
// specified angle a. It returns the node number (as the value) and the
// angle to that node via ta.

	double CA, MA, TA, b, d;

	Long N;

	RTE_i *re;

	N = NONE;	// The best neighbor so far
	MA = 10.0;	// Current minimum angle difference (never more than PI)

	for_pool (re, Head) {
		// Angle to this node
		b = angle (X, Y, re->X, re->Y);
		d = adiff (a, b);
		if (d < MA) {
			MA = d;
			N = re->A;
			TA = b;
		}
	}

	ta = TA;
	return N;
}

void RTable::deleteOld (TIME et) {

// Deletes obsolete entries from the table. We only run it for the neighbor
// pool to remove those entries that have not been updated for a time longer
// than the threshold (specified as the argument).

	RTE_i *pe;

	// This operation scans the pool and removes from it all items that
	// fulfill the indicated condition.
	trim_pool (pe, Head, pe->TStamp + et < Time);
}
		
void Node::receive (WDPacket *P) {

// This method determines what to do with a received packet

	double XO, YO, XD, YD, DA, TA;
	Long NE;

	// Tracing macros, normally disabled and expanded into nothing
	trc_h (P, "ARRIVAL"); trc_d (P, "ARRIVAL");
	
	if (P->Sender == getId ()) {
		// This packet was sent by you - ignore it no matter
		// what
		trc_h (P, "OWN"); trc_d (P, "OWN");
		return;
	}

	// Duplicate check
	if (PC->add (P)) {
		// The packet has been seen recently - drop it
		trc_h (P, "DUPLICATE"); trc_d (P, "DUPLICATE");
		return;
	}

	if (P->TP == PKT_TYPE_HELLO) {
		// This is a HELLO packet: update your tables
		if (P->Sender == P->SA)
			// This means that the packet was sent by a neighbor
			// (its MAC-level sender is the same as the "transport"
			// sender). Add the coordinates to the Neighbor Pool.
			Neighbors->update (P->Sender,
					((HelloPacket*)P)->X,
					((HelloPacket*)P)->Y);

		// Always update the global routing table. It stores the
		// coordinates of all nodes in the network.
		NetMap->update (P->Sender,
				((HelloPacket*)P)->X,
				((HelloPacket*)P)->Y);
		trc_t (Neighbors, "NEIGHBORS");
		trc_t (NetMap, "NETWORK");
	} else {
		// This is a data packet: check if addressed to you
		if (P->isMy ()) {
			// This is SMURPH's way to check if you are the
			// "transport" recipient of the packet. You are, so
			// pass the packet to "higher layers". This is done
			// by the (standard) receive method of Client.
			trc_d (P, "RECEIVED");
			Client->receive (P, RFI);
			return;
		}
	}

	// Check if should retransmit the packet
	if (P->TTL <= 1) {
		// The packet has made its one hop
		trc_h (P, "TTL LIMIT"); trc_d (P, "TTL LIMIT");
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
		// naive idea is to select the single neighbor whose angle
		// from us is the closest to the angle towards the destination
		// and forward the packet to that neighbor while setting the
		// antenna to point exactly at the neighbor.

		// Get the destination coordinates
		if (!NetMap->getCoords (P->Receiver, XD, YD)) {
			// Sorry, we don't know about the destination, drop
			// the packet
			trc_d (P, "DEST UNKNOWN");
			trc_t (NetMap, "NETWORK, DEST UNKNOWN");
			return;
		}

		trc_d (P, "ROUTING");

		// Our own coordinates
		RFI->getLocation (XO, YO);

		// Angle from us to the destination
		DA = angle (XO, YO, XD, YD);

		trc_a (DA, "DESTINATION");

		if ((NE = Neighbors->route (XO, YO, DA, TA)) == NONE) {
			// Sorry, no way. This means we have no neighbors.
			trc_d (P, "NO NEIGHBOR");
			trc_t (Neighbors, "NO NEIGHBOR");
			return;
		}

		trc_r (NE, " -> ");
		trc_a (TA, "TARGET");

		// Now is the time to create a copy of the packet. You must
		// call the clone method as the operation is a bit trickier
		// than it seems at first sight.
		P = (WDPacket*) (P->clone ());
		// And now we can decrement the TTL
		--(P->TTL);

		// New direction. We set it to the angle to the selected
		// Neighbor. Formally, Direction is a packet attribute, but
		// it is only for convenience (we don't pretend that it is
		// sent along with the packet). It will be used by the
		// transmitter to set the antenna direction before transmitting
		// this particular packet.
		P->Direction = (float) TA;
	} else {
		// Here we handle the case of a forwarded HELLO packet
		P = (WDPacket*) (P->clone ());
		// This is illegal value for an angle, which means
		// omnidirectional broadcast.
		P->Direction = -1.0;
		trc_h (P, "REBROADCAST");
		--(P->TTL);
	}

	// We are the new MAC-level sender
	P->SA = getId ();
	// Queue the packet for (re) transmission
	PQ->queue (P);
}

void Node::dispatch () {

// This one is called to send a data packet that originates at this node

	double XO, YO, XD, YD, DA, TA;
	Long NE;
	WDPacket *P;

	trc_d (&Buffer, "DISPATCH");

	// This is essentially the same as for routing (in receive). A bit
	// simpler because we know that this is is a data packet and that it
	// just begins its life.

	if (!NetMap->getCoords (Buffer.Receiver, XD, YD)) {
		trc_d (&Buffer, "DEST UNKNOWN");
		return;
	}

	// Our own coordinates
	RFI->getLocation (XO, YO);

	// Angle from us to the destination
	DA = angle (XO, YO, XD, YD);

	trc_a (DA, "DESTINATION");

	if ((NE = Neighbors->route (XO, YO, DA, TA)) == NONE) {
		// Sorry, no way
		trc_d (&Buffer, "NO NEIGHBOR");
		trc_t (Neighbors, "NO NEIGHBOR");
		return;
	}

	trc_r (NE, " -> ");
	trc_a (TA, "TARGET");

	P = (WDPacket*) (Buffer.clone ());

	// New direction
	P->Direction = (float) TA;
	PQ->queue (P);
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
	SA = Sender = TN->getId ();

	// No explicit transport receiver
	Receiver = NONE;

	// Serial nmber
	SN = TN->Serial++;

	// Time to live
	TTL = PKT_TTL_HELLO;

	// Broadcast (omni) antenna setting
	Direction = -1.0;
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
		if (S->PQ->free ()) {
			P = create HelloPacket ();
			S->PQ->put (P);
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
			Client->release (S->Buffer);
			proceed WMess;
		}

		Client->wait (ARRIVAL, WMess);
}

// ============================================================================

void Node::setup (RATE xr) {

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

	Event = create Barrier ();

	// LBT
	readIn (lbd);	// Delay in seconds
	readIn (lbt);	// Threshold in dBm

	// Backoff
	readIn (minb);	// In seconds
	readIn (maxb);

	RFBusy = NO;

	// Create the processes
	create Xmitter (lbd, lbt, minb, maxb);
	create Receiver ();

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

	PQ = create PQueue (q);

	Neighbors = new RTable;
	NetMap = new RTable;

	PC = new PCache (c);

	Serial = 0;
	// The high-level transmitter
	create DataSender ();
}

void initNodes (Long N) {

	double  d;
	Long n;
	RATE XmitRate;

	// Number of ITUs in one second
	d = (double) etuToItu (1.0);
	// Number of ITUs per bit
	XmitRate = (RATE) round (d / BitRate);

	print (form ("Bit rate (same for all nodes): %1d bps\n\n", BitRate));

	for (n = 0; n < N; n++)
		create Node (XmitRate);

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
