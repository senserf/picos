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
#include "dqdb.h"
#include "fstrfl.h"
#include "sprotocl.h"

extern TIME SegmWindow; // Segment window length in ITUs

process Transmitter;

station HStation : HBusInterface, ClientInterface {
  Mailbox *Strobe [2];  // Go signals from the strobers
  int CD [2],           // The count-down counter (one version per bus)
      RQ [2],           // Request counter
      Active;           // The active bus
  void setup () {
    int i;
    HBusInterface::configure ();
    ClientInterface::configure ();
    for (i = 0; i < 2; i++) {
      Strobe [i] = create Mailbox (0);
      CD [i] = 0;
      RQ [i] = 0;
      Active = 0;
    }
  };
};

station HeadEnd : HStation {
  Packet SMarker;
  void setup () {
    HStation::setup ();
    SMarker.fill (NONE, NONE, SlotML);
  };
};

process Transmitter : STransmitter (HStation) {
  void setup () {
    Buffer = &(S->Buffer);
  };
  Boolean gotPacket ();
};

process Receiver : SReceiver (HStation) {
  void setup (int dir) {
    Bus = S->Bus [dir];
  };
};

process Strober (HStation) {
  int BusId;
  Port *Bus;
  Mailbox *Strobe;
  Packet *Buffer;
  Transmitter *Xmitter;
  int *MyRQ, *OtherRQ, *MyCD;
  void setup (int dir, Transmitter *pr) {
    Bus = S->Bus [BusId = dir];
    Strobe = S->Strobe [dir];
    Buffer = &(S->Buffer);
    MyRQ = &(S->RQ [dir]);
    MyCD = &(S->CD [dir]);
    OtherRQ = &(S->RQ [1-dir]);
    Xmitter = pr;
  };
  states {WaitSlot, WaitLoop};
  perform;
};

process SlotGen (HeadEnd) {
  Port *Bus;
  Packet *SMarker;
  void setup (int dir) {
    Bus = S->Bus [dir];
    SMarker = &(S->SMarker);
  };
  states {Generate, XDone};
  perform;
};
  
