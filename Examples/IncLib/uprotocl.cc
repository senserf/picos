#ifndef __uprotocol_c__
#define __uprotocol_c__

#include "uprotocl.h"

// This file contains the common transmitter and receiver code for H-Net
// (versions 1 and 2) and U-Net

Long MinPL, MaxPL, FrameL;  // Read from the input file

UTransmitter::perform {
  state NPacket:
    if (gotPacket ())
      proceed WSilence;
    else
      Client->wait (ARRIVAL, NPacket);
  state WSilence:
    Bus->wait (SILENCE, Transmit);
  state Transmit:
    Bus->transmit (Buffer, XDone);
    Bus->wait (COLLISION, Abort);
  state XDone:
    Bus->stop ();
    Buffer->release ();
    proceed NPacket;
  state Abort:
    Bus->abort ();
    proceed WSilence;
};

UReceiver::perform {
  state WPacket:
    Bus->wait (EMP, Rcvd);
  state Rcvd:
    Client->receive (ThePacket, ThePort);
    skipto (WPacket);
};
#endif
