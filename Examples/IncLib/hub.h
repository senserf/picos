#ifndef __hub_h__
#define __hub_h__

// Type announcements for 'hub.cc': a Hubnet configuration consisting of a
// single hub. 

station HubInterface virtual {
  Port *SPort,    // Selection port
       *BPort;    // Broadcast port
  void configure ();
};

station HubStation virtual {
  Port **SPorts,  // Array of selection ports
        *BPort;
  void configure ();
};

void initHub (RATE, TIME, int);

#endif
