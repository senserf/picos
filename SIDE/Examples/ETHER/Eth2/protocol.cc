/*
	Copyright 1995-2020 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/
identify Ethernet2;

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

Receiver::perform {
  state WPacket:
    Bus->wait (EMP, Rcvd);
  state Rcvd:
    assert (ThePacket->isMy (), "Receiver: not my packet");
    Client->receive (ThePacket, ThePort);
    skipto (WPacket);
};
