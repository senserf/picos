#include "types.h"
#include "uprotocl.cc"

identify U-Net2;

Boolean Transmitter::gotPacket () {
  return S->ready (MinPL, MaxPL, FrameL);
};
