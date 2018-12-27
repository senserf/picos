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
#include "ubus.cc"
#include "utraffic.cc"

static RATE TRate;          // Transmission rate
static double CTolerance;   // Clock tolerance

process Root {
  states {Start, Stop};
  perform {
    int NNodes;
    Long NMessages;
    TIME BusLength;
	DISTANCE TurnLength;
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
	  readIn (TurnLength);
      BusLength *= TRate;   // Convert bus length to ITUs
	  TurnLength *= TRate;
      initUBus (TRate, BusLength, TurnLength, NNodes);
      // Packetization parameters
      readIn (MinPL);
      readIn (MaxPL);
      readIn (FrameL);
      // Traffic
      initTraffic ();
      // Build the stations
      while (NNodes--) create UStation;
      // Processes
      for (NNodes = 0; NNodes < NStations; NNodes++) {
        // Only one transmitter
        create (NNodes) Transmitter;
        // And only one receiver
        create (NNodes) Receiver ();
      }
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
      idToLink (0)->printPfm ("Bus performance measures");
  };
};
