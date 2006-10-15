#include "types.h"
#include "mring.cc"
#include "utraffi2.cc"

process Root {
  states {Start, Stop};
  perform {
    int NNodes, i;
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
      readIn (BufferLength);
      // Traffic
      initTraffic ();
      // Build the stations
      while (NNodes--) create MStation (BufferLength);
      // Processes
      for (NNodes = 0; NNodes < NStations; NNodes++) {
        Transmitter *pr;
        for (i = 0; i < 2; i++) {
          pr = create (NNodes) Transmitter (i);
          create (NNodes) Input (i, pr);
          create (NNodes) Relay (i);
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
