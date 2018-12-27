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
#include "utraffin.cc"

#include "mesh.cc"

static DISTANCE LinkLength;

#define NPorts 2

int Connected (Long a, Long b, DISTANCE &d) {
  // Determines if there is a link from a to b
  Long ra, ca, rb, cb, t;
  d = LinkLength;
  ra = row (a); ca = col (a);
  rb = row (b); cb = col (b);
  if (ra == rb) {
    if (odd (ra)) {
      t = ca; ca = cb; cb = t;
    }
    cb -= ca;
    return cb == 1 || cb == 1 - NCols;
  } else if (ca == cb) {
    if (odd (ca)) {
      t = ra; ra = rb; rb = t;
    }
    rb -= ra;
    return rb == 1 || rb == 1 - NRows;
  } else
    return NO;
};

process Root {
  states {Start, Stop};
  perform {
    Long NNodes, i, j;
    Long NMessages;
    double CTolerance;
    state Start:
      readIn (TRate);
      setEtu (TRate);         // 1 ETU = 1 bit
      readIn (CTolerance);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NCols);         // Number of columns
      readIn (NRows);         // Number of rows
      Assert (evn (NCols) && evn (NRows),
                     "The number of rows/columns must be even");
      NCols05 = NCols / 2;
      NRows05 = NRows / 2;
      NCols15 = NCols + NCols05;
      NRows15 = NRows + NRows05;
      NNodes = NCols * NRows;
      readIn (LinkLength);
      LinkLength *= TRate;    // Convert from bits
      initMesh (TRate, Connected, NNodes);
      // Packetization parameters
      readIn (SlotML);
      readIn (SegmPL);
      readIn (SegmFL);
      readIn (SegmWindow);
      SegmWindow *= TRate;  // Convert from bits
      // Traffic
      initTraffic ();
      // Build the stations
      for (i = 0; i < NNodes; i++) create MStation;
      // Processes
      for (i = 0; i < NStations; i++) {
        for (j = 0; j < 2; j++) {
          create (i) Input ((int) j);
          create (i) SlotGen ((int) j);
        }
        create (i) Router;
      }
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      // Uncomment the line below if you want the network configuration
      // to be printed
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
  };
};
