/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

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
