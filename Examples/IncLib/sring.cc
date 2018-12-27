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

#ifndef __sring_c__
#define __sring_c__

// This file contains network configuration functions defining a single
// unidirectional ring with stations equally spaced along it.

#include "sring.h"

static PLink **Segments;
static RATE TR;
static DISTANCE D;
static int NP, NC;
static SRingInterface **RStations;

void initSRing (RATE r, TIME l, int np) {
  int i;
  NP = np;         // The number of stations to connect
  Segments = new PLink* [NP];
  for (i = 0; i < NP; i++) Segments [i] = create PLink (2);
  TR = r;
  D = l / NP;      // Distance between neighbors
  NC = 0;          // Number of ring stations built so far
  RStations = new SRingInterface* [NP];
};

void SRingInterface::configure () {
  int i, j;
  IRing = create Port;      // Rate irrelevant for the input port
  ORing = create Port (TR);
  RStations [NC++] = this;
  if (NC == NP) {
    // Connect ports and set distances
    for (i = 0; i < NP; i++) {
      j = (i+1) % NP;
      RStations [i] -> ORing -> connect (Segments [i]);
      RStations [j] -> IRing -> connect (Segments [i]);
      RStations [i] -> ORing -> setDTo (RStations [j] -> IRing, D);
    }
    delete RStations;
    delete Segments;
  }
};

#endif
