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

#ifndef __ctransmitter_h__
#define __ctransmitter_h__

// This file defines the process type describing the shared transmitter code
// for TCR and DP, and possibly other collision protocols in which colliding
// stations play a 'tournament' to determine the transmission order. This
// process type must be extended to redefine the virtual methods.

extern TIME SlotLength,      // The length of a virtual slot (2L + epsilon)
            GuardDelay,      // To compensate for non-zero clock tolerance
            TPSpace,         // Precomputed TIME versions of PSpace and JamL
            TJamL;

extern RATE TRate;           // Time granularity (read from data file)

process CTransmitter abstract (EtherStation) {
  Port *Bus;          // A copy of the bus port
  Packet *Buffer;     // Packet buffer pointer
  Boolean TournamentInProgress, Transmitting, Competing;
  void setup () {
    Bus = S->Bus;
    Buffer = &(S->Buffer);
    TournamentInProgress = Transmitting = Competing = NO;
  };
  virtual void onCollision () = 0;   // When the station senses a collision
  virtual void onEndSlot () = 0;     // End of an empty slot
  virtual void onEOT () = 0;         // Sensing end of a valid transmission
  virtual Boolean participating () {
    // Tells whether station is making a move
    return YES;
  };
  states {NPacket, Transmit, Busy, XDone, SenseEOT, SenseCollision, EndJam,
          NewSlot, EndSlot};
  perform;
};

#endif
