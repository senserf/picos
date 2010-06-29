#ifndef __sprotocl_h__
#define __sprotocl_h__

// This is the header file for the generic transmitter/receiver code for
// slotted protocols (used in Fasnet and DQDB)

process STransmitter {
  Port *Bus;
  Packet *Buffer;
  Mailbox *Strobe;
  virtual Boolean gotPacket () { return NO; };
  states {NPacket, Transmit, XDone, Error};
  perform;
};

process SReceiver {
  Port *Bus;
  states {WPacket, Rcvd};
  perform;
};

#endif

