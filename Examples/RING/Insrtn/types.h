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

#include "sring.h"
#include "utraffic.h"

extern Long MinPL, MaxPL, FrameL, HdrL;    // Read from the input file

mailbox PQueue (Packet*) {
  Long MaxLength,    // Maximum length of the buffer in bits
       CurLength;    // Combined length of all packets in the buffer
  TIME TSTime;       // Time when the first packet became ready
  Long free () {     // Returns the length of the used portion
    return CurLength == 0 ? MaxLength : 
      MaxLength - CurLength + (Long) (Time - TSTime);
  };
  void inItem (Packet *p) {
    if (CurLength == 0) TSTime = Time;
    CurLength += p->TLength;
  };
  void outItem (Packet *p) {
    CurLength -= p->TLength;
    TSTime = Time;
    delete p;
  };
  void setup (Long l) {
    MaxLength = l;
    CurLength = 0;
    setLimit ((MaxLength + MinPL + FrameL - 1) / (MinPL + FrameL));
  };
};

station IStation : SRingInterface, ClientInterface {
  PQueue *IBuffer;
  Boolean Blocked;       // Incoming packet, transmitter blocked
  void setup (Long bufl) {
    SRingInterface::configure ();
    ClientInterface::configure ();
    IBuffer = create PQueue (bufl);
    Blocked = NO;
  };
};

process Transmitter (IStation) {
  PQueue *IBuffer;
  Packet *Buffer;
  void setup () {
    IBuffer = S->IBuffer;
    Buffer = &(S->Buffer);
  };
  states {Acquire};
  perform;
};

process Input (IStation) {
  Port *IRing;
  PQueue *IBuffer;
  Transmitter *Xmitter;  // Pointer to the transmitter process
  Packet *Pkt;           // Used to save pointer to the incoming packet
  void setup (Transmitter *pr) {
    Xmitter = pr;
    IRing = S->IRing;
    IBuffer = S->IBuffer;
  };
  states {WaitBOT, NewPacket, CheckRcv, Receive, Drop};
  perform;
};

process Relay (IStation) {
  Port *ORing;
  PQueue *IBuffer;
  void setup () {
    ORing = S->ORing;
    IBuffer = S->IBuffer;
  };
  states {WaitPacket, XDone};
  perform;
};
