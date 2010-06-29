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
