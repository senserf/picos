#ifndef __cobserver_h__
#define __cobserver_h__

// This file contains the type declaration of an observer whose code
// is provided in 'cobsrvr.cc' This observer verifies that no packet
// collides more than a specified number of times. It can be used to
// validate the collision protocols that enforce a limit on the
// maximum number of collisions per packet, e.g., TCR, DP, and VT.

observer CObserver {
  int *CCount,              // Collision counters per stations
      MaxCollisions;        // Maximum number of collisions per packet
  void setup (int);
  states {Monitoring, EndTransfer, Collision};
  perform;
};

#endif
