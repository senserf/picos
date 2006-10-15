#include "types.h"

#include "lbus.cc"

#include "utraffic.cc"

process Root {
  states {Start, Stop};
  perform {
    int NNodes;
    Long NMessages;
    TIME BusLength;
    state Start:
      setEtu (1);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NNodes);
      readIn (BusLength);
	  // The bus
      initBus (TRate, BusLength, NNodes, PSpace + 10);
      // Traffic
      initTraffic ();
      // Build the stations
      while (NNodes--) create EtherStation;
      // Processes
      for (NNodes = 0; NNodes < NStations; NNodes++) {
        create (NNodes) Transmitter;
        create (NNodes) Receiver;
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
