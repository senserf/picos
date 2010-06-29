#include "types.h"
#include "pring.cc"
#include "utraffic.cc"

process Root {
  PMonitor *PM;
  states {Start, Running, Stop};
  perform {
    int NNodes, i, j;
    Long NMessages;
    TIME RingLength;
    RATE TRate;
    double CTolerance;
    SlotGen *SG;
    state Start:
      readIn (TRate);
      setEtu (TRate);
      readIn (CTolerance);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NNodes);
      // This is the length of a single segment in bits
      readIn (RingLength);
      RingLength *= TRate;
      initPRing (TRate, RingLength, NNodes);
      // Packetization parameters
      readIn (SlotML);      // Slot marker length in bits
      readIn (SegmPL);      // Segment payload length in bits
      readIn (SegmFL);      // Segment header and trailer
      readIn (SegmWindow);  // Segment window size in bits
      SegmWindow *= TRate;  // Convert to ITUs
      readIn (THT);         // Token holding interval per station
      TZLength = THT * NNodes;
      // Traffic
      initTraffic ();
      readIn (NMessages);
      setLimit (NMessages);
      // Build the stations
      for (i = 0; i < NNodes; i++) create PStation;
      PM = create PMonitor;
      Client->suspend ();   // No traffic during initialization
      // Processes
      for (i = 0; i < NNodes; i++) {
        for (j = 0; j < 2; j++) create (i) Relay (j);
      }
      SG = create (PM) SlotGen;
      create (PM) IConnector;
      SG->wait (DEATH, Running);
    state Running:
      create (PM) OConnector;
      Client->resume ();
      Kernel->wait (DEATH, Stop);
    state Stop:
      System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
  };
};
