/* -------- */
/* ETHERNET */
/* -------- */

identify Ethernet;

long    MinPL = 368,    // Minimum packet length in bits (frame excluded)
	MaxPL = 12000,  // Maximum packet length in bits (frame excluded)
	FrameL = 208,   // The combined length of packet header and trailer
	PSpace = 96,    // Inter-packet space in bits
	JamL = 32,      // Length of the jamming signal in bits
	RTrip = 512;    // Maximum round-trip delay in bits (for backoff)

long    TGran = 1000;   // Time granularity: number of ITUs in a bit

station Node {

	Packet  Buffer; // One packet buffer per station
	Port    *Bus;   // The single port to the bus

	void    setup () {
		Bus = create Port (TGran);
	};
};

process Xmitter (Node) {

	int     CCounter;       // Collision counter
	Port    *Bus;           // A copy of the bus port
	Packet  *Buffer;        // Packet buffer pointer
	TIME    PSpace, JamL,   // Time versions of bit values
		RTrip;

	TIME    backoff () {    // This is the standard backoff function
		return (RTrip * lRndUniform (0.0, pow (2.0,
		    (double) (CCounter > 10 ? 10 : CCounter))));
	};

	void    setup () {
		Bus = S->Bus;
		Buffer = &(S->Buffer);
		// Convert from bits to ITUs
		PSpace = Bus->bitsToTime (::PSpace);
		JamL = Bus->bitsToTime (::JamL);
		RTrip = Bus->bitsToTime (::RTrip);
	};

	states {NPacket, Retry, Xmit, XDone, XAbort, JDone};

	perform {

	    TIME  LSTime, IPeriod;

		state NPacket:
		  if (Client->getPacket (Buffer, MinPL, MaxPL, FrameL)) {
		    CCounter = 0;
		    proceed (Retry);
		  }
		  // No packet to transmit
		  Client->wait (ARRIVAL, NPacket);
		state Retry:
		  if (undef (LSTime = Bus->lastEOA ())) {
		    // The bus, as perceived by the station, is busy
		    Bus->wait (SILENCE, Retry);
		    sleep;
		  }
		  // Check if the packet space has been obeyed
		  if ((IPeriod = Time - LSTime) >= PSpace) proceed (Xmit);
		  // Space too short: wait for the remainder
		  Timer->wait (PSpace - IPeriod, Xmit);
		state Xmit:
		  Bus->transmit (Buffer, XDone);
		  Bus->wait (COLLISION, XAbort);
		state XDone:
		  // Successfull transfer
		  Bus->stop ();
		  Buffer->release ();
		  proceed (NPacket);
		state XAbort:
		  // A collision
		  Bus->abort ();
		  CCounter++;
		  // Send a jamming signal
		  Bus->sendJam (JamL, JDone);
		state JDone:
		  Bus->stop ();
		  Timer->wait (backoff (), Retry);
	};
};

process Receiver (Node) {

	Port    *Bus;           // A copy of the bus port

	void    setup () {
		Bus = S->Bus;
	};

	states {WPacket, Rcvd};

	perform {

		state WPacket:
		  Bus->wait (EMP, Rcvd);
		state Rcvd:
		  assert (ThePacket->isMy (), "Receiver: not my packet");
		  Client->receive (ThePacket, ThePort);
		  skipto (WPacket);
	};
};

process Root {

	Link    *Bus;
	Traffic *TP;

	states {Start, Stop};

	perform {

	    long        BusLength;
	    int         NNodes, i, j;
	    DISTANCE    d;
	    double      mit, mle;

		state Start:
		  setEtu (TGran);
		  setTolerance (0.0001);
		  setLimit (2000);

		  // Initialize topology
		  readIn (NNodes);
		  readIn (BusLength);

		  // Create the bus
		  Bus = create Link (NNodes, (PSpace + 10) * TGran);
		  for (i = 0; i < NNodes; i++) {
		    // Create the stations and connect them to the bus
		    create Node;
		    ((Node*)TheStation)->Bus->connect (Bus);
		  }

		  // Define distances
		  d = (BusLength * TGran) / (NNodes - 1);
		  for (i = 0; i < NNodes-1; i++)
		    for (j = i+1; j < NNodes; j++)
		      setD (((Node*)idToStation(i))->Bus,
			((Node*)idToStation(j))->Bus, d * (j - i));

		  // Traffic
		  readIn (mit);
		  readIn (mle);
		  TP = create Traffic (MIT_exp+MLE_exp, mit, mle);
		  TP->addSender ();
		  TP->addReceiver ();

		  // Processes
		  for (i = 0; i < NNodes; i++) {
		    TheStation = idToStation (i);
		    create Xmitter;
		    create Receiver;
		  }
		  Kernel->wait (DEATH, Stop);
		state Stop:
		  System->printTop ("Network topology");
		  Client->printDef ("Traffic parameters");
		  Client->printPfm ();
		  Bus->printPfm ();
	};
};
