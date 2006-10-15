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
