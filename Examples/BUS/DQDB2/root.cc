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
#include "hbus.cc"
#include "fstrfl.cc"

static RATE TRate;          // Transmission rate

process Root {
  states {Start, Stop};
  perform {
    int NNodes, i;
    Long NMessages;
    TIME BusLength;
    Transmitter *pr;
    state Start:
      // Time granularity
      readIn (TRate);
      // 1 ETU = 1 bit
      setEtu (TRate);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NNodes);
      readIn (BusLength);
      BusLength *= TRate;           // Convert bus length to ITUs
      initHBus (TRate, BusLength, NNodes);
      SegmWindow = SegmWL * TRate;  // Convert to ITUs
      // Traffic
      initTraffic ();
      // Build the stations
      for (i = 0; i < NNodes; i++)
        if (i == 0 || i == NNodes - 1)
          create HeadEnd;
        else
          create HStation;
      // Processes
      for (i = 0; i < NStations; i++) {
        pr = create (i) Transmitter;
        create (i) Strober (LRBus, pr);
        create (i) Strober (RLBus, pr);
        create (i) Receiver (RLBus);
        create (i) Receiver (LRBus);
        if (i == 0) create (i) SlotGen (LRBus);
        if (i == NStations-1) create (i) SlotGen (RLBus);
      }
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
      printLocalMeasures ();
      idToLink (RLBus)->printPfm ("RL-link performance measures");
      idToLink (LRBus)->printPfm ("LR-link performance measures");
  };
};
