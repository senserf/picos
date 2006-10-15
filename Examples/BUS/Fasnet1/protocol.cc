#include "types.h"

identify Fasnet1;

Long SlotML,     // Slot marker length (in bits)
     SegmPL,     // Segment payload length
     SegmFL;     // The length of the segment header and trailer
            
TIME SegmWindow; // Segment window length in ITUs

Receiver::perform {
  state WPacket:
    Bus->wait (EMP, Rcvd);
  state Rcvd:
    Client->receive (ThePacket, ThePort);
    skipto (WPacket);
};

Transmitter::perform {
  state NPacket:
    if (S->ready (BusId, SegmPL, SegmPL, SegmFL)) {
      signal ();
      Strobe->wait (NEWITEM, Transmit);
    } else
      Client->wait (ARRIVAL, NPacket);
  state Transmit:
    Bus->transmit (Buffer, XDone);
    Bus->wait (COLLISION, Error);
  state XDone:
    Bus->stop ();
    Buffer->release ();
    proceed NPacket;
  state Error:
    excptn ("Slot collision");
};

Strober::perform {
  state WaitReady:
    MyXmitter->wait (SIGNAL, WaitEmpty);
  state WaitEmpty:
    Bus->wait (EOT, EmptyLoop);
  state EmptyLoop:
    if (ThePacket->TP == SLOT && flagCleared (ThePacket->Flags, FULL)) {
      setFlag (ThePacket->Flags, FULL);
      Strobe->put ();
      skipto WaitBOC;
    } else
      skipto WaitEmpty;
  state WaitBOC:
    Bus->wait (EOT, BOCLoop);
  state BOCLoop:
    if (ThePacket->TP == SLOT && flagSet (ThePacket->Flags, BOC))
      proceed WaitReady;
    else
      skipto WaitBOC;
};

SlotGen::perform {
  state Generate:
    if (SendBOC->get ())
      setFlag (SMarker->Flags, BOC);
    else
      clearFlag (SMarker->Flags, BOC);
    if (SendSNC->get ())
      setFlag (SMarker->Flags, SNC);
    else
      clearFlag (SMarker->Flags, SNC);
    Bus->transmit (SMarker, XDone);
  state XDone:
    Bus->stop ();
    Timer->wait (SegmWindow, Generate);
};

Absorber::perform {
  state WaitSlot:
    Bus->wait (EOT, SlotLoop);
  state SlotLoop:
    if (ThePacket->TP == SLOT) {
      if (flagSet (ThePacket->Flags, BOC)) WithinCycle = YES;
      if (WithinCycle && flagCleared (ThePacket->Flags, FULL)) {
        WithinCycle = NO;
        SendSNC->put ();
      }
      if (flagSet (ThePacket->Flags, SNC)) SendBOC->put ();
    }
    skipto WaitSlot;
};
