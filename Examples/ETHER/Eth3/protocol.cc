identify Ethernet3;

#include "types.h"

TIME Transmitter::backoff () {
  return (TIME) (TwoL * toss (1 << (CCounter > 10 ? 10 : CCounter)));
};

Transmitter:: perform {
  TIME  LSTime, IPeriod;
  state NPacket:
    CCounter = 0;
    if (S->ready (MinPL, MaxPL, FrameL))
      proceed Retry;
    else
      Client->wait (ARRIVAL, NPacket);
  state Retry:
    if (undef (LSTime = Bus->lastEOA ())) {
      // The bus, as perceived by the station, is busy
      Bus->wait (SILENCE, Retry);
    } else {
      // Check if the packet space has been obeyed
      if ((IPeriod = Time - LSTime) >= PSpace)
        proceed (Xmit);
      else
        // Space too short: wait for the remainder
        Timer->wait ((TIME) PSpace - IPeriod, Xmit);
    }
  state Xmit:
    Bus->transmit (Buffer, XDone);
    Bus->wait (COLLISION, XAbort);
  state XDone:
    // Successfull transfer
    Bus->stop ();
    Buffer->release ();
    proceed (NPacket);
  state XAbort:
    // A collision
    Bus->abort ();
    CCounter++;
    // Send a jamming signal
    Bus->sendJam (JamL, JDone);
  state JDone:
    Bus->stop ();
    Timer->wait (backoff (), Retry);
};
