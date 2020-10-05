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
#include "obsrvrs.h"
#include "sring.cc"
#include "utraffic.cc"

static RATE TRate;

process Root {
  states {Start, Stop};
  perform {
    int NNodes;
    Long NMessages;
    TIME RingLength;
    state Start:
      readIn (TRate);
      setEtu (TRate);         // 1 ETU = 1 bit
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NNodes);
      readIn (RingLength);
      RingLength *= TRate;    // Convert from bits
      initSRing (TRate, RingLength, NNodes);
      // Packetization parameters are in 'fddi.h'
      PrTime = PrmbL * TRate;
      readIn (TTRT);
      TTRT *= TRate;          // Convert from bits
      // Traffic
      initTraffic ();
      // Build the stations
      while (NNodes--) create FStation;
      // Processes
      create (0) Starter;
      for (NNodes = 0; NNodes < NStations; NNodes++) {
        create (NNodes) Relay;
        create (NNodes) Receiver;
      }
      readIn (NMessages);
      setLimit (NMessages);
      // Start observers
      create TokenMonitor ();
      create FairnessMonitor (NStations * (TTRT +
        (MaxPL + PrmbL + FrameL + TokL +PrmbL) * TRate));
      Kernel->wait (DEATH, Stop);
    state Stop:
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
  };
};
