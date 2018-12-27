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

#include "lbus.cc"

#include "utraffic.cc"

static int NNodes;

void PiggyStation::setup () {
  EtherStation::setup ();
  Ready = create Mailbox (0);  // Note that this must be a capacity zero mailbox
  Blocked = NO;
};

process Root {
  states {Start, Stop};
  perform {
    int i;
    Long NMessages;
    state Start:
      // Time granularity
      readIn (TRate);
      // 1 ETU = 1 bit
      setEtu (TRate);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NNodes);
      readIn (L);
      readIn (DelayQuantum);
      // Assuming the above three values are in bits, we convert them
      // to ITUs
      L *= TRate;
      DelayQuantum *= TRate;
      // We should do the same with packet spaces and jamming signals
      TPSpace = (TIME) PSpace * TRate;
      TJamL = (TIME) JamL * TRate;
      initBus (TRate, L, NNodes);
      // Traffic
      readIn (MinIPL);
      readIn (MinUPL);
      readIn (MaxUPL);
      readIn (PFrame);
      initTraffic ();
      // Build the stations
      for (i = 0; i < NNodes; i++) create PiggyStation;
      // Processes
      for (i = 0; i < NStations; i++) {
        create (i) Mtr;
        create (i) Transmitter;
        create (i) Receiver;
      }
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
  };
};
