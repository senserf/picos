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

#include "hbus.h"
#include "utraff2l.h"
#include "sprotocl.h"

extern Long SlotML,     // Slot marker length (in bits)
            SegmPL,     // Segment payload length
            SegmFL;     // The length of the segment header and trailer
            
extern TIME SegmWindow; // Segment window length in ITUs

#define SLOT NONE       // The type of the packet representing slot markers
#define BOC  PF_usr0    // The BOC flag in the slot marker
#define FULL PF_usr1    // The full/empty status of the slot
#define SNC  PF_usr2    // The SNC flag in the slot marker

station HStation : HBusInterface, ClientInterface {
  Mailbox *Strobe [2];
  void setup () {
    HBusInterface::configure ();
    ClientInterface::configure ();
    Strobe [LRBus] = create Mailbox (0);
    Strobe [RLBus] = create Mailbox (0);
  };
};

station HeadEnd : HStation {
  Packet SMarker;
  Mailbox *SendBOC, *SendSNC;
  void setup () {
    HStation::setup ();
    SMarker.fill (NONE, NONE, SlotML);
    SendBOC = create Mailbox (1);
    SendSNC = create Mailbox (1);
  };
};

process Transmitter : STransmitter (HStation) {
  int BusId;
  void setup (int dir) {
    Bus = S->Bus [BusId = dir];
    Buffer = &(S->Buffer [dir]);
    Strobe = S->Strobe [dir];
  };
  Boolean gotPacket ();
};

process Receiver : SReceiver (HStation) {
  void setup (int dir) {
    Bus = S->Bus [dir];
  };
};

process Strober (HStation) {
  Port *Bus;
  Mailbox *Strobe;
  Transmitter *MyXmitter;
  void setup (int dir, Transmitter *pr) {
    Bus = S->Bus [dir];
    MyXmitter = pr;
    Strobe = S->Strobe [dir];
  };
  states {WaitReady, WaitEmpty, EmptyLoop, WaitBOC, BOCLoop};
  perform;
};

process SlotGen (HeadEnd) {
  Port *Bus;
  Mailbox *SendBOC, *SendSNC;
  Packet *SMarker;
  void setup (int dir) {
    Bus = S->Bus [dir];
    SMarker = &(S->SMarker);
    SendBOC = S->SendBOC;
    SendSNC = S->SendSNC;
    SendBOC->put ();
  };
  states {Generate, XDone};
  perform;
};

process Absorber (HeadEnd) {
  Port *Bus;
  Mailbox *SendBOC, *SendSNC;
  Boolean WithinCycle;
  void setup (int dir) {
    Bus = S->Bus [dir];
    SendBOC = S->SendBOC;
    SendSNC = S->SendSNC;
    WithinCycle = NO;
  };
  states {WaitSlot, SlotLoop};
  perform;
};
  
