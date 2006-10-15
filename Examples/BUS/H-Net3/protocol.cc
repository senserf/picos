#include "types.h"
#include "uprotocl.cc"

identify H-Net3;

Boolean Transmitter::gotPacket () {
  return S->ready (BusId, MinPL, MaxPL, FrameL);
};
