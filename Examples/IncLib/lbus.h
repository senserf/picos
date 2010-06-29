#ifndef __lbus_h__
#define __lbus_h__

// Type announcements for 'lbus.cc': a strictly linear bus with stations spaced
// equally along it.

station BusInterface virtual {
  Port *Bus;
  void configure ();
};

void initBus (RATE, DISTANCE, int, TIME at);

// Note: the following declaration is only for the ATT compiler which doesn't
// accept this: void initBus (RATE, TIME, int, TIME at = TIME_0);

inline void initBus (RATE r, DISTANCE l, int n) {
  initBus (r, l, n, TIME_0);
};

#endif
