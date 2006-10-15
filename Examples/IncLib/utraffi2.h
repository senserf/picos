#ifndef __utraffic2_h__
#define __utraffic2_h__

// Uniform traffic (all stations) with Poisson arrival and exponentially
// distributed message length. Two packet buffers per station, each buffer
// for packets going in one direction.

#define Right 0    // Directions
#define Left 1

station ClientInterface virtual {
  Packet Buffer [2];
  Boolean ready (int, Long, Long, Long);
  void configure ();
};

void initTraffic ();

#endif
