#include "types.h"
#include "utraffin.cc"

static DISTANCE LinkLength;
static int NCols, NRows;    // Number of columns/rows

#define NPorts 2

int Connected (Long a, Long b, DISTANCE &d) {
  // Determines if there is a link from a to b
  Long ra, ca, rb, cb, t;
  d = LinkLength;
  ra = row (a); ca = col (a);
  rb = row (b); cb = col (b);
  if (ra == rb) {
    if (odd (ra)) {
      t = ca; ca = cb; cb = t;
    }
    ca -= cb;
    return ca == 1 || ca == 1 - NCols;
  } else if (ca == cb) {
    if (odd (ca)) {
      t = ra; ra = rb; rb = t;
    }
    ra -= rb;
    return ra == 1 || ra == 1 - NRows;
  } else
    return NO;
};

process Root {
  states {Start, Stop};
  perform {
    int NNodes, i, j;
    Long NMessages;
    DISTANCE DelayLineLength;
    double CTolerance;
    state Start:
      readIn (TRate);
      setEtu (TRate);         // 1 ETU = 1 bit
      readIn (CTolerance);
      setTolerance (CTolerance);
      // Configuration parameters
      readIn (NCols);         // Number of columns
      readIn (NRows);         // Number of rows
      Assert (evn (NCols) && evn (NRows),
                     "The number of rows/columns must be even");
      NNodes = NCols * NRows;
      readIn (LinkLength);
      LinkLength *= TRate;    // Convert from bits
      initMesh (TRate, Connected, NNodes);
      readIn (MinPL);       // Packetization
      readIn (MaxPL);
      readIn (FrameL);
      readIn (DelayLineLength);
      DelayLineLength *= TRate;
      // Traffic
      initTraffic ();
      // Build the stations
      for (i = 0; i < NNodes; i++)
        if (evn (col (i)) && evn (row (i)))
          create Host (NPorts, DelayLineLength);
        else
          create Switch (NPorts);
      assignPortRanks ();
      // Processes
      for (i = 0; i < NNodes; i++) {
        if (evn (col (i)) && evn (row (i))) {
          for (j = 0; j < NPorts; j++) {
            create (i) Router (j);
            create (i) InDelay (j);
            create (i) OutDelay (j);
            create (i) Transmitter (j);
          }
        } else {
          for (j = 0; j < NPorts; j++)
            create (i) Router (j);
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
