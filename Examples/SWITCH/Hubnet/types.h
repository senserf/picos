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

#include "hub.h"
#include "utraffic.h"

extern Long MinPL, MaxPL, FrameL;   // Packetization parameters
extern TIME SndRecTime;             // Sender recognition delay

station HStation : HubInterface, ClientInterface {
  Mailbox *StartEW, *ACK, *NACK;
  void setup () {
    HubInterface::configure ();
    ClientInterface::configure ();
    StartEW = create Mailbox (1);
    ACK = create Mailbox (1);
    NACK = create Mailbox (1);
  };
};

station Hub : HubStation {
  Boolean Busy;
  void setup () {
    HubStation::configure ();
    Busy = NO;
  };
};

process HubProcess (Hub) {
  Port *SPort, *BPort;
  void setup (int sn) {
    BPort = S->BPort;
    SPort = S->SPorts [sn];
  };
  states {Wait, NewPacket, Done};
  perform;
};

process Transmitter (HStation) {
  Port *SPort;
  Packet *Buffer;
  Mailbox *StartEW, *ACK, *NACK;
  void setup () {
    SPort = S->SPort;
    Buffer = &(S->Buffer);
    StartEW = S->StartEW;
    ACK = S->ACK;
    NACK = S->NACK;
  };
  states {NewPacket, Retransmit, Done, Confirmed, Lost};
  perform;
};

process Receiver (HStation) {
  Port *BPort;
  void setup () {
    BPort = S->BPort;
  };
  states {Wait, NewPacket};
  perform;
};

process AClock;

process Supervisor (HStation) {
  Port *BPort;
  Mailbox *StartEW, *ACK, *NACK;
  Packet *Pkt;
  TIME EchoTimeout;
  AClock *AC;
  void setup () {
    BPort = S->BPort;
    StartEW = S->StartEW;
    ACK = S->ACK;
    NACK = S->NACK;
    // Calculate echo timeout
    EchoTimeout = 
	(duToItu (BPort->distTo (((Hub*)idToStation (NStations-1))->BPort)) +
      		SndRecTime) * 2;
  };
  states {WaitSignal, WaitEcho, Waiting, NewPacket, CheckEcho, NoEcho};
  perform;
};
      
process AClock (HStation, Supervisor) {
  TIME Delay;
  void setup (TIME d) { Delay = d; };
  states {Start, GoOff};
  perform;
};
