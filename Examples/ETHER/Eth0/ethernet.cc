identify Ethernet0;

#define MinPL      368   // Minimum payload length in bits (frame excluded)
#define MaxPL    12000   // Maximum payload length in bits (frame excluded)
#define FrameL     208   // The combined length of packet header and trailer

#define PSpace      96   // Inter-packet space length in bits
#define JamL        32   // Length of the jamming signal in bits
#define TwoL       512   // Maximum round-trip delay in bits

#define TRate        1   // Transmission rate: 1 ITU per bit

station EtherStation { 
  Port *Bus;
  Packet Buffer;
  void setup () { Bus = create Port (TRate); };
};

process Transmitter (EtherStation) {
  TIME IdleTime;
  int CCounter;       // Collision counter
  Port *Bus;          // A copy of the bus port
  Packet *Buffer;     // Packet buffer pointer
  TIME backoff ();    // The standard backoff function
  Boolean ready ();   // Packet acquisition
  void setup () {
    Bus = S->Bus;
    Buffer = &(S->Buffer);
    IdleTime = TIME_0;
  };
  states {Loop, Idle, Space, Xmit, Done, Coll, Abort};
  perform;
};

process Receiver (EtherStation) {
  Port *Bus;       // A copy of the bus port
  void setup () { Bus = S->Bus; };
  states {WPacket, Rcvd};
  perform;
};

TIME Transmitter::backoff () {
  return (TIME) (TwoL * toss (1 << (CCounter > 10 ? 10 : CCounter)));
}

Boolean Transmitter::ready () {
  if (!Buffer->isFull ()) {
    if (Client->getPacket (Buffer, MinPL, MaxPL, FrameL)) {
      CCounter = 0;
      return YES;
    } else
      return NO;
  }
  return YES;
}

Transmitter:: perform {

  state Loop:

    if (Bus->busy ()) {
	Bus->wait (SILENCE, Idle);
	sleep;
    }

  transient Idle:

    IdleTime = Time;

  transient Space:

    if (!ready ()) {
      Bus->wait (ACTIVITY, Loop);
      Client->wait (ARRIVAL, Space);
      sleep;
    }

    // We have a packet to transmit
    if (Time - IdleTime < PSpace) {
      // Have to obey packet spacing
      Timer->wait (PSpace - (Time - IdleTime), Xmit);
      sleep;
    }

  transient Xmit:

    Bus->transmit (Buffer, Done);
    Bus->wait (COLLISION, Coll);

  state Done:

    // Successfull transmission
    Bus->stop ();
    Buffer->release ();
    proceed Idle;

  state Coll:
    // A collision: increment the counter
    CCounter++;
    // And keep going for a while longer
    Timer->wait (JamL, Abort);

  state Abort:

    Bus->abort ();
    Timer->wait (backoff (), Loop);
};

Receiver::perform {
  state WPacket:
    Bus->wait (EMP, Rcvd);
  state Rcvd:
    assert (ThePacket->isMy (), "Receiver: not my packet");
    Client->receive (ThePacket, ThePort);
    skipto (WPacket);
};

process Root {
  void buildNetwork (int, TIME);
  void initTraffic ();
  states {Start, Stop};
  perform {
    int n;
    Long NMessages;
    TIME BusLength;
    state Start:
      setEtu (1);
      setTolerance (0.0001);
      // Configuration parameters
      readIn (n);
      readIn (BusLength);
      // Build the network
      buildNetwork (n, BusLength);
      // Traffic
      initTraffic ();
      // Processes
      for (n = 0; n < NStations; n++) {
        create (n) Transmitter;
        create (n) Receiver;
      }
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
  };
};

void Root::buildNetwork (int ns, TIME bl) {
  int i, j;
  Link *lk;
  EtherStation *s;
  DISTANCE d;
  d = bl / (ns - 1);  // Distance between neighboring stations
  for (i = 0; i < ns; i++) create EtherStation;
  lk = create Link (ns);
  for (i = 0; i < ns; i++) {
    s = (EtherStation*) idToStation (i);
    s->Bus->connect (lk);
    for (j = 0; j < i; j++)
      ((EtherStation*) idToStation (j))->Bus->setDTo (s->Bus, d * (i - j));
  }
};

void Root::initTraffic () {
  Traffic *tp;
  double mit, mle;
  readIn (mit);           // Mean message interarrival time
  readIn (mle);           // Mean message length
  tp = create Traffic (MIT_exp+MLE_exp, mit, mle);
  tp->addSender (ALL);    // All stations
  tp->addReceiver (ALL);
};
