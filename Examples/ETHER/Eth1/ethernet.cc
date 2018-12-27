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
identify Ethernet1;

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
  int CCounter;       // Collision counter
  Port *Bus;          // A copy of the bus port
  Packet *Buffer;     // Packet buffer pointer
  TIME backoff ();    // The standard backoff function
  void setup () {
    Bus = S->Bus;
    Buffer = &(S->Buffer);
  };
  states {NPacket, Retry, Xmit, XDone, XAbort, JDone};
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
};

Transmitter:: perform {
  TIME  LSTime, IPeriod;
  state NPacket:
    CCounter = 0;
    if (Client->getPacket (Buffer, MinPL, MaxPL, FrameL))
      proceed Retry;
    else
      Client->wait (ARRIVAL, NPacket);
  state Retry:
    if (undef (LSTime = Bus->lastEOA ())) {
      // The bus, as perceived by the station, is busy
      Bus->wait (SILENCE, Retry);
    } else {
      // Check if the packet space has been obeyed
      if ((IPeriod = Time - LSTime) >= PSpace)
        proceed (Xmit);
      else
        // Space too short: wait for the remainder
        Timer->wait ((TIME) PSpace - IPeriod, Xmit);
    }
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
  lk = create Link (ns, PSpace+10);
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
