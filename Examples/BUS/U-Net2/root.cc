#include "types.h"
#include "ubus.cc"
#include "utraffil.cc"

static RATE TRate;          // Transmission rate
static double CTolerance;   // Clock tolerance

process Root {
  states {Start, Stop};
  perform {
    int NNodes;
    Long NMessages;
    TIME BusLength;
	DISTANCE TurnLength;
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
      initUBus (TRate, BusLength, TurnLength, NNodes);
      // Packetization parameters
      readIn (MinPL);
      readIn (MaxPL);
      readIn (FrameL);
      // Traffic
      initTraffic ();
      // Build the stations
      while (NNodes--) create UStation;
      // Processes
      for (NNodes = 0; NNodes < NStations; NNodes++) {
        // Only one transmitter
        create (NNodes) Transmitter;
        // And only one receiver
        create (NNodes) Receiver ();
      }
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
      // Note: the following function is defined in utraffil.[ch]
	  printLocalMeasures ();
      idToLink (0)->printPfm ("Bus performance measures");
  };
};
