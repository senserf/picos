#ifndef __hbus_h__
#define __hbus_h__

// Type announcements for 'hbus.cc': a dual unidirectional bus with two ports
// per station

#define LRBus 0        // The number of the left-to-right bus
#define RLBus 1        // The number of the right-to-left bus

station HBusInterface virtual {
  Port *Bus [2];
  void configure ();
};

void initHBus (RATE, TIME, int);

#endif
