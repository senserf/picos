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

#ifndef __ptransmitter_h__
#define __ptransmitter_h__

// Defines global variables and announces the transmitter type for Piggyback
// Ethernet.

#define Left  0          // Piggyback directions
#define Right 1

#define piggyDirection(p) (flagSet (p->Flags, PF_usr0))
#define setPiggyLeft(p)   (clearFlag (p->Flags, PF_usr0))
#define setPiggyRight(p)  (setFlag (p->Flags, PF_usr0))

extern int  MinIPL,      // Minimum inflated packet length (frame excluded)
            MinUPL,      // Minimum uninflated packet length
            MaxUPL,      // Maximum packet length (payload)
            PFrame;      // Frame length

// JamL and PSpace are borrowed from Ethernet, but we need these to have
// their TIME versions

extern TIME TPSpace,     // After a collision
            TJamL;

extern RATE TRate;

extern Long DelayQuantum;

extern DISTANCE L;       // Actual bus length

process PTransmitter (PiggyStation) {
  Port *Bus;
  Packet *Buffer;
  int CCounter;
  Mailbox *Ready;
  void inflate (), deflate ();
  TIME backoff ();
  virtual void setPiggyDirection () { };
  void setup ();
  states {NPacket, WaitBus, Piggyback, XDone, Abort, EndJam, Error};
  perform;
};

#endif
