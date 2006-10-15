#ifndef __utrafficl_c__
#define __utrafficl_c__

// Interface to a uniform traffic pattern (all stations) with Poisson arrival
// and exponentially distributed message length. This is the same as
// utraffic.[ch], except that here we calculate message access time
// individually for each station -- to see how fair/unfair the protocol is.

#include "utraffil.h"

static UTraffic *UTP;

static ClientInterface *CInt [MAXSTATIONS];

void initTraffic () {
  double mit, mle;
  int i;
  readIn (mit);
  readIn (mle);
  UTP = create UTraffic (MIT_exp+MLE_exp, mit, mle);
  for (i = 0; i < MAXSTATIONS; i++) CInt [i] = NULL;
};

void ClientInterface::configure () {
  UTP->addSender (TheStation);
  UTP->addReceiver (TheStation);
  MAT = create RVariable, form ("MAT Sttn %3d", TheStation->getId ());
  Assert (TheStation->getId () < MAXSTATIONS,
    "Too many stations, increase MAXSTATIONS in utraffil.h");
  CInt [TheStation->getId ()] = this;
};

Boolean ClientInterface::ready (Long mn, Long mx, Long fm) {
  return Buffer.isFull () || Client->getPacket (&Buffer, mn, mx, fm);
};

void UTraffic::pfmMTR (Packet *p) {
  double d;
  d = (double) (Time - p->QTime) * Itu;
  CInt [TheStation->getId ()] -> MAT -> update (d);
};

#include "lmatexp.cc"

#endif
