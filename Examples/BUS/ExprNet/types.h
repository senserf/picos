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

#include "sbus.h"
#include "utraffic.h"

extern Long MinPL,    // Minimum packet length (payload)
            MaxPL,    // Maximum packet length
            FrameL,   // Header and trailer (preamble excluded)
            PrmbL;    // Preamble length

extern TIME EOTDelay; // Amount of silence needed to detect the end of train 

station ExStation : SBusInterface, ClientInterface {
  Packet Preamble;
  void setup () {
    SBusInterface::configure ();
    ClientInterface::configure ();
    Preamble.fill (NONE, NONE, PrmbL);
  };
};

process EOTMonitor (ExStation) {
  Port *Bus;
  void setup () {
    Bus = S->IBus;
  };
  states {Wait, Count, Signal, Retry};
  perform;
};

process Transmitter (ExStation) {
  Packet *Preamble, *Buffer;
  Port *Bus;
  EOTMonitor *EOTrain;
  void setup (EOTMonitor *et = NULL) {
    Bus = S->OBus;
    Buffer = &(S->Buffer);
    Preamble = &(S->Preamble);
    EOTrain = et;
  };
  states {Wait, CheckBuf, Transmit, Yield, PDone, XDone, Error};
  perform;
};

process Receiver (ExStation) {
  Port *Bus;
  void setup () {
    Bus = S->IBus;
  };
  states {WPacket, Rcvd};
  perform;
};
