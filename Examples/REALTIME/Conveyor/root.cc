#include "operator.h"

#include "sensor.cc"

Process *TheRoot;   // To make Root visible to the operator

process Root {
  states {Init, Start, Stop};
  perform {
    Long nml;
    state Init:
      TheRoot = this;
      initSystem ();
      initBoard ();
      initIo ();
      initOperator ();
      readIn (nml);
      if (nml) {
        // Run on your own
        setLimit (nml);
        signal ((void*)1);
      }
      signal (); // To force immediate start
      wait (SIGNAL, Start);
    state Start:
      startIo ();
      Kernel->wait (DEATH, Stop);
    state Stop:
      outputIo ();
      terminate;
  };
};
