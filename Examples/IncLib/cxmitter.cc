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

#ifndef __ctransmitter_c__
#define __ctransmitter_c__

// This is the shared code of the transmitter process for TCR and DP, and
// possibly other collision protocols in which colliding stations play a
// 'tournament' to determine the transmission order. This code must be
// augmented by virtual methods defined in the process subtype.

#include "cxmitter.h"

CTransmitter:: perform {
  state NPacket:
    if (S->ready (MinPL, MaxPL, FrameL))
      proceed Transmit;
    else {
      Client->wait (ARRIVAL, NPacket);
      Bus->wait (ACTIVITY, Busy);
    }
  state Transmit:
    Transmitting = YES;
    Competing = YES;
    Bus->transmit (Buffer, XDone);
    Bus->wait (COLLISION, SenseCollision);
  state Busy:
    Bus->wait (EOT, SenseEOT);
    Bus->wait (COLLISION, SenseCollision);
  state XDone:
    Bus->stop ();
    Transmitting = NO;
    Competing = NO;
    Buffer->release ();
    proceed SenseEOT;
  state SenseCollision:
    onCollision ();
    if (Transmitting) {
      Bus->abort ();
      Transmitting = NO;
      Bus->sendJam (TJamL, EndJam);
    } else
      Timer->wait (TJamL + SlotLength, NewSlot);
  state EndJam:
    Bus->stop ();
    Timer->wait (SlotLength, NewSlot);
  state NewSlot:
    if (participating ())
      Timer->wait (GuardDelay, Transmit);
    else {
      Timer->wait (SlotLength, EndSlot);
      Bus->wait (ACTIVITY, Busy);
    }
  state EndSlot:
    onEndSlot ();
    if (TournamentInProgress)
      proceed NewSlot;
    else
      Timer->wait (GuardDelay, NPacket);
  state SenseEOT:
    onEOT ();
    if (TournamentInProgress)
      Timer->wait (TPSpace, NewSlot);
    else
      Timer->wait (TPSpace, NPacket);
};

#endif
