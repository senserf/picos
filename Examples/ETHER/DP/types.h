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

#include "lbus.h"
#include "utraffic.h"
#include "ether.h"
#include "etherstn.h"
#include "cxmitter.h"
#include "etherrcv.h"

process Transmitter : CTransmitter (EtherStation) {
  Long Priority,         // The dynamic priority of the station
       DelayCount,       // Delay counter for the current station
       DeferCount;       // Delay counter for the permanent loser
  void onCollision ();   // When the station senses a collision
  void onEndSlot ();     // End of an empty slot
  void onEOT ();         // End of a successful transmission
  Boolean participating ();
  void setup () {
    CTransmitter::setup ();
    Priority = S->getId ();
  };
};
