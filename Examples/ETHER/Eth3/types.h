#include "ether.h"
#include "lbus.h"
#include "utraffic.h"

#include "etherstn.h"

#include "etherrcv.h"

#define TRate 1       // Transmission rate: 1 ITU / bit

process Transmitter (EtherStation) {
  int CCounter;       // Collision counter
  Port *Bus;          // A copy of the bus port
  Packet *Buffer;     // Packet buffer pointer
  TIME backoff ();    // The standard backoff function
  void setup () {
    Bus = S->Bus;
    Buffer = &(S->Buffer);
  };
  states {NPacket, Retry, Xmit, XDone, XAbort, JDone};
  perform;
};
