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
