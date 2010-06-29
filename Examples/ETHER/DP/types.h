#include "lbus.h"
#include "utraffic.h"
#include "ether.h"
#include "etherstn.h"
#include "cxmitter.h"
#include "etherrcv.h"

process Transmitter : CTransmitter (EtherStation) {
  Long Priority,         // The dynamic priority of the station
       DelayCount,       // Delay counter for the current station
       DeferCount;       // Delay counter for the permanent loser
  void onCollision ();   // When the station senses a collision
  void onEndSlot ();     // End of an empty slot
  void onEOT ();         // End of a successful transmission
  Boolean participating ();
  void setup () {
    CTransmitter::setup ();
    Priority = S->getId ();
  };
};
