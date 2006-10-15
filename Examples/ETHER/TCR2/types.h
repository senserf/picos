#include "lbus.h"
#include "utraffic.h"
#include "ether.h"
#include "etherstn.h"
#include "cxmitter.h"
#include "etherrcv.h"

process Transmitter : CTransmitter (EtherStation) {
  int Ply,               // The level at which the station is competing
      DelayCount,        // Slots remaining to join the game
      DeferCount;        // Slots remaining until the tournament is over
  Boolean loser ();      // Tells whether the station loses in the current move
  void onCollision ();   // When the station senses a collision
  void onEndSlot ();     // End of an empty slot
  void onEOT () { onEndSlot (); };
  Boolean participating ();
};
