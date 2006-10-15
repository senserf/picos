#include "types.h"

identify "Pretzel";

Long SlotML,     // Slot marker length (in bits)
     SegmPL,     // Segment payload length
     SegmFL;     // The length of the segment header and trailer

TIME SegmWindow; // Segment window length in ITUs
int  THT,        // Token holding time per station
     TZLength;   // Token zone length where transmissions are illegal

Relay::perform {
  SMarker *SMark;
  state WaitSlot:
    Erasing = Transmitting = NO;
    IRing->wait (BOT, NewSlot);
  state NewSlot:
    assert (ThePacket->TP == SLOT, "Slot marker expected");
    SMark = (SMarker*) ThePacket;
    // Update token holding time or delay since last token departure 
    ++TCount;
    if (MyPurge->nonempty () && MyPurge->first () == SMark->Number) {
      Erasing = YES;
      MyPurge->get ();
    }
    if (HoldingToken) {
      if (TCount == THT) {
        // The token should be passed in this slot
        clearFlag (SMark->Flags, TOKEN);
        *IAmYellow = YES;
        *YouAreYellow = NO;
        TCount = 0;  // To count delay since token departure
        HoldingToken = NO;
      } else
        Erasing = YES;
    } else if (flagCleared (SMark->Flags, TOKEN)) {
      // Receive the token
      setFlag (SMark->Flags, TOKEN);
      HoldingToken = YES;
      Erasing = YES;
      TCount = 0;
    }
    if (Erasing)
      clearFlag (SMark->Flags, FULL);
    if (flagCleared (SMark->Flags, FULL) &&
     (!(*IAmYellow) || TCount >= TZLength) &&
      S->ready (SegmPL, SegmPL, SegmFL)) {
      // Copy the packet to private buffer (so that both transmitters
      // can operate in parallel)
      *LBuffer = *SBuffer;
      clearFlag (SBuffer->Flags, PF_full);
      setFlag (SMark->Flags, FULL);
      Transmitting = YES;
      YourPurge->put (SMark->Number);
    };
    // Relay the slot marker
    ORing->startTransfer (SMark);
    IRing->wait (EOT, MDone);
  state MDone:
    ORing->stop ();
    if (Transmitting)
      ORing->transmit (LBuffer, XDone);
    else if (Erasing)
      skipto WaitSlot;
    else if (IRing->events (BOT)) {
      ORing->startTransfer (ThePacket);
      skipto WaitEOT;
    } else
      proceed WaitSlot;
  state WaitEOT:
    IRing->wait (EOT, RDone);
  state RDone:
    ORing->stop ();
    if (ThePacket->isMy ()) Client->receive (ThePacket, IRing);
    proceed WaitSlot;
  state XDone:
    ORing->stop ();
    LBuffer->release ();
    proceed WaitSlot;
};

SlotGen::perform {
  state GenSlot:
    if (DB->nonempty ())
      terminate;
    else {
      SMark->Number = SCount;
      ORing->transmit (SMark, XDone);
    }
  state XDone:
    ORing->stop ();
    ++SCount;
    Timer->wait (SegmWindow, GenSlot);
};

IConnector::perform {
  SMarker *sm;
  Packet *pk;
  state WaitBOT:
    IRing->wait (BOT, Activity);
  state Activity:
    if (ThePacket->TP == SLOT) {
      sm = create SMarker;
      *sm = *((SMarker*)ThePacket);
      DB->put (sm);
    } else {
      pk = create Packet;
      *pk = *ThePacket;
      DB->put (pk);
    }
    skipto WaitBOT;
};

OConnector::perform {
  TIME d;
  state NextPacket:
    if (DB->nonempty ()) {
      if ((DB->first ())->TP == SLOT && (d = Time - LastSlot) < SegmWindow)
        Timer->wait (SegmWindow - d, NextPacket);
      else
        ORing->transmit (DB->first (), XDone);
    } else
      DB->wait (NONEMPTY, NextPacket);
  state XDone:
    ORing->stop ();
    if ((DB->first ())->TP == SLOT) LastSlot = Time;
    DB->get ();
    proceed NextPacket;
};
