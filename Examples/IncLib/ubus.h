#ifndef __ubus_h__
#define __ubus_h__

// Type announcements for 'ubus.cc': a U-shaped, folded unidirectional bus
// with two ports per station

station UBusInterface virtual {
  Port *IBus, *OBus;
  void configure ();
};

void initUBus (RATE, TIME, DISTANCE, int);

#endif
