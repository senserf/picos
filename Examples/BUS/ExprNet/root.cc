#include "types.h"
#include "sbus.cc"
#include "utraffic.cc"

static RATE TRate;          // Transmission rate
static double CTolerance;   // Clock tolerance

process Root {
  states {Start, Stop};
  perform {
    int NNodes, i;
    Long NMessages;
    DISTANCE BusLength, TurnLength;
    EOTMonitor *et;
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
      readIn (TurnLength);
      BusLength *= TRate;   // Convert bus length to ITUs
      TurnLength *= TRate;
      initSBus (TRate, BusLength, TurnLength, NNodes);
      // Packetization parameters
      readIn (MinPL);
      readIn (MaxPL);
      readIn (FrameL);
      readIn (PrmbL);
      readIn (EOTDelay);
      EOTDelay *= TRate;    // Convert to ITUs
      // Traffic
      initTraffic ();
      // Build the stations
      for (i = 0; i < NNodes; i++) create ExStation;
      // Processes
      for (i = 0; i < NStations; i++) {
        // Note: alternatively EOTMonitor can be defined at station 0 only
        et = create (i) EOTMonitor;
        if (i == 0) et->signal ();
        create (i) Transmitter (et);
        create (i) Receiver ();
      }
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
      idToLink (0)->printPfm ("Bus performance measures");
  };
};
