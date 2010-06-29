#include "types.h"
#include "uprotocl.cc"

identify U-Net3;

Boolean Transmitter::gotPacket () {
  return S->ready (MinPL, MaxPL, FrameL);
};
