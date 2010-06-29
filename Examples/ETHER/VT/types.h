#include "lbus.h"
#include "utraffic.h"
#include "ether.h"
#include "etherstn.h"
#include "cxmitter.h"
#include "etherrcv.h"

process Transmitter : CTransmitter (EtherStation) {
  Long Priority;         // The station's priority
  void onCollision ();   // When the station senses a collision
  void onEndSlot ();     // End of an empty slot
  void onEOT () {        // Sensing end of a valid transmission
    onEndSlot ();
  };
  Boolean participating ();
  void setup () {
    CTransmitter::setup ();
    Priority = S->getId ();
  };
};
