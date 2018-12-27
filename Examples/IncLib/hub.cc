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

#ifndef __hub_c__
#define __hub_c__

// Hubnet with a single hub. Lengths of link segments are generated as
// exponentially distributed random numbers with the specified mean. Define
// 'SameLinkLength' to make all links of the same length (equal to the
// second argument of 'initHub').

#include "hub.h"

static RATE TR;
static TIME MLL;
static int NP, NC;
static HubInterface **HStations;

void initHub (RATE r, TIME l, int np) {
  NP = np;         // The number of stations to connect
  HStations = new HubInterface* [NP];
  TR = r;
  MLL = l;
  NC = 0;          // Number of stations built so far
};

void HubInterface::configure () {
  SPort = create Port (TR);
  BPort = create Port;       // Rate irrelevant, used for reception
  Assert (NC < NP, "Too many stations");
  HStations [NC++] = this;
};

void HubStation::configure () {
  int i, j;
  Link *lk;
  TIME *lg;
  // The hub is created last
  Assert (NC == NP, "The hub must be created last");
  SPorts = new Port* [NP];
  lg = new TIME [NP];
  for (i = 0; i < NP; i++) SPorts [i] = create Port;
  BPort = create Port (TR);
  // Selection links
  for (i = 0; i < NP; i++) {
    lk = create Link (2);
    HStations [i] -> SPort -> connect (lk);
    SPorts [i] -> connect (lk);
#ifdef SameLinkLength
    lg [i] = MLL;
#else
    lg [i] = tRndPoisson (MLL);
#endif
    setD (HStations [i] -> SPort, SPorts [i], lg [i]);
  }
  // The broadcast link
  lk = create Link (NP + 1);
  BPort->connect (lk);
  for (i = 0; i < NP; i++) {
    HStations [i] -> BPort -> connect (lk);
    setD (HStations [i] -> BPort, BPort, lg [i]);
  }
  // Distances between regular stations
  for (i = 0; i < NP-1; i++)
    for (j = i+1; j < NP; j++)
      setD (HStations[i]->BPort, HStations[j]->BPort, lg[i] + lg[j]);
  delete lg;
  delete HStations;
};

#endif
