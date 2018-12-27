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

#ifndef __hbus_c__
#define __hbus_c__

// This file contains network configuration functions defining a strictly
// linear, dual, unidirectional bus with all stations equally spaced along it.
// Each station has one port to each bus.

#include "hbus.h"

static Link *TheBus [2];
static RATE TR;
static DISTANCE D;
static int NP, NC;
static HBusInterface **Created;

void initHBus (RATE r, TIME l, int np) {
  int i;
  for (i = 0; i < 2; i++) TheBus [i] = create PLink (np);
  TR = r;
  D = l / (np-1);  // Distance between neighbors
  NP = np;         // Number of stations to connect
  NC = 0;          // Number of connected stations
  Created = new HBusInterface* [NP];
};

void HBusInterface::configure () {
  int i;
  HBusInterface *s;
  Created [NC++] = this;
  if (NC == NP) {
    // All stations have been created -- connect the ports
    for (i = 0; i < NP; i++) {
      // LR bus
      s = Created [i];
      s->Bus [LRBus] = create (i) Port (TR);
      s->Bus [LRBus] -> connect (TheBus [LRBus]);
      if (i > 0) Created [i-1] -> Bus [LRBus] -> setDTo (s->Bus [LRBus], D);
    };
    for (i = NP-1; i >= 0; i--) {
      // RL bus
      s = Created [i];
      s->Bus [RLBus] = create (i) Port (TR);
      s->Bus [RLBus] -> connect (TheBus [RLBus]);
      if (i < NP-1) Created [i+1] -> Bus [RLBus] -> setDTo (s->Bus [RLBus], D);
    };
	delete Created;
    // Restore TheStation
    TheStation = idToStation (NP-1);
  };
};

#endif
