#include "ubus.h"
#include "fstrfl.h"
#include "uprotocl.h"

station UStation : UBusInterface, ClientInterface {
  void setup () {
    UBusInterface::configure ();
    ClientInterface::configure ();
  };
};

extern Long MinPL, MaxPL, FrameL;    // Read from the input file

process Transmitter : UTransmitter (UStation) {
  void setup () {
    Bus = S->OBus;
    Buffer = &(S->Buffer);
  };
  Boolean gotPacket ();
};

process Receiver : UReceiver (UStation) {
  void setup () {
    Bus = S->IBus;
  };
};
