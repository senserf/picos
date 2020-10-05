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

/* ------ */
/* HUBNET */
/* ------ */

identify (Hubnet);

int     NUsers;                         // The number of (regular) stations

TIME    EchoTimeout,
		// The amount of time after which the station assumes that
		// its packet didn't make it through the hub, if the
		// packet's echo hasn't been heard in the meantime.

	HdrRecTime;
		// The amount of time required to recognize the sender
		// of a packet. Used by the transmitter to simulate the 
		// recognition of the echo of its own packet.

long    MinPL, MaxPL, FrameL;           // Packet parameters

	enum    {Idle, Busy};                    // Hub status

long    TRate;                          // Transmission rate (itus/bit)

/* -------------------- */
/* The Hub station type */
/* -------------------- */
station Hub {

  int   Status;
  Port  **SPorts,       // Selection ports
	*BPort;         // Broadcast port
  void  setup ();
};

void  Hub::setup () {
      SPorts = new Port* [NUsers];
      BPort = create Port (TRate);
      for (int i = 0; i < NUsers; i++) SPorts [i] = create Port;
      Status = Idle;
};

/* ------------------------------- */
/* The User (regular) station type */
/* ------------------------------- */
station User {

  Port   *SPort, *BPort;

  Mailbox *StartEW, *ACK, *NACK;

  Packet Buffer;

  void setup () {
      StartEW = create Mailbox (1);
      ACK = create Mailbox (1);
      NACK = create Mailbox (1);
      SPort = create Port (TRate);
      BPort = create Port;
  };
};

/* ---------------------------------------------------------- */
/* The  hub  process:  each  selection  port of the hub has a */
/* separate copy of this process                              */
/* ---------------------------------------------------------- */
process HubProcess (Hub) {

  Port  *SPort, *BPort;

  void setup (int pn) {
      SPort = S->SPorts [pn];
      BPort = S->BPort;
  };

  states {Wait, NewPacket, Done};
  perform;
};

HubProcess::perform {
    state Wait:                                 // Wait for a packet
      SPort->wait (BOT, NewPacket);
    state NewPacket:                            // A packet arrives
      if (S->Status == Busy) skipto (Wait);     // Hub busy, ignore
      S->Status = Busy;                         // Reserve the hub
      BPort->transmit (ThePacket, Done);
    state Done:                                 // Stop packet transmission
      BPort->stop ();
      S->Status = Idle;                         // Release the hub
      proceed (Wait);                           // Wait for more
};

/* ------------------------------------ */
/* The transmitter of a regular station */
/* ------------------------------------ */
process Xmitter (User) {

  Port    *SPort;
  Packet  *Buffer;

  void setup () {
      SPort  = S->SPort;
      Buffer = &(S->Buffer);
  };
  states {NewPacket, Retransmit, Done, Confirmed, Lost};
  perform;
};

Xmitter::perform {
    state NewPacket:                            // Attempt to acquire a packet
      if (Client->getPacket (Buffer, MinPL, MaxPL, FrameL))
	proceed (Retransmit);
      Client->wait (ARRIVAL, NewPacket);        // No ready packet
    state Retransmit:                           // Transmit or retransmit
      SPort->transmit (Buffer, Done);
      S->StartEW->put ();                       // Wait for packet echo
      S->NACK->wait (RECEIVE, Lost);
    state Done:
      SPort->stop ();                           // Terminate the packet
      S->NACK->wait (RECEIVE, Retransmit);      // Wait for a signal from
      S->ACK->wait (RECEIVE, Confirmed);        // monitor
    state Confirmed:                            // Release the packet
      Buffer->release ();
      proceed (NewPacket);                      // Take care of the next one
    state Lost:
      SPort->abort ();
      proceed (Retransmit);
};

/* --------------------------------- */
/* The receiver of a regular station */
/* --------------------------------- */
process Receiver (User) {

  Port  *BPort;

  void setup () {
	BPort = S->BPort;
  };

  states {Wait, NewPacket};
  perform;

};

Receiver::perform {
    state Wait:                                 // Wait for the end of packet
      BPort->wait (EMP, NewPacket);
    state NewPacket:                            // Accept the packet
      Client->receive (ThePacket, BPort);
      skipto (Wait);                            // and continue
};

process HMonitor;

/* -------------------------------------------------------- */
/* An auxiliary alarm clock used by the monitor (see below) */
/* -------------------------------------------------------- */
process Trumpet (User, HMonitor) {

  TIME  Delay;

  states {Start, Play};

  void setup (TIME t) {
    Delay = t;
  };
  perform;
};

/* ---------------------------------------------------------- */
/* Detects  the  echo  of  a packet sent by this station or a */
/* timeout                                                    */
/* ---------------------------------------------------------- */
process HMonitor (User) {

  Port          *BPort;
  Trumpet       *AClock;

  void setup () {
	BPort = S->BPort;
  };
  states {WaitSignal, WaitEcho, Waiting, NewPacket, Echo, NoEcho};
  perform;
};

Trumpet::perform {
    state Start:                                // After creation
      Timer->wait (Delay, Play);
    state Play:                                 // Play the trumpet
      F->signal ();
      terminate ();
};

HMonitor::perform {
    state WaitSignal:                           // Wait for a waking signal
      S->StartEW->wait (RECEIVE, WaitEcho);
    state WaitEcho:                             // Setup the alarm clock
      AClock = create Trumpet (EchoTimeout);
      proceed (Waiting);
    state Waiting:                              // Continue waiting for echo
      BPort->wait (BOT, NewPacket);
      TheProcess->wait (SIGNAL, NoEcho);
    state NewPacket:                            // Determine the packet sender
      if (ThePacket -> Sender == ident (S)) {
	// The  arrival of a packet sent by this station; wait until the
	// packet header (i.e. the sender) is formally recognized
	Timer->wait (HdrRecTime, Echo);
        TheProcess->wait (SIGNAL, NoEcho);
      } else
	skipto (Waiting);
    state Echo:                                 // Echo has been recognized
      if (erase () == 0)
	// No pending Signal from the Trumpet
	AClock -> terminate ();
      S->ACK->put ();
      proceed (WaitSignal);
    state NoEcho:                               // Timeout
      S->NACK->put ();
      proceed (WaitSignal);
};

/* ------------------ */
/* The "Root" process */
/* ------------------ */
process Root {

  states {Start, Exit};
  void  initTopology ();
  void  initTraffic ();
  void  initProtocol ();
  perform;
};

void    Root::initTopology () {

      long    LinkLength;
      Link    *slk, *blk;
      Hub     *h;
      User    *s1, *s2;
      int     i;

      readIn (NUsers);          // Number of regular stations
      readIn (LinkLength);      // The length of a link segment in itu
      readIn (TRate);           // The transmission rate in bits/itu

      // Create the broadcast link and the hub
      blk = create Link (NUsers+1);
      h = create Hub;
      // Connect the hub to the broadcast link
      h->BPort->connect (blk);

      // Create selection links (two ports each) and regular stations
      for (i = 0; i < NUsers; i++) {
	slk = create Link (2);
	create User;
	((User*)TheStation)->BPort->connect (blk);
	((User*)TheStation)->SPort->connect (slk);
	// Connect the hub to the selection link
	h->SPorts [i]->connect (slk);
      }

      // Set up distances
      for (i = 0; i < NUsers; i++) {
	// Note that regular stations are numbered from 1 to NUsers
	s1 = (User*) idToStation (i+1);
	// Regular station <-> hub
	s1->BPort->setDTo (h->BPort, LinkLength);
	s1->SPort->setDTo (h->SPorts [i], LinkLength);
	for (int j = i+1; j < NUsers; j++) {
	  // Regular station <-> regular station
	  s2 = (User*) idToStation (j+1);
	  s1->BPort->setDTo (s2->BPort, LinkLength+LinkLength);
	}
      }
};

void    Root::initTraffic () {

      Traffic  *tr;
      double   MeanMIT, MeanMLE;

      readIn (MinPL);                   // Minimum packet length
      readIn (MaxPL);                   // Maximum packet length
      readIn (FrameL);                  // Frame information (header+trailer)

      readIn (MeanMIT);                 // Mean message interarrival time
      readIn (MeanMLE);                 // Mean message length

      readIn (EchoTimeout);             // Echo waiting time
      readIn (HdrRecTime);              // Header recognition time

      // Create the traffic pattern
      tr = create Traffic (MIT_exp+MLE_exp, MeanMIT, MeanMLE);
      // Add regular stations as senders and receivers
      for (int i = 1; i <= NUsers; i++) {
	tr->addSender (i);
	tr->addReceiver (i);
      }
};

void    Root::initProtocol () {

      int i;

      for (i = 1; i <= NUsers; i++) {
	// User stations' processes
	create (i) Xmitter;
	create (i) Receiver;
	create (i) HMonitor;
      }

      TheStation = idToStation (0);
      // The Hub
      for (i = 0; i < NUsers; i++)
	create HubProcess, form ("Hub%03d", i+1), (i);
};

Root::perform {
    state Start:
      setEtu (1000);                    // Time unit = 1000 ITUs
      setLimit (1000);                  // Set limits
      initTopology ();
      initTraffic ();
      initProtocol ();
      // That's it, now wait until the simulation is over
      Kernel->wait (DEATH, Exit);
    state Exit:
      // Print out simulation results
      idToTraffic (0) -> printPfm ("Global Performance Measures");
      idToLink (0)-> printPfm ("Broadcast Link Performance Measures");
};
