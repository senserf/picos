#include "lbus.h"
#include "ether.h"
#include "utraffic.h"
#include "etherstn.h"
#include "etherrcv.h"

station PiggyStation : EtherStation {
  int PiggyDirection;    // Piggyback direction of the last seen packet
  Mailbox *Ready;        // To signal readiness to transmit
  Boolean Blocked;       // Controlled mode
  void setup ();
};

#include "pxmitter.h"

process Monitor (PiggyStation) {
  Mailbox *Ready;
  TIME DelayP, DelayX;
  Port *Bus;
  Boolean eligible ();
  void setup ();
  states {Waiting, Active, EndActivity, GoSignal, EndPacket, MyTurn};
  perform;
};

process Transmitter : PTransmitter {
  void setPiggyDirection ();
};
