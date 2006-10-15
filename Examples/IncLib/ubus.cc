#ifndef __ubus_c__
#define __ubus_c__

// This file contains network configuration functions defining a folded,
// U-shaped, unidirectional bus with all stations equally spaced along it.
// Each station has two ports to the bus.

#include "ubus.h"

static Link *TheBus;
static RATE TR;
static DISTANCE D, Turn;
static int NP, NC;
static UBusInterface **Created;

void initUBus (RATE r, TIME l, DISTANCE tl, int np) {
  TheBus = create PLink (np + np);
  TR = r;
  D = l / (np-1);  // Distance between neighbors
  Turn = tl;       // Length of the turning segment
  NP = np;         // Number of stations to connect
  NC = 0;          // Number of connected stations
  Created = new UBusInterface* [NP];
};

void UBusInterface::configure () {
  int i;
  UBusInterface *s;
  Created [NC++] = this;
  if (NC == NP) {
    // All stations have been created -- connect the ports
    for (i = 0; i < NP; i++) {
      // The outbound segment
      s = Created [i];
      s->OBus = create (i) Port (TR);
      s->OBus -> connect (TheBus);
      if (i > 0) Created [i-1] -> OBus -> setDTo (s->OBus, D);
    };
    for (i = NP-1; i >= 0; i--) {
      // The inbound segment
      s = Created [i];
      s->IBus = create (i) Port (TR);
      s->IBus -> connect (TheBus);
      if (i < NP-1) Created [i+1] -> IBus -> setDTo (s->IBus, D);
    };
	// The turning segment
    Created [NP-1]->OBus -> setDTo (Created [NP-1]->IBus, Turn);
	delete Created;
    // Restore TheStation
    TheStation = idToStation (NP-1);
  };
};

#endif
