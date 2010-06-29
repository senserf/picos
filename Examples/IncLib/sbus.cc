#ifndef __sbus_c__
#define __sbus_c__

// This file contains network configuration functions defining an S-shaped
// unidirectional bus with all stations equally spaced along it.
// Each station has two ports to the bus. This topology is used in
// Expressnet.

#include "sbus.h"

static Link *TheBus;
static RATE TR;
static DISTANCE D;       // Distance between neighbors
static DISTANCE Turn;   // The length of the turning segment
static int NP, NC;
static SBusInterface **Created;

void initSBus (RATE r, DISTANCE l, DISTANCE t, int np) {
  TheBus = create PLink (np+np);
  TR = r;
  D = l / (np-1);   // Distance between neighbors
  Turn = t;
  NP = np;          // Number of stations to connect
  NC = 0;           // Number of connected stations
  Created = new SBusInterface* [NP];
};

void SBusInterface::configure () {
  int i;
  SBusInterface *s;
  Created [NC++] = this;
  if (NC == NP) {
    // All stations have been created -- connect the ports
    for (i = 0; i < NP; i++) {
      // The outgoing segment
      s = Created [i];
      s->OBus = create (i) Port (TR);
      s->OBus -> connect (TheBus);
      if (i > 0) Created [i-1] -> OBus -> setDTo (s->OBus, D);
    }
    for (i = 0; i < NP; i++) {
      // The incoming segment
      s = Created [i];
      s->IBus = create (i) Port;  // Transmission rate is irrelevant
      s->IBus -> connect (TheBus);
      if (i == 0)
        Created [NP-1] -> OBus -> setDTo (s->IBus, Turn);
      else
        Created [i-1] -> IBus -> setDTo (s->IBus, D);
    };
    delete Created;
    // Restore TheStation
    TheStation = idToStation (NP-1);
  }
};

#endif
