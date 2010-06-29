#ifndef __sring_h__
#define __sring_h__

// Type announcements for 'sring.cc': a sngle unidirectional ring with stations
// equally spaced along it.

station SRingInterface virtual {
  Port *IRing, *ORing;
  void configure ();
};

void initSRing (RATE, TIME, int);

#endif
