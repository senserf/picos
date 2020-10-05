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

#include "types.h"
#include "sprotocl.cc"

identify DQDB2;
            
TIME SegmWindow; // Segment window length in ITUs

Boolean Transmitter::gotPacket () {
  int dir;
  if (S->ready (SegmPL, SegmPL, SegmFL)) {
    // Determine transfer direction
    S->Active = dir = (Buffer->Receiver < S->getId ()) ? RLBus : LRBus;
    S->CD [dir] = S->RQ [dir];
    S->RQ [dir] = 0;
    Bus = S->Bus [dir];
    Strobe = S->Strobe [dir];
    return YES;
  } else
    return NO;
};

Strober::perform {
  Packet *p;
  state WaitSlot:
    Bus->wait (EOT, WaitLoop);
  state WaitLoop:
    p = ThePacket;
    if (p->TP == SLOT) {
      if (flagSet (p->Flags, RQST))
        (*OtherRQ)++;
      else if (S->Active != BusId && Xmitter->erase ())
        setFlag (p->Flags, RQST);
      if (flagCleared (p->Flags, FULL)) {
        if (S->Active == BusId && Buffer->isFull ()) {
          // The transmitter is waiting
          if (*MyCD == 0) {
            setFlag (p->Flags, FULL);
            Strobe->put ();
          } else
            (*MyCD)--;
        } else {
          if (*MyRQ > 0) (*MyRQ)--;
        }
      }
    }
    skipto WaitSlot;
};

SlotGen::perform {
  state Generate:
    Bus->transmit (SMarker, XDone);
  state XDone:
    Bus->stop ();
    Timer->wait (SegmWindow, Generate);
};
