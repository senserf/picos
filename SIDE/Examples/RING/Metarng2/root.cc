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
#include "mring.cc"
#include "utraffi2.cc"

process Root {
  states {Start, Stop};
  perform {
    int NNodes, i, j;
    Long NMessages, BufferLength;
    TIME RingLength;
    state Start:
      setEtu (1.0);         // 1 ETU = 1 ITU = 1 bit
      setTolerance (0.0);
      // Configuration parameters
      readIn (NNodes);
      readIn (RingLength);
      initMRing (1, RingLength, NNodes);
      // Packetization parameters
      readIn (MinPL);
      readIn (MaxPL);
      readIn (FrameL);
      readIn (HdrL);
      readIn (SATLength);
      readIn (SATTimeout);
      readIn (K);
      readIn (L);
      readIn (BufferLength);
      // Traffic
      initTraffic ();
      // Build the stations
      for (i = 0; i < NNodes; i++) create MStation (BufferLength);
      // Processes
      for (i = 0; i < NNodes; i++) {
        Transmitter *tr;
        SATSender *ss;
        Input *in;
        SATReceiver *sr;
        for (j = 0; j < 2; j++) {
          tr = create (i) Transmitter (j);
          in = create (i) Input (j);
          create (i) Relay (j);
          ss = create (i) SATSender (1-j);
          sr = create (i) SATReceiver (1-j);
          tr -> SATSnd = ss;
          in -> Xmitter = tr;
          sr -> SATSnd = ss;
          ss -> SATRcv = sr;
          ss -> Xmitter = tr;
          if (i == 0) {
            *(ss->SATFlag) = YES;
            ss->signal ();
          }
        }
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
