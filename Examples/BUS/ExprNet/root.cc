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

#include "types.h"
#include "sbus.cc"
#include "utraffic.cc"

static RATE TRate;          // Transmission rate
static double CTolerance;   // Clock tolerance

process Root {
  states {Start, Stop};
  perform {
    int NNodes, i;
    Long NMessages;
    DISTANCE BusLength, TurnLength;
    EOTMonitor *et;
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
      initSBus (TRate, BusLength, TurnLength, NNodes);
      // Packetization parameters
      readIn (MinPL);
      readIn (MaxPL);
      readIn (FrameL);
      readIn (PrmbL);
      readIn (EOTDelay);
      EOTDelay *= TRate;    // Convert to ITUs
      // Traffic
      initTraffic ();
      // Build the stations
      for (i = 0; i < NNodes; i++) create ExStation;
      // Processes
      for (i = 0; i < NStations; i++) {
        // Note: alternatively EOTMonitor can be defined at station 0 only
        et = create (i) EOTMonitor;
        if (i == 0) et->signal ();
        create (i) Transmitter (et);
        create (i) Receiver ();
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
