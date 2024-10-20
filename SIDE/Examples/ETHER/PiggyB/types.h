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
  int PiggyDirection;    // Piggyback direction of the last seen packet
  Mailbox *Ready;        // To signal readiness to transmit
  Boolean Blocked;       // Controlled mode
  void setup ();
};

#include "pxmitter.h"

process Mtr (PiggyStation) {
  Mailbox *Ready;
  TIME DelayP, DelayX;
  Port *Bus;
  Boolean eligible ();
  void setup ();
  states {Waiting, Active, EndActivity, GoSignal, EndPacket, MyTurn};
  perform;
};

process Transmitter : PTransmitter {
  void setPiggyDirection ();
};
