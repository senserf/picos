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

static Link *TheBus;
static RATE TR;
static DISTANCE D;
static int NP, NC;
static BusInterface **Connected;

void initBus (RATE r, DISTANCE l, int np, TIME ar) {
  TheBus = create Link (np, ar);
  TR = r;
  D = l / (np-1);  // Distance between neighbors
  NP = np;         // Number of stations to connect
  NC = 0;          // Number of connected stations
  Connected = new BusInterface* [NP];
};

void BusInterface::configure () {
  int i;
  Bus = create Port (TR);
  Bus->connect (TheBus);
  for (i = 0; i < NC; i++)
    Bus->setDTo (Connected[i]->Bus, D * (NC - i));
  if (NC == NP-1)
    delete Connected;
  else
    Connected [NC++] = this;
};
