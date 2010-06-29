#ifndef __utraffic_c__
#define __utraffic_c__

// Interface to a uniform traffic pattern (all stations) with Poisson arrival
// and exponentially distributed message length

#include "utraffic.h"

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

Boolean ClientInterface::ready (Long mn, Long mx, Long fm) {
  return Buffer.isFull () || Client->getPacket (&Buffer, mn, mx, fm);
};

#endif
