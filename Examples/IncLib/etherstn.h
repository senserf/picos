#ifndef __etherstation_h__
#define __etherstation_h__

// This virtual type defines a station component used to interface the station
// to a single bus (via a single port).

station EtherStation : BusInterface, ClientInterface { 
  void setup () {
    BusInterface::configure ();
    ClientInterface::configure ();
  };
};

#endif
