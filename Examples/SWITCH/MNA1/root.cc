#include "types.h"
#include "utraffin.cc"

static DISTANCE LinkLength;

int Connected (Long a, Long b, DISTANCE &d) {
  register Long p, i;
  // Calculate the number of ones on which a and b differ
  p = (a | b) & (~a | ~b);
  for (i = 1; i < p; i += i);
  d = LinkLength;
  return i == p;
}

process Root {
  states {Start, Stop};
  perform {
    int NNodes, NPorts, i, j;
    Long NMessages;
    DISTANCE DelayLineLength;
    double CTolerance;
    state Start:
      readIn (TRate);
      setEtu (TRate);         // 1 ETU = 1 bit
      readIn (CTolerance);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NNodes);
      for (NPorts = 1, i = 2; i < NNodes; NPorts++, i += i);
      Assert (i == NNodes, "The number of switches must be a power of two");
      readIn (LinkLength);
      LinkLength *= TRate;    // Convert from bits
      initMesh (TRate, Connected, NNodes);
      readIn (MinPL);         // Packetization
      readIn (MaxPL);
      readIn (FrameL);
      readIn (DelayLineLength);
      DelayLineLength *= TRate;
      // Traffic
      initTraffic ();
      // Build the stations
      for (i = 0; i < NNodes; i++) create Host (NPorts, DelayLineLength);
      assignPortRanks ();
      // Processes
      for (i = 0; i < NNodes; i++) {
        for (j = 0; j < NPorts; j++) {
          create (i) Router (j);
          create (i) InDelay (j);
          create (i) OutDelay (j);
          create (i) Transmitter (j);
        }
      }
      readIn (NMessages);
      setLimit (NMessages);
      Kernel->wait (DEATH, Stop);
    state Stop:
      // Uncomment the line below if you want the network configuration
      // to be printed
      // System->printTop ("Network topology");
      Client->printDef ("Traffic parameters");
      Client->printPfm ();
  };
};
