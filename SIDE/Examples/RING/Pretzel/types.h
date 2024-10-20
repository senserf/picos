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

#include "pring.h"
#include "utraffic.h"

// Note: the declarations below are borrowed from Fasnet
extern Long SlotML,     // Slot marker length (in bits)
            SegmPL,     // Segment payload length
            SegmFL;     // The length of the segment header and trailer
            
extern TIME SegmWindow; // Segment window length in ITUs

#define SLOT  NONE      // The type of the packet representing slot markers
#define FULL  PF_usr0   // The full/empty status of the slot
// End of the part borrowed from Fasnet

#define TOKEN PF_usr1   // The token flag in the slot marker

packet SMarker {
  int Number;
};

extern int THT,         // Token holding time per station
           TZLength;    // Token zone length (where transmissions are illegal)

mailbox SList (int);    // To store slot numbers

station PStation : PRingInterface, ClientInterface {
  SList *Purge [2];     // List of slots to be erased
  Boolean Yellow [2];
  void setup () {
    int i;
    PRingInterface::configure ();
    ClientInterface::configure ();
    for (i = 0; i < 2; i++)
      Purge [i] = create SList (MAX_Long);
    Yellow [0] = NO;
    Yellow [1] = YES;
  };
};

process Relay (PStation) {
  Port *IRing, *ORing;
  SList *MyPurge, *YourPurge;
  Packet *SBuffer,      // Station buffer
         *LBuffer;      // Local buffer for transmitted segment
  Long TCount;          // Slot counter to measure token intervals 
  Boolean HoldingToken, Erasing, Transmitting, *IAmYellow, *YouAreYellow;
  void setup (int segment) {
    IRing = S->IRing [segment];
    ORing = S->ORing [segment];
    MyPurge = S->Purge [segment];
    YourPurge = S->Purge [1-segment];
    SBuffer = &(S->Buffer);
    LBuffer = create Packet;
    TCount = 0;
    HoldingToken = Erasing = Transmitting = NO;
    IAmYellow = &(S->Yellow [segment]);
    YouAreYellow = &(S->Yellow [1-segment]);
  };
  states {WaitSlot, NewSlot, MDone, WaitEOT, RDone, XDone};
  perform;
};

mailbox DBuffer (Packet*) {
  void outItem (Packet *p) { delete p; };
  void setup () { setLimit (MAX_Long); };
};

station PMonitor : PMonitorInterface {
  DBuffer *DB;
  void setup () {
    PMonitorInterface::configure ();
    DB = create DBuffer;
  };
};

process IConnector (PMonitor) {
  Port *IRing;
  DBuffer *DB;
  void setup () {
    IRing = S->IRing;
    DB = S->DB;
  };
  states {WaitBOT, Activity};
  perform;
};

process OConnector (PMonitor) {
  Port *ORing;
  DBuffer *DB;
  TIME LastSlot;
  void setup () {
    ORing = S->ORing;
    DB = S->DB;
    LastSlot = TIME_0;
    assert (DB->nonempty (), "OConnector -- ring not filled");
    clearFlag ((DB->first ())->Flags, TOKEN);
  };
  states {NextPacket, XDone};
  perform;
};

process SlotGen (PMonitor) {
  Port *ORing;
  DBuffer *DB;
  SMarker *SMark;
  int SCount;
  void setup () {
    ORing = S->ORing;
    DB = S->DB;
    SMark = create SMarker;
    SMark->fill (NONE, NONE, SlotML);
    setFlag (SMark->Flags, TOKEN);
    // Note: this actually means "no token"
    SCount = 0;
  };
  states {GenSlot, XDone};
  perform;
};
