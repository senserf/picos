#include "lbus.h"
#include "ether.h"
#include "utraffic.h"
#include "etherstn.h"
#include "etherrcv.h"

station PiggyStation : EtherStation {
  DISTANCE LDist,        // Distance from the left end of the cable
           RDist;        // Distance from the right end of the cable
  int PiggyDirection,    // Piggyback direction of the last seen packet
      Turn;              // Piggyback turn (comes with ready signal)
  Mailbox *Ready;        // To signal readiness to transmit
  Boolean Blocked;       // Controlled mode
  void setup ();
};

#include "pxmitter.h"

process Mtr (PiggyStation) {
  DISTANCE LDist, RDist;
  Mailbox *Ready;
  TIME Delay1, Delay2, DelayX;
  Port *Bus;
  void setDelays ();
  void setup ();
  states {Waiting, Active, EndActivity, GoSignal, EndPacket, FTurn, STurn};
  perform;
};

process Transmitter : PTransmitter {
  void setPiggyDirection ();
};
