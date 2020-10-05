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

#include "lbus.h"
#include "ether.h"
#include "utraffic.h"
#include "etherstn.h"
#include "etherrcv.h"

station PiggyStation : EtherStation {
  DISTANCE LDist,        // Distance from the left end of the cable
           RDist;        // Distance from the right end of the cable
  int PiggyDirection,    // Piggyback direction of the last seen packet
      Turn;              // Piggyback turn (comes with ready signal)
  Mailbox *Ready;        // To signal readiness to transmit
  Boolean Blocked;       // Controlled mode
  void setup ();
};

#include "pxmitter.h"

process Mtr (PiggyStation) {
  DISTANCE LDist, RDist;
  Mailbox *Ready;
  TIME Delay1, Delay2, DelayX;
  Port *Bus;
  void setDelays ();
  void setup ();
  states {Waiting, Active, EndActivity, GoSignal, EndPacket, FTurn, STurn};
  perform;
};

process Transmitter : PTransmitter {
  void setPiggyDirection ();
};
