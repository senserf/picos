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
