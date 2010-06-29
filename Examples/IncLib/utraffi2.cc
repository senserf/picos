#ifndef __utraffic2_c__
#define __utraffic2_c__

// Interface from a dual-bus station to a uniform traffic pattern (all
// stations) with Poisson arrival and exponentially distributed message
// length. Each station has two buffers, each buffers for storing
// packets going in a given direction.

#include "utraffi2.h"

static Traffic *UTP;

void initTraffic () {
  double mit, mle;
  readIn (mit);
  readIn (mle);
  UTP = create Traffic (MIT_exp+MLE_exp, mit, mle);
};

void ClientInterface::configure () {
  UTP->addSender (TheStation);
  UTP->addReceiver (TheStation);
};

#ifdef PrivateQualifier

  // Note: by defining PrivateQualifier you can program your own 'qual'
  // function that will be used for packet acquisition. This is done
  // in Metaring.

  extern int Direction;
  int qual (Message*);

#else

  static int Direction;      // For packet acquisition

  static int qual (Message *m) {
    // The qualifier function for getPacket
    return (Direction == Left && TheStation->getId () > m->Receiver) ||
        (Direction == Right && TheStation->getId () < m->Receiver);
  };
  
#endif

Boolean ClientInterface::ready (int d, Long mn, Long mx, Long fm) {
  Direction = d;
  return Buffer [d] . isFull () ||
    Client->getPacket (&(Buffer [d]), qual, mn, mx, fm);
};

#endif
