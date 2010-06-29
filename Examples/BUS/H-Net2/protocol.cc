#include "types.h"
#include "uprotocl.cc"

identify H-Net2;

Boolean Transmitter::gotPacket () {
  if (S->ready (MinPL, MaxPL, FrameL)) {
    if (S->getId () > Buffer->Receiver)
      Bus = S->Bus [RLBus];
    else
      Bus = S->Bus [LRBus];
    return YES;
  } else
    return NO;
};
