/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

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
