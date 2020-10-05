/*
	Copyright 1995-2020 Pawel Gburzynski

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

#define PrivateQualifier

#include "mring.h"
#include "utraffi2.h"

extern Long MinPL, MaxPL, FrameL, HdrL;    // Packetization parameters

// This is the same as in 'Insertion'
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

station MStation : MRingInterface, ClientInterface {
  PQueue *IBuffer [2];
  Boolean Blocked [2];     // Incoming packet, transmitter blocked
  void setup (Long bufl) {
    int i;
    MRingInterface::configure ();
    ClientInterface::configure ();
    for (i = 0; i < 2; i++) {
      IBuffer [i] = create PQueue (bufl);
      Blocked [i] = NO;
    }
  };
};

process Transmitter (MStation) {
  PQueue *IBuffer;
  Packet *Buffer;
  Boolean *Blocked;
  int Direction;
  void setup (int dir) {
    Direction = dir;
    IBuffer = S->IBuffer [dir];
    Buffer = &(S->Buffer [dir]);
    Blocked = &(S->Blocked [dir]);
  };
  states {Acquire};
  perform;
};

process Input (MStation) {
  Port *IRing;
  PQueue *IBuffer;
  Transmitter *Xmitter;  // Pointer to the transmitter process
  Packet *Pkt;           // Used to save pointer to the incoming packet
  Boolean *Blocked;
  void setup (int dir, Transmitter *pr) {
    Xmitter = pr;
    IRing = S->IRing [dir];
    IBuffer = S->IBuffer [dir];
    Blocked = &(S->Blocked [dir]);
  };
  states {WaitBOT, NewPacket, CheckRcv, Receive, Drop};
  perform;
};

process Relay (MStation) {
  Port *ORing;
  PQueue *IBuffer;
  void setup (int dir) {
    ORing = S->ORing [dir];
    IBuffer = S->IBuffer [dir];
  };
  states {WaitPacket, XDone};
  perform;
};
