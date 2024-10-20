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

#include "ether.h"
#include "lbus.h"
#include "utraffic.h"
#include "etherstn.h"
#include "etherrcv.h"

extern TIME SlotLength,      // The length of a virtual slot (2L + epsilon)
            GuardDelay,      // To compensate for non-zero clock tolerance
            TPSpace,         // Precomputed TIME versions of PSpace and JamL
            TJamL;

extern RATE TRate;           // Time granularity (read from data file)

process Transmitter (EtherStation) {
  Port *Bus;          // A copy of the bus port
  Packet *Buffer;     // Packet buffer pointer
  Boolean TournamentInProgress,
          Transmitting,
          Competing;
  int Ply,            // The level at which the station is competing
      DelayCount,     // Waiting time (in slots) for a loser
      DeferCount;     // Waiting time for a 'permanent loser'
  Boolean loser ();   // Tells whether the station loses in the current move
  void setup () {
    Bus = S->Bus;
    Buffer = &(S->Buffer);
    TournamentInProgress = Transmitting = Competing = NO;
  };
  states {NPacket, Transmit, Busy, XDone, SenseEOT, SenseCollision, EndJam,
          NewSlot, EndSlot};
  perform;
};
