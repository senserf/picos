#include "types.h"
#include "uprotocl.cc"

identify H-Net1;

Boolean Transmitter::gotPacket () {
  return S->ready (BusId, MinPL, MaxPL, FrameL);
};
