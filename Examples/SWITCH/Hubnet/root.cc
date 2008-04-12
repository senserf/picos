#include "types.h"
#include "hub.cc"
#include "utraffic.cc"

static RATE TRate;
static double CTolerance;

process Root {
  states {Start, Stop};
  perform {
    int NNodes;
    Long NMessages;
    TIME MeanLinkLength;
    state Start:
      readIn (TRate);
      setEtu (TRate);         // 1 ETU = 1 bit
      readIn (CTolerance);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NNodes);
      readIn (MeanLinkLength);
      MeanLinkLength *= TRate;
      initHub (TRate, MeanLinkLength, NNodes);
      // Packetization parameters
      readIn (MinPL);
      readIn (MaxPL);
      readIn (FrameL);
      readIn (SndRecTime);   // Sender recognition delay
      SndRecTime *= TRate;   // Convert to ITU
      // Traffic
      initTraffic ();
      // Build the stations
      while (NNodes--) create HStation;
      create Hub;
      // Processes
      for (NNodes = 0; NNodes < NStations-1; NNodes++) {
        create (NNodes) Transmitter;
        create (NNodes) Receiver;
        create (NNodes) Supervisor;
        create (NStations-1) HubProcess (NNodes);
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
