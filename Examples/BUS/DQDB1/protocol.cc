#include "types.h"
#include "sprotocl.cc"

identify DQDB1;
            
TIME SegmWindow; // Segment window length in ITUs

Boolean Transmitter::gotPacket () {
  if (S->ready (BusId, SegmPL, SegmPL, SegmFL)) {
    *CD = *RQ;
    *RQ = 0;
    return YES;
  } else
    return NO;
};

Strober::perform {
  Packet *p;
  state WaitSlot:
    Bus->wait (EOT, WaitLoop);
  state WaitLoop:
    p = ThePacket;
    if (p->TP == SLOT) {
      if (flagSet (p->Flags, RQST))
        (*OtherRQ)++;
      else if (OtherXmitter->erase ())
        setFlag (p->Flags, RQST);
      if (flagCleared (p->Flags, FULL)) {
        if (Buffer->isFull ()) {
          // The transmitter is waiting
          if (*MyCD == 0) {
            setFlag (p->Flags, FULL);
            Strobe->put ();
          } else
            (*MyCD)--;
        } else {
          if (*MyRQ > 0) (*MyRQ)--;
        }
      }
    }
    skipto WaitSlot;
};

SlotGen::perform {
  state Generate:
    Bus->transmit (SMarker, XDone);
  state XDone:
    Bus->stop ();
    Timer->wait (SegmWindow, Generate);
};
