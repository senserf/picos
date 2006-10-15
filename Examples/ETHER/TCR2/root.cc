#include "types.h"

#include "lbus.cc"

#include "utraffic.cc"

#include "tcrobsrv.cc"

#include "cobsrvr.cc"

process Root {
  states {Start, Stop};
  perform {
    int NNodes;
    Long NMessages;
    TIME BusLength;
    state Start:
      int lv, i;
      // Time granularity
      readIn (TRate);
      // 1 ETU = 1 bit
      setEtu (TRate);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NNodes);
      readIn (BusLength);
      readIn (SlotLength);
      readIn (GuardDelay);
      // Assuming the above three values are in bits, we convert them
      // to ITUs
      BusLength *= TRate;
      SlotLength *= TRate;
      GuardDelay *= TRate;
      // We should do the same with packet spaces and jamming signals
      TPSpace = (TIME) PSpace * TRate;
      TJamL = (TIME) JamL * TRate;
      initBus (TRate, BusLength, NNodes);
      // Traffic
      initTraffic ();
      // Build the stations
      while (NNodes--) create EtherStation;
      // Processes
      for (NNodes = 0; NNodes < NStations; NNodes++) {
        create (NNodes) Transmitter;
        create (NNodes) Receiver;
      }
      create TCRObserver ();
      // Calculate the maximum number of tournament levels
      for (lv = 1, i = 1; i < NStations; i += i, lv++);
      create CObserver (lv);
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
  };
};
