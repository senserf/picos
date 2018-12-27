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

#ifndef __mring_c__
#define __mring_c__

// Metaring configuration consisting of four independent rings. Two of those
// rings represent virtual channels used for SAT communicates.

#include "mring.h"

static PLink **REG [2], **SAT [2];
static RATE TR;
static DISTANCE D;
static int NP, NC;
static MRingInterface **RStations;

void initMRing (RATE r, TIME l, int np) {
  int i, j;
  NP = np;         // The number of stations to connect
  for (j = 0; j < 2; j++) {
    REG [j] = new PLink* [NP];
    SAT [j] = new PLink* [NP];
    for (i = 0; i < NP; i++) {
      REG [j][i] = create PLink (2);
      SAT [j][i] = create PLink (2);
    }
  }
  TR = r;
  D = l / NP;      // Distance between neighbors
  NC = 0;          // Number of ring stations built so far
  RStations = new MRingInterface* [NP];
};

void MRingInterface::configure () {
  int i, j;
  for (i = 0; i < 2; i++) {
    IRing [i] = create Port;      // Rate irrelevant for the input port
    ORing [i] = create Port (TR);
    ISat [i] = create Port;
    OSat [i] = create Port (TR);
  }
  RStations [NC++] = this;
  if (NC == NP) {
    // Connect ports and set distances
    for (i = 0; i < NP; i++) {
      // The clockwise rings
      j = (i+1) % NP;
      RStations [i] -> ORing [CWRing] -> connect (REG [CWRing][i]);
      RStations [j] -> IRing [CWRing] -> connect (REG [CWRing][i]);
      RStations [i] -> ORing [CWRing] ->
        setDTo (RStations [j] -> IRing [CWRing], D);
      // The clockwise SAT ring belongs to the counter-clockwise direction
      RStations [i] -> OSat [CCRing] -> connect (SAT [CCRing][i]);
      RStations [j] -> ISat [CCRing] -> connect (SAT [CCRing][i]);
      RStations [i] -> OSat [CCRing] ->
        setDTo (RStations [j] -> ISat [CCRing], D);
      // The counter-clockwise rings
      j = (i+1) % NP;
      RStations [j] -> ORing [CCRing] -> connect (REG [CCRing][i]);
      RStations [i] -> IRing [CCRing] -> connect (REG [CCRing][i]);
      RStations [j] -> ORing [CCRing] ->
        setDTo (RStations [i] -> IRing [CCRing], D);
      // The counter-clockwise SAT ring belongs to the clockwise direction
      RStations [j] -> OSat [CWRing] -> connect (SAT [CWRing][i]);
      RStations [i] -> ISat [CWRing] -> connect (SAT [CWRing][i]);
      RStations [j] -> OSat [CWRing] ->
        setDTo (RStations [i] -> ISat [CWRing], D);
    }
    delete RStations;
    for (i = 0; i < 2; i++) {
      delete REG [i];
      delete SAT [i];
    }
  }
};

#endif
