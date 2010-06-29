#include "types.h"
#include "uprotocl.cc"

identify U-Net1;

Boolean Transmitter::gotPacket () {
  return S->ready (MinPL, MaxPL, FrameL);
};
