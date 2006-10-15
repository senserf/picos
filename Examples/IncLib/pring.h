#ifndef __pring_h__
#define __pring_h__

// Type announcements for 'pring.cc': Pretzel Ring configuration consisting of
// two segments.

station PRingInterface virtual {
  Port *IRing [2], *ORing [2];
  void configure ();
};

station PMonitorInterface virtual {
  Port *IRing, *ORing;
  void configure ();
};

void initPRing (RATE, TIME, int);

#endif
