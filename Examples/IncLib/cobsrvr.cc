#ifndef __cobserver_c__
#define __cobserver_c__

// This is the code of an observer verifying that no packet collides
// more than a specified number of times. This observer can be used
// to validate the collision protocols that enforce a limit on the
// maximum number of collisions per packet, e.g., TCR, DP, and VT.

#include "cobsrvr.h"

void CObserver::setup (int max) {
  CCount = new int [NStations];
  for (int i = 0; i < NStations; i++) CCount [i] = 0;
  MaxCollisions = max;
};

CObserver::perform {
  state Monitoring:
    inspect (ANY, Transmitter, XDone, EndTransfer);
    inspect (ANY, Transmitter, EndJam, Collision);
  state EndTransfer:
    CCount [TheStation->getId ()] = 0;
    proceed Monitoring;
  state Collision:
    if (++(CCount [TheStation->getId ()]) > MaxCollisions)
      excptn ("Too many collisions");
    proceed Monitoring;
};

#endif
