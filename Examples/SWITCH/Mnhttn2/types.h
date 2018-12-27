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

#include "mesh.h"
#include "utraffin.h"

// This is the type definition file for the Manhattan-Street protocol. It is
// based on the 'mesh' configuration, similarly as Floodnet and MNA.

// Note: the declarations below are borrowed from Fasnet
extern Long SlotML,     // Slot marker length (in bits)
            SegmPL,     // Segment payload length
            SegmFL;     // The length of the segment header and trailer
            
extern TIME SegmWindow; // Segment window length in ITUs

#define SLOT  NONE      // The type of the packet representing slot markers
#define FULL  PF_usr0   // The full/empty status of the slot
// End of the part borrowed from Fasnet

extern Long NCols,      // The number of columns
            NRows;      // The number of rows

#define col(n)  ((n) % NCols)   // Macros to convert Id to column/row
#define row(n)  ((n) / NCols)
#define odd(n)  ((n) & 1)       // Macros to tell whether a number is odd/event
#define evn(n)  (!odd (n))

mailbox DBuffer (Packet*);
            
extern RATE TRate;         // Time granularity: ITUs/bit

void assignPortRanks ();
            
station MStation : MeshNode, ClientInterface {
  DBuffer *DB [2];                  // Compensation buffers
  Packet SMarker;                   // Slot marker
  unsigned char *Pref;              // Port preference table
  void setup () {
    int i;
    MeshNode::configure (2);
    ClientInterface::configure (2);
    for (i = 0; i < 2; i++) DB [i] = create DBuffer (MAX_Long);
    SMarker.fill (NONE, NONE, SlotML);
  };
};

process Input (MStation) {
  DBuffer *DB;
  Port *IPort;
  void setup (int d) {
    DB = S->DB [d];
    IPort = S->IPorts [d];
  };
  states {WaitSlot, NewSlot, Receive, RDone};
  perform;
};

process SlotGen (MStation) {
  DBuffer *DB;
  Port *IPort;
  Packet *SMarker;
  void setup (int d) {
    DB = S->DB [d];
    IPort = S->IPorts [d];
    SMarker = &(S->SMarker);
  };
  states {GenSlot, Exit};
  perform;
};

process Router (MStation) {
  DBuffer **DB;
  Port *OPorts [2];
  Packet **Buffer, *SMarker, *OP [2];
  int OS [2];
  TIME RTime;
  void route ();
  void setup () {
    DB = S->DB;
    Buffer = S->Buffer;
    SMarker = &(S->SMarker);
  };
  states {Wait2, SDone, PDone};
  perform;
};
