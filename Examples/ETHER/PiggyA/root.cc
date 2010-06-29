#include "types.h"

#include "lbus.cc"

#include "utraffic.cc"

static int NNodes;

void PiggyStation::setup () {
  int i;
  PiggyStation *ps;
  EtherStation::setup ();
  if (getId () == NNodes - 1) {
    // The last station
    for (i = 0; i < NNodes; i++) {
      ps = (PiggyStation*) idToStation (i);
      ps->RDist = (DISTANCE) duToItu (ps->Bus->distTo (Bus));
    }
  }
  LDist = (DISTANCE) duToItu (((PiggyStation*) idToStation (0)) -> Bus ->
	distTo (Bus));
  Ready = create Mailbox (0);  // Note that this must be a capacity zero mailbox
  Blocked = NO;
};

process Root {
  states {Start, Stop};
  perform {
    int i;
    Long NMessages;
    state Start:
      // Time granularity
      readIn (TRate);
      // 1 ETU = 1 bit
      setEtu (TRate);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NNodes);
      readIn (L);
      readIn (DelayQuantum);
      // Assuming the above three values are in bits, we convert them
      // to ITUs
      L *= TRate;
      DelayQuantum *= TRate;
      // We should do the same with packet spaces and jamming signals
      TPSpace = (TIME) PSpace * TRate;
      TJamL = (TIME) JamL * TRate;
      initBus (TRate, L, NNodes);
      // Traffic
      readIn (MinIPL);
      readIn (MinUPL);
      readIn (MaxUPL);
      readIn (PFrame);
      initTraffic ();
      // Build the stations
      for (i = 0; i < NNodes; i++) create PiggyStation;
      // Processes
      for (i = 0; i < NStations; i++) {
        create (i) Mtr;
        create (i) Transmitter;
        create (i) Receiver;
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
