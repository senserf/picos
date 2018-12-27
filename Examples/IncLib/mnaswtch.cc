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

#ifndef __mnaswitch_c__
#define __mnaswitch_c__

#include "mnaswtch.h"
#include "mesh.cc"

// This is the MNA deflection protocol. It is used in a number of MNA
// instances (see directories MNA? in MISC).

Long MinPL,         // Minimum packet length
     MaxPL,         // Maximum packet length
     FrameL;        // Header and trailer
     
RATE TRate;         // Transmission rate

void assignPortRanks () {
  short **IM,   // Mesh graph incidence matrix
        t1, t2, t;
  Long i, j, k;
  Switch *S;
  IM = new short* [NStations];
  for (i = 0; i < NStations; i++) IM [i] = new short [NStations];
  // Initialize the incidence matrix
  for (i = 0; i < NStations; i++) {
    for (j = 0; j < NStations; j++)
      IM [i][j] = (i == j) ? 0 : MAX_short;
    S = (Switch*) idToStation (i);
    for (j = 0; j < S->Order; j++)
      IM [i][S->Neighbors [j]] = 1;
  }
  // The following is the well known all shortest paths algorithm
  for (k = 0; k < NStations; k++)
    for (i = 0; i < NStations; i++) {
      if ((t1 = IM [i][k]) < MAX_short)
        for (j = 0; j < NStations; j++) 
          if ((t2 = IM [k][j]) < MAX_short && (t = t1 + t2) < IM [i][j])
            IM [i][j] = t;
    }
  // End of the shortest paths algorithm
  for (i = 0; i < NStations; i++) {
    S = (Switch*) idToStation (i);
    // Assign ranks to the ports of station i
    S->PRanks = new unsigned char* [S->Order];
    for (j = 0; j < S->Order; j++) {
      S->PRanks [j] = new unsigned char [NStations];
      for (k = 0; k < NStations; k++)
        S->PRanks [j][k] = IM [S->Neighbors [j]][k];
    }
  }
  // Done, now deallocate the distance matrix which is no longer needed
  for (i = 0; i < NStations; i++)
    delete IM [i];
  delete IM;
};

// The protocol

#define MAXORDER 256   // Seems to be a safe upper limit

int Switch::route (Packet *p) {
  int P [MAXORDER], R [MAXORDER], NP, r, min, i, j;
  Long rcv;
  rcv = p -> Receiver;
  for (min = MAX_int, i = NP = 0; i < Order; i++) {
    // Collect together all idle output ports with ranks not higher
    // than the current minimum
    if (Idle [i]) {
      r = PRanks [i][rcv];
      if (r <= min) {
        P [NP] = i;
        R [NP] = r;
        min = r;
        NP++;
      }
    }
  }
  assert (NP, "route: can't relay packet");
  // Now leave only the ports with minimum ranks
  for (i = 0, j = 0; i < NP; i++)
    if (R [i] == min) P [j++] = P [i];
  return (j > 1) ? P [toss (j)] : P [0];
};

Router::perform {
  state Waiting:
    // Wait for a packet on the input port
    IPort->wait (BOT, NewPacket);
  state NewPacket:
    if (ThePacket->isMy ())
      // The packet is addressed to this host
      skipto WaitRcv;
    // The packet must be relayed
    OP = S->route (ThePacket);
    OPort = S->OPorts [OP];
    S->Idle [OP] = NO;
    // Relay the packet
    OPort->startTransmit (ThePacket);
    skipto WaitEnd;
  state WaitEnd:
    IPort->wait (EOT, EndPacket);
  state EndPacket:
    OPort->stop ();
    S->Idle [OP] = YES;
    proceed Waiting;
  state WaitRcv:
    IPort->wait (EOT, Rcv);
  state Rcv:
    // Receive the packet
    Client->receive (ThePacket, IPort);
    proceed Waiting;
};

InDelay::perform {
  state Waiting:
    IPort->wait (BOT, In);
  state In:
    NPort->startTransmit (ThePacket);
    S->Used [MP] ++;
    skipto WaitEOT;
  state WaitEOT:
    IPort->wait (EOT, RDone);
  state RDone:
    NPort->stop ();
    proceed Waiting;
};

OutDelay::perform {
  state Waiting:
    XPort->wait (EOT, Out);
  state Out:
    assert (S->Used [MP], "Port should be marked as 'used'");
    if (--(S->Used [MP]) == 0) FreePort->put ();
    skipto Waiting;
};

Transmitter::perform {
  state NewMessage:
    if (!S->ready (BF, MinPL, MaxPL, FrameL)) {
      Client->wait (ARRIVAL, NewMessage);
      sleep;
    }
  transient RetryRoute:
    // Find an idle input port
    for (XP = 0; XP < Order; XP++)
      if (!S->Used [XP]) break;
    if (XP == Order) {
      FreePort->wait (NEWITEM, RetryRoute);
    } else {
      // Reserve the port
      S->Used [XP] ++;
      XPort = S->XDelay [XP];
      XPort->transmit (Buffer, EndXmit);
      // The next wait is just in case, collisions are impossible
      XPort->wait (COLLISION, Error);
    }
  state EndXmit:
    XPort->stop ();
    Buffer->release ();
    proceed NewMessage;
  state Error:
    excptn ("Transmitter: illegal collision");
};

#endif
