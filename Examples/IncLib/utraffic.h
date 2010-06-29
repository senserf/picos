#ifndef __utraffic_h__
#define __utraffic_h__

// Uniform traffic (all stations) with Poisson arrival and exponentially
// distributed message length

station ClientInterface virtual {
  Packet Buffer;
  Boolean ready (Long, Long, Long);
  void configure ();
};

void initTraffic ();

#endif
