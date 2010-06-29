#ifndef __sbus_h__
#define __sbus_h__

// Type announcements for 'sbus.cc': a single S-shaped unidirectional bus with
// two ports per station.

station SBusInterface virtual {
  Port *OBus,    // Output port
       *IBus;    // Input port
  void configure ();
};

void initSBus (RATE, DISTANCE, DISTANCE, int);

#endif
