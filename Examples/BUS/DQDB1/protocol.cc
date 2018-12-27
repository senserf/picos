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

identify DQDB1;
            
TIME SegmWindow; // Segment window length in ITUs

Boolean Transmitter::gotPacket () {
  if (S->ready (BusId, SegmPL, SegmPL, SegmFL)) {
    *CD = *RQ;
    *RQ = 0;
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
      else if (OtherXmitter->erase ())
        setFlag (p->Flags, RQST);
      if (flagCleared (p->Flags, FULL)) {
        if (Buffer->isFull ()) {
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
