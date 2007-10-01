#include "types.h"

// Include the Random WayPoint Mobility Model from the library
#include "rwpmm.cc"

// This counts all received packets over all nodes. Pretty much pointless, but
// illustrates how to collect relevant statistics at the end of experiment.
static Long CNT_Received_Packets = 0;

void printStatistics () {

	print (CNT_Received_Packets, "Total number of received packets:");
}

void Node::receive (Packet *P, double rssi) {

// Called when a packet is received by the node. The present action is
// trivial - we just count all received packets globally. Note that we do
// not Client->receive those packets. That would be pointless as the same
// packet would be received multiple times (note that the standard performance
// measures like delays, throughput are completely meaningless here).

	CNT_Received_Packets++;
}

Dispatcher::perform {

// This process waits for message (packet) arrivals from the Client and
// dispatches them out

	state WMess:
	
		// One message == one packet: assuming traffic parameters are
		// decent
		if (Client->getPacket (S->Buffer)) {
			if (!S->RFM->send (&(S->Buffer)))
				// This shouldn't happen because we wait with
				// the next packet until the previous one is
				// gone
				excptn ("MAC-level packet queue not empty");
			// Now wait until the packet has been transmitted
			Monitor->wait (EVENT_PACKET_TRANSMITTED, PDone);
			sleep;
		}

		// Wait for packet arrival
		Client->wait (ARRIVAL, WMess);

	state PDone:

		S->Buffer . release ();
		proceed WMess;
}

// ============================================================================

void MY_RFModule::UPPER_rcv (Packet *p, double rssi) {

	mac_trc ("RCV PACKET: %1d <- %1d", TheNode->getId (), p->Sender);
	TheNode -> receive (p, rssi);
}

// ============================================================================

void Node::setup ( RATE xr, Long Pre,	// These two are sort of standard
		   double xp,		// And this one two (transmit power)
		   double lbth,		// These four are for MY_RFModule
		   double lbtd,
		   double minb,
		   double maxb
		 ) {

	// Make sure the sequence of readIn's matches the values in the input
	// data file

	double X, Y;

	// Read the node's initial coordinates
	readIn (X);
	readIn (Y);

	// This is the standard set of parameters for a Transceiver
	RFI = create Transceiver (xr, Pre, dBToLin (xp), 1.0, X, Y);

	// Interface to the channel
	SEther->connect (RFI);

	// The transport-level transmitter
	create Dispatcher;

	// There is at most one outstanding packet for the MAC-transmitter
	RFM = new MY_RFModule (RFI, lbth, lbtd, minb, maxb);

	print (form ("  Node %4d at <%7.3f,%7.3f>\n", getId (), X, Y));
}

void initNodes (Long N, Long P) {

	double d, XP, LBTH, LBTD, MINB, MAXB;
	Long n;
	RATE XmitRate;
	Node *TN;
	Transceiver *t;

	// Number of ITUs in one second
	d = (double) etuToItu (1.0);
	// Number of ITUs per bit
	XmitRate = (RATE) round (d / SEther->BitRate);

	print ("Parameters shared by all nodes: \n\n");

	readIn (XP);		// Transmission power
	readIn (LBTH);		// LBT threshold
	readIn (LBTD);		// LBT delay
	readIn (MINB);		// Minimum backoff
	readIn (MAXB);		// Maximum backoff

	print (XP,  		"  Transmission power:", 10, 26);
	print (LBTH,  		"  LBT threshold:", 10, 26);
	print (LBTD,  		"  LBT delay:", 10, 26);
	print (MINB,  		"  Minimum backoff:", 10, 26);
	print (MAXB,  		"  Maximum backoff:", 10, 26);

	// Convert LBT threshold to linear
	LBTH = dBToLin (LBTH);

	for (n = 0; n < N; n++) {
		TN = create Node (XmitRate, P, XP, LBTH, LBTD, MINB, MAXB);
	}

	print ("\n");
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
	Long NN, ns, F, T;
	Boolean Used [NStations];

	readIn (ns);
	print (ns, "Number of mobility sets: ");

	for (NN = 0; NN < NStations; NN++)
		Used [NN] = NO;

	if (ns > 0)
		// To report node positions every minute (comment out if not
		// needed)
		create PositionReporter ();

	while (ns--) {

		readIn (F);	// From node
		readIn (T);	// ... to node

		readIn (X0);
		readIn (Y0);		// The bounding rectangle
		readIn (X1);
		readIn (Y1);

		readIn (Mns);		// Minimum speed (m/s)
		readIn (Mxs);		// Maximum speed

		readIn (Mnp);		// Minimum pause (s)
		readIn (Mxp);		// Maximum pause (s)

		while (F <= T) {

			if (Used [F])
				excptn ("Node %1d: duplicate mobility", F);

			Used [F] = YES;
		
			// The way the model works (Examples/IncLib/rwpmm.cc)
			// is that the mobility process of a node is described
			// by 9 floating point numbers. The first four specify
			// the bounding rectangle, the next two the minimum
			// and maximum speed (in m/s), the next two the minimum
			// and maximum pause time (in s), and the last one
			// the total duration of the mobility episode (in s).
			// If the duration is < 0, the "episode" lasts forever.

			rwpmmStart (F, ((Node*)idToStation(F))->RFI,
				X0, Y0, X1, Y1,
				Mns, Mxs,
				Mnp, Mxp,
				-1.0		// Move forever
			);
			F++;
		}

	}
}
