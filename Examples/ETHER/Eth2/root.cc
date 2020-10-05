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

process Root {
  states {Start, Stop};
  perform {
    int n;
    Long NMessages;
    TIME BusLength;
    state Start:
      setEtu (1);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (n);
      readIn (BusLength);
      initBus (TRate, BusLength, n, PSpace + 10);
      // Traffic
      initTraffic ();
      // Build the stations
      while (n--) create EtherStation;
      // Processes
      for (n = 0; n < NStations; n++) {
        create (n) Transmitter;
        create (n) Receiver;
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
