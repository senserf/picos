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

Boolean ClientInterface::ready (long mn, long mx, long fm) {
  return Buffer.isFull () || UTP->getPacket (&Buffer, mn, mx, fm);
};
