#include "types.h"
#include "hbus.cc"
#include "utraff2l.cc"

static RATE TRate;          // Transmission rate
static double CTolerance;   // Clock tolerance

process Root {
  states {Start, Stop};
  perform {
    int NNodes;
    Long NMessages;
    TIME BusLength;
    state Start:
      // Time granularity
      readIn (TRate);
      // 1 ETU = 1 bit
      setEtu (TRate);
      readIn (CTolerance);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NNodes);
      readIn (BusLength);
      BusLength *= TRate;   // Convert bus length to ITUs
      initHBus (TRate, BusLength, NNodes);
      // Packetization parameters
      readIn (MinPL);
      readIn (MaxPL);
      readIn (FrameL);
      // Traffic
      initTraffic ();
      // Build the stations
      while (NNodes--) create HStation;
      // Processes
      for (NNodes = 0; NNodes < NStations; NNodes++) {
        create (NNodes) Transmitter (RLBus);
        create (NNodes) Transmitter (LRBus);
        create (NNodes) Receiver (RLBus);
        create (NNodes) Receiver (LRBus);
      }
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
      printLocalMeasures ();
      idToLink (RLBus)->printPfm ("RL-link performance measures");
      idToLink (LRBus)->printPfm ("LR-link performance measures");
  };
};
