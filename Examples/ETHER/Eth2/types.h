#include "ether.h"
#include "lbus.h"
#include "utraffic.h"

station EtherStation : BusInterface, ClientInterface { 
  void setup () {
    BusInterface::configure ();
    ClientInterface::configure ();
  };
};

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

process Receiver (EtherStation) {
  Port *Bus;       // A copy of the bus port
  void setup () { Bus = S->Bus; };
  states {WPacket, Rcvd};
  perform;
};
