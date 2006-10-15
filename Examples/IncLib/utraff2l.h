#ifndef __utraffic2l_h__
#define __utraffic2l_h__

// Uniform traffic (all stations) with Poisson arrival and exponentially
// distributed message length. Two packet buffers per station, each buffer
// for packets going in one direction. The same as utraffi2.[ch], except
// that here we calculate message access time individually for each station
// -- to see how fair/unfair the protocol is.

#define MAXSTATIONS 256   // Increase if SMURPH starts complaining 

traffic UTraffic {
  void pfmMTR (Packet*);
  exposure;
};

#define Right 0    // Directions
#define Left 1

station ClientInterface virtual {
  Packet Buffer [2];
  RVariable *MAT;
  Boolean ready (int, Long, Long, Long);
  void configure ();
};

void initTraffic ();
void printLocalMeasures ();

#endif
