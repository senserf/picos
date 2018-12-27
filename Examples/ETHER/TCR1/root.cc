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

#include "cobsrvr.cc"

#include "tcrobsrv.cc"

process Root {
  states {Start, Stop};
  perform {
    int NNodes, i, lv;
    Long NMessages;
    TIME BusLength;
    state Start:
      // Time granularity
      readIn (TRate);
      // 1 ETU = 1 bit
      setEtu (TRate);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NNodes);
      readIn (BusLength);
      readIn (SlotLength);
      readIn (GuardDelay);
      // Assuming the above three values are in bits, we convert them
      // to ITUs
      BusLength *= TRate;
      SlotLength *= TRate;
      GuardDelay *= TRate;
      // We should do the same with packet spaces and jamming signals
      TPSpace = (TIME) PSpace * TRate;
      TJamL = (TIME) JamL * TRate;
      initBus (TRate, BusLength, NNodes);
      // Traffic
      initTraffic ();
      // Build the stations
      for (i = 0; i < NNodes; i++) create EtherStation;
      // Processes
      for (i = 0; i < NStations; i++) {
        create (i) Transmitter;
        create (i) Receiver;
      }
      create TCRObserver ();
      // Calculate the maximum number of tournament levels
      for (lv = 1, i = 1; i < NStations; i += i, lv++);
      create CObserver (lv);
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
  };
};
