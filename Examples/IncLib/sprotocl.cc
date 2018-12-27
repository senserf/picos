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

