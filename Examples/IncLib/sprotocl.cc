#ifndef __sprotocl_c__
#define __sprotocl_c__

#include "sprotocl.h"

// This is the generic transmitter/receiver code for slotted protocols (used
// in Fasnet and DQDB)

STransmitter::perform {
  state NPacket:
    if (gotPacket ()) {
      signal ();
      Strobe->wait (NEWITEM, Transmit);
    } else
      Client->wait (ARRIVAL, NPacket);
  state Transmit:
    Bus->transmit (Buffer, XDone);
    Bus->wait (COLLISION, Error);
  state XDone:
    Bus->stop ();
    Buffer->release ();
    proceed NPacket;
  state Error:
    excptn ("Slot collision");
};

SReceiver::perform {
  state WPacket:
    Bus->wait (EMP, Rcvd);
  state Rcvd:
    Client->receive (ThePacket, ThePort);
    skipto (WPacket);
};

#endif

