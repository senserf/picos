#include "hbus.h"
#include "utraff2l.h"
#include "uprotocl.h"

station HStation : HBusInterface, ClientInterface {
  void setup () {
    HBusInterface::configure ();
    ClientInterface::configure ();
  };
};

extern Long MinPL, MaxPL, FrameL;    // Read from the input file

process Transmitter : UTransmitter (HStation) {
  int BusId;
  void setup (int dir) {
    Bus = S->Bus [BusId = dir];
    Buffer = &(S->Buffer [dir]);
  };
  Boolean gotPacket ();
};

process Receiver : UReceiver (HStation) {
  void setup (int dir) {
    Bus = S->Bus [dir];
  };
};
