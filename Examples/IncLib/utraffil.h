#ifndef __utrafficl_h__
#define __utrafficl_h__

// Uniform traffic (all stations) with Poisson arrival and exponentially
// distributed message length. The same as utraffic.[ch], except that here
// we calculate message access time individually for each station -- to
// see how fair/unfair the protocol is.

#define MAXSTATIONS 256   // Increase if SMURPH starts complaining

traffic UTraffic {
  void pfmMTR (Packet*);
  exposure;
};

station ClientInterface virtual {
  Packet Buffer;
  RVariable *MAT;
  Boolean ready (Long, Long, Long);
  void configure ();
};

void initTraffic ();
void printLocalMeasures ();

#endif
