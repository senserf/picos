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

#include "ether.h"
#include "lbus.h"
#include "utraffic.h"

#include "etherstn.h"

#include "etherrcv.h"

#define TRate 1       // Transmission rate: 1 ITU / bit

process Transmitter (EtherStation) {
  int CCounter;       // Collision counter
  Port *Bus;          // A copy of the bus port
  Packet *Buffer;     // Packet buffer pointer
  TIME backoff ();    // The standard backoff function
  void setup () {
    Bus = S->Bus;
    Buffer = &(S->Buffer);
  };
  states {NPacket, Retry, Xmit, XDone, XAbort, JDone};
  perform;
};
