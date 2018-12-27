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

#include "types.h"
#include "hbus.cc"
#include "utraffic.cc"

static RATE TRate;          // Transmission rate
static double CTolerance;   // Clock tolerance

process Root {
  states {Start, Stop};
  perform {
    int NNodes;
    Long NMessages;
    TIME BusLength;
    state Start:
      // Time granularity
      readIn (TRate);
      // 1 ETU = 1 bit
      setEtu (TRate);
	  readIn (CTolerance);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NNodes);
      readIn (BusLength);
      BusLength *= TRate;   // Convert bus length to ITUs
      initHBus (TRate, BusLength, NNodes);
      // Packetization parameters
      readIn (MinPL);
      readIn (MaxPL);
      readIn (FrameL);
      // Traffic
      initTraffic ();
      // Build the stations
      while (NNodes--) create HStation;
      // Processes
      for (NNodes = 0; NNodes < NStations; NNodes++) {
        // Only one transmitter
        create (NNodes) Transmitter;
        // But still two receivers
        create (NNodes) Receiver (RLBus);
        create (NNodes) Receiver (LRBus);
      }
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
      idToLink (RLBus)->printPfm ("RL-link performance measures");
      idToLink (LRBus)->printPfm ("LR-link performance measures");
  };
};
