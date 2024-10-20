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
#include "sring.cc"
#include "utraffic.cc"

process Root {
  states {Start, Stop};
  perform {
    int NNodes;
    Long NMessages, BufferLength;
    TIME RingLength;
    state Start:
      setEtu (1.0);         // 1 ETU = 1 ITU = 1 bit
      setTolerance (0.0);
      // Configuration parameters
      readIn (NNodes);
      readIn (RingLength);
      initSRing (1, RingLength, NNodes);
      // Packetization parameters
      readIn (MinPL);
      readIn (MaxPL);
      readIn (FrameL);
      readIn (HdrL);
      readIn (BufferLength);
      // Traffic
      initTraffic ();
      // Build the stations
      while (NNodes--) create IStation (BufferLength);
      // Processes
      for (NNodes = 0; NNodes < NStations; NNodes++) {
        Transmitter *pr;
        pr = create (NNodes) Transmitter;
        create (NNodes) Input (pr);
        create (NNodes) Relay;
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
