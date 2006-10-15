#include "types.h"

identify "Insertion Ring";

Long MinPL,             // Minimum payload length
     MaxPL,             // Maximum payload length
     FrameL,            // Header + trailer
     HdrL;              // Header

Input::perform {
  state WaitBOT:
    IRing->wait (BOT, NewPacket);
  state NewPacket:
    Pkt = ThePacket;
    Timer->wait (HdrL, CheckRcv);
  state CheckRcv:
    if (Pkt->isMy ()) {
      IRing->wait (EOT, Receive);
    } else {
      Packet *p;
      p = create Packet;
      *p = *Pkt;
      S->Blocked = YES;
      IBuffer->put (p);
      IRing->wait (EOT, Drop);
    }
  state Receive:
    Client->receive (Pkt, IRing);
    proceed WaitBOT;
  state Drop:
    S->Blocked = NO;
    Xmitter->signal ();
    proceed WaitBOT;
};

Transmitter::perform {
  Long f;
  state Acquire:
    if (S->Blocked)
      wait (SIGNAL, Acquire);
    else if (S->ready (MinPL, MaxPL, FrameL)) {
      // Check if enough room in the buffer
      if ((f = IBuffer->free ()) >= Buffer->TLength) {
        Packet *p;
        p = create Packet;
        *p = *Buffer;
        IBuffer->put (p);
        Buffer->release ();
        proceed Acquire;
      } else
        Timer->wait (Buffer->TLength - f, Acquire);
    } else
      Client->wait (ARRIVAL, Acquire);
};

Relay::perform {
  state WaitPacket:
    if (IBuffer->first () == NULL)
      IBuffer->wait (NONEMPTY, WaitPacket);
    else
      ORing->transmit (IBuffer->first (), XDone);
  state XDone:
    ORing->stop ();
    IBuffer->get ();
    proceed WaitPacket;
};
