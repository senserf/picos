#include "hbus.h"
#include "utraffic.h"
#include "uprotocl.h"

station HStation : HBusInterface, ClientInterface {
  void setup () {
    HBusInterface::configure ();
    ClientInterface::configure ();
  };
};

extern Long MinPL, MaxPL, FrameL;    // Read from the input file

process Transmitter : UTransmitter (HStation) {
  void setup () {
    // Note: Bus is set by gotPacket after packet acquisition
    Buffer = &(S->Buffer);
  };
  Boolean gotPacket ();
};

process Receiver : UReceiver (HStation) {
  void setup (int dir) {
    // We still need two receivers, so this part is the same as for
    // for H-Net1. It could be put into IncLib.
    Bus = S->Bus [dir];
  };
};
