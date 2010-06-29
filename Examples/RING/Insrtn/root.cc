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
