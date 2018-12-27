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

#ifndef __pring_c__
#define __pring_c__

// Pretzel Ring configuration consisting of two segments. One dedicated station
// is a monitor responsible for ring initialization.

#include "pring.h"

static PLink **Segments;
static RATE TR;
static DISTANCE D;
static int NP, NC;
static PRingInterface **RStations;

void initPRing (RATE r, TIME l, int np) {
  int i, nl;
  NP = np;           // The number of stations to connect
  Segments = new PLink* [nl = NP+NP+1];
  // Note: one additional segment is used to connect the monitor
  // The monitor will be connected
  for (i = 0; i < nl; i++)
      Segments [i] = create PLink (2);
  TR = r;
  D = l / NP;        // Distance between neighbors
  NC = 0;            // Number of stations built so far
  RStations = new PRingInterface* [NP];
};

void PRingInterface::configure () {
  int i;
  for (i = 0; i < 2; i++) {
    IRing [i] = create Port;      // Rate irrelevant for the input port
    ORing [i] = create Port (TR);
  }
  RStations [NC++] = this;
};

void PMonitorInterface::configure () {
  int i, LN;
  Port *p1, *p2;
  // We assume that the monitor station is created last
  Assert (NC == NP, "Monitor must be created last");
  IRing = create Port;
  ORing = create Port (TR);
  for (LN = 0, i = 0; i < NP-1; i++, LN++) {
    // The first segment (ports number 0)
    (p1 = RStations [i] -> ORing [0]) -> connect (Segments [LN]);
    (p2 = RStations [i+1] -> IRing [0]) -> connect (Segments [LN]);
    p1->setDTo (p2, D);
  };
  // Cross connection from station NP-1 to station 0, port 0 to 1
  (p1 = RStations [NP-1] -> ORing [0]) -> connect (Segments [LN]);
  (p2 = RStations [0] -> IRing [1]) -> connect (Segments [LN]);
  p1->setDTo (p2, D);
  for (LN++, i = 0; i < NP-1; i++, LN++) {
    // The second segment (ports number 1)
    (p1 = RStations [i] -> ORing [1]) -> connect (Segments [LN]);
    (p2 = RStations [i+1] -> IRing [1]) -> connect (Segments [LN]);
    p1->setDTo (p2, D);
  };
  // Connection from station NP-1 to monitor
  (p1 = RStations [NP-1] -> ORing [1]) -> connect (Segments [LN]);
  IRing -> connect (Segments [LN]);
  p1->setDTo (IRing, D/2);
  LN++;
  // Cross connection from monitor to station 0
  ORing -> connect (Segments [LN]);
  (p2 = RStations [0] -> IRing [0]) -> connect (Segments [LN]);
  ORing->setDTo (p2, D/2);
  delete Segments;
  delete RStations;
};

#endif
