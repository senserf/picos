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

#include "fddi.h"
#include "sring.h"
#include "utraffic.h"

#define TOKEN NONE   // Token packet type

extern TIME TTRT,    // Target token rotation time
            PrTime;  // Preamble insertion time

station FStation : SRingInterface, ClientInterface {
  TIME TRT, THT;
  void setup () {
    SRingInterface::configure ();
    ClientInterface::configure ();
    TRT = THT = TIME_0;
  };
};

process Transmitter (FStation) {
  Port *ORing;
  TIME TStarted;
  void setup () { ORing = S->ORing; };
  states {Xmit, PDone, EXmit};
  perform;
};

process Relay (FStation) {
  Port *IRing, *ORing;
  Packet *Relayed;
  void setup () {
    IRing = S->IRing;
    ORing = S->ORing;
    Relayed = create Packet;
  };
  states {Mtr, SPrm, EPrm, Frm, WFrm, EFrm, MyTkn, IgTkn, PsTkn, PDone, TDone};
  perform;
};

process Receiver (FStation) {
  Port *IRing;
  void setup () { IRing = S->IRing; };
  states {WPacket, Rcvd};
  perform;
};

process Starter (FStation) {
  Port *ORing;
  Packet *Token;
  void setup () { ORing = S->ORing; };
  states {Start, PDone, Stop};
  perform;
};
