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
identify "FLOODNET-Torus";

#include "utraffic.cc"
#include "fldswtch.cc"

#define NSPorts 5    // The number of port pairs at a switch

static DISTANCE SwitchLinkLength, HostLinkLength;
static int NSwitches;
static int D;        // The row/column length (the square root of NSwitches)

int Connected (Long a, Long b, DISTANCE &d) {
  Long ra, ca, rb, cb;
  if (a < NSwitches && b < NSwitches) {
    d = SwitchLinkLength;
    ra = a / D; ca = a % D;
    rb = b / D; cb = b % D;
    if (ra == rb) {
      if ((ca -= cb) < 0) ca = -ca;
      return ca == 1 || ca == D - 1;
    } else if (ca == cb) {
      if ((ra -= rb) < 0) ra = -ra;
      return ra == 1 || ra == D - 1;
    } else
      return NO;
  } else {
    d = HostLinkLength;
    return a == b + NSwitches || b == a + NSwitches;
  }
}

process Root {
  states {Start, Stop};
  perform {
    int i, j;
    Long NMessages;
    double CTolerance;
    state Start:
      readIn (TRate);
      setEtu (TRate);         // 1 ETU = 1 bit
      readIn (CTolerance);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NSwitches);
      for (D = 1; D * D < NSwitches; D++);
      Assert (D * D == NSwitches, "The number of switches must be a square");
      readIn (SwitchLinkLength);
      SwitchLinkLength *= TRate;    // Convert from bits
      readIn (HostLinkLength);
      HostLinkLength *= TRate;      // Convert from bits
      initMesh (TRate, Connected, NSwitches + NSwitches);
      readIn (MinPL);         // Packetization parameters
      readIn (MaxPL);
      readIn (FrameL);
      readIn (RcvRecDelay);   // Receiver recognition time in bits
      RcvRecDelay *= TRate;   // Convert to ITUs
      readIn (PSpace);        // Packet space in bits
      PSpace *= TRate;        // Convert to ITUs
      readIn (NoiseL);        // The length of noise packet
      readIn (MinActTime);    // Minimum duration of a recognizable activity
      readIn (FloodTime);     // Network flooding time (for backoff)
      MinActTime *= TRate;
      FloodTime *= TRate;
      // Traffic
      initTraffic ();
      // Build the stations
      for (i = 0; i < NSwitches; i++) create Switch (NSPorts);
      for (i = 0; i < NSwitches; i++) create Host;
      // Processes
      for (i = 0; i < NSwitches; i++)
        for (j = 0; j < NSPorts; j++)
          create (i) PortServer (j);
      for (i = NSwitches; i < NSwitches + NSwitches; i++) {
        create (i) Receiver;
        create (i) Transmitter;
      }
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      // Uncomment the line below if you want the network configuration
      // to be printed
      // System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
  };
};
