#ifndef __uprotocol_h__
#define __uprotocol_h__

// This file announces the generic transmitter and receiver types for H-Net
// (versions 1 and 2) and U-Net

extern Long MinPL, MaxPL, FrameL;    // Read from the input file

process UTransmitter abstract {
  Port *Bus;
  Packet *Buffer;
  virtual Boolean gotPacket () = 0;
  states {NPacket, WSilence, Transmit, XDone, Abort};
  perform;
};

process UReceiver abstract {
  Port *Bus;
  states {WPacket, Rcvd};
  perform;
};

#endif
