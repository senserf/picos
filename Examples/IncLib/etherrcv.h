#ifndef __etherreceiver_h__
#define __etherreceiver_h__

// This is the standard simple Ethernet receiver used in all collision
// protocols (and a few other ones as well).

process Receiver (EtherStation) {
  Port *Bus;       // A copy of the bus port
  void setup () { Bus = S->Bus; };
  states {WPacket, Rcvd};
  perform {
    state WPacket:
      Bus->wait (EMP, Rcvd);
    state Rcvd:
      assert (ThePacket->isMy (), "Receiver: not my packet");
      Client->receive (ThePacket, ThePort);
      skipto (WPacket);
  };
};

#endif
