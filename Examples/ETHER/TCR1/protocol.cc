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
identify TCR1;

#include "types.h"

TIME SlotLength,      // The length of a virtual slot (2L + epsilon)
     GuardDelay,      // To compensate for non-zero clock tolerance
     TPSpace,         // Precomputed TIME versions of PSpace and JamL
     TJamL;

RATE TRate;           // Time granularity (read from data file)

Transmitter:: perform {
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
    if (!TournamentInProgress) {
      Ply = DelayCount = 0;
      DeferCount = 1;
      TournamentInProgress = YES;
    }
    DeferCount++;
    if (Competing) {
      if (Transmitting) {
        if (loser ()) DelayCount = 1;
        Ply++;
        Bus->abort ();
        Transmitting = NO;
        Bus->sendJam (TJamL, EndJam);
        sleep;
      } else
        DelayCount++;
    }
    Timer->wait (TJamL + SlotLength, NewSlot);
  state EndJam:
    Bus->stop ();
    Timer->wait (SlotLength, NewSlot);
  state NewSlot:
    if (Competing && DelayCount == 0)
      Timer->wait (GuardDelay, Transmit);
    else {
      Timer->wait (SlotLength, EndSlot);
      Bus->wait (ACTIVITY, Busy);
    }
  state EndSlot:
    if (--DeferCount == 0) {
      TournamentInProgress = NO;
      Timer->wait (GuardDelay, NPacket);
    } else {
      if (Competing) DelayCount--;
      proceed NewSlot;
    }
  state SenseEOT:
    if (TournamentInProgress)
      Timer->wait (TPSpace, EndSlot);
    else
      Timer->wait (TPSpace, NPacket);
};

Boolean Transmitter::loser () {
  return (S->getId () >> Ply) & 01;
};
