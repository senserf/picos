#ifndef __utrafficn_h__
#define __utrafficn_h__

// Uniform traffic (all stations) with Poisson arrival and exponentially
// distributed message length. Each station has the requested number of
// buffers which are filled independently. This traffic pattern is used
// in mesh networks (e.g., MNA) in which a single station runs a number
// of independent transmitters.


// Note: If you want to use non-standard message/packet types, just make
// sure that the two constants below are defined before this file is
// included.

#ifndef PACKET_TYPE
#define PACKET_TYPE Packet
#endif

#ifndef MESSAGE_TYPE
#define MESSAGE_TYPE Message
#endif

station ClientInterface virtual {
  PACKET_TYPE **Buffer;
  Boolean ready (int, Long, Long, Long);
  void configure (int);
};

void initTraffic (int fml = NO);

#endif
