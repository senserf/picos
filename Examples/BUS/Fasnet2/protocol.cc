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

#include "types.h"
#include "sprotocl.cc"

identify Fasnet2;

Long SlotML,     // Slot marker length (in bits)
     SegmPL,     // Segment payload length
     SegmFL;     // The length of the segment header and trailer
            
TIME SegmWindow; // Segment window length in ITUs

Boolean Transmitter::gotPacket () {
  return S->ready (BusId, SegmPL, SegmPL, SegmFL);
};

Strober::perform {
  state WaitReady:
    MyXmitter->wait (SIGNAL, WaitEmpty);
  state WaitEmpty:
    Bus->wait (EOT, EmptyLoop);
  state EmptyLoop:
    if (ThePacket->TP == SLOT && flagCleared (ThePacket->Flags, FULL)) {
      setFlag (ThePacket->Flags, FULL);
      Strobe->put ();
      skipto WaitBOC;
    } else
      skipto WaitEmpty;
  state WaitBOC:
    Bus->wait (EOT, BOCLoop);
  state BOCLoop:
    if (ThePacket->TP == SLOT && flagSet (ThePacket->Flags, BOC))
      proceed WaitReady;
    else
      skipto WaitBOC;
};

SlotGen::perform {
  state Generate:
    if (SendBOC->get ())
      setFlag (SMarker->Flags, BOC);
    else
      clearFlag (SMarker->Flags, BOC);
    if (SendSNC->get ())
      setFlag (SMarker->Flags, SNC);
    else
      clearFlag (SMarker->Flags, SNC);
    Bus->transmit (SMarker, XDone);
  state XDone:
    Bus->stop ();
    Timer->wait (SegmWindow, Generate);
};

Absorber::perform {
  state WaitSlot:
    Bus->wait (EOT, SlotLoop);
  state SlotLoop:
    if (ThePacket->TP == SLOT) {
      if (flagSet (ThePacket->Flags, BOC)) WithinCycle = YES;
      if (WithinCycle && flagCleared (ThePacket->Flags, FULL)) {
        WithinCycle = NO;
        SendSNC->put ();
      }
      if (flagSet (ThePacket->Flags, SNC)) SendBOC->put ();
    }
    skipto WaitSlot;
};
