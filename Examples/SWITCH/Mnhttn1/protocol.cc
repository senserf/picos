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

// This is the Manhattan-Street protocol based on the 'mesh' configuration,
// similarly as MNA and Floodnet

Long SlotML,        // Slot marker length (in bits)
     SegmPL,        // Segment payload length
     SegmFL;        // The length of the segment header and trailer

TIME SegmWindow;    // Segment window length in ITUs
     
RATE TRate;         // Time granularity

Long NCols,         // The number of columns
     NRows,         // The number of rows
     NCols05,       // NCols/2
     NRows05,       // NRows/2
     NCols15,       // NCols * 1.5
     NRows15;       // NRows * 1.5

#define	FREE       0     // Status of a slot to be relayed
#define OWN        1
#define INCOMING   2

#define geq(a,b) ((a) != (b) - 1 && (a) <= (b) + 1)
#define leq(a,b) ((a) != (b) + 1 && (a) >= (b) - 1)

#define setpref(l,r,u,d) \
  for (i = 0; i < 2; i++) \
    prf = (prf << 1) | ((l || geq (nc [i], sc)) && (r || leq (nc [i], sc)) && \
              (u || geq (nr [i], sr)) && (d || leq (nr [i], sr)))
			  
// The following is for the MAC MPW compiler which doesn't like
// "too complicated" expressions

/*
#define setpref(l,r,u,d) \
  do { \
    for (i = 0; i < 2; i++) { \
      prf <<= 1; \
      if (l || geq (nc [i], sc)) \
        if (r || leq (nc [i], sc)) \
          if (u || geq (nr [i], sr)) \
            if (d || leq (nr [i], sr)) \
              prf |= 1; \
    } \
  } while (0)
*/

void transform (Long &c, Long &r, Long cd, Long rd) {
  c = NCols05 - (NCols15 + (odd (rd) ? cd - c : c - cd)) % NCols;
  r = NRows05 - (NRows15 + (odd (cd) ? rd - r : r - rd)) % NRows;
};

void MStation::setup () {
    int i;
    MeshNode::configure (2);
    ClientInterface::configure (2);
    for (i = 0; i < 2; i++) DB [i] = create DBuffer (MAX_Long);
    SMarker.fill (NONE, NONE, SlotML);
};

Input::perform {
  Packet *pk;
  state WaitSlot:
    IPort->wait (EOT, NewSlot);
  state NewSlot:
    if (ThePacket->TP == SLOT) {
      pk = create Packet;
      *pk = *ThePacket;
      DB->put (pk);
      if (IPort->events (BOT)) {
        assert (flagSet (pk->Flags, FULL) && ThePacket->TP != SLOT,
          "Slot marker followed by garbage");
        if (ThePacket->isMy ()) {
          clearFlag (pk->Flags, FULL);
          skipto Receive;
        } else {
          pk = create Packet;
          *pk = *ThePacket;
          DB->put (pk);
        }
      }
    }
    skipto WaitSlot;
  state Receive:
    IPort->wait (EOT, RDone);
  state RDone:
    assert (ThePacket->isMy (), "My packet expected");
    Client->receive (ThePacket, IPort);
    skipto WaitSlot;
};

SlotGen::perform {
  Packet *pk;
  state GenSlot:
    clearFlag (SMarker -> Flags, FULL);
    pk = create Packet;
    *pk = *SMarker;
    DB->put (pk);
    Timer->wait (SegmWindow + (SMarker->TLength) * TRate , GenSlot);
    IPort->wait (BOT, Exit);
  state Exit:
    terminate;
};

Router::perform {
  int i;
  Packet *sm, *pk;
  state Wait2:
    for (i = 0; i < 2; i++)
      if (DB [i] -> empty ()) {
        DB [i] -> wait (NONEMPTY, Wait2);
        sleep;
      }
    for (i = 0; i < 2; i++) {
      sm = DB [i] -> get ();
      assert (sm -> TP == SLOT, "Slot marker expected");
      if (flagSet (sm -> Flags, FULL)) {
        assert (DB [i] -> nonempty (), "Missing payload");
        pk = DB [i] -> get ();
        assert (pk -> TP != SLOT, "Payload expected");
        OP [i] = pk;
        OS [i] = INCOMING;
      } else {
        if (S->ready (i, SegmPL, SegmPL, SegmFL)) {
          OP [i] = Buffer [i];
          OS [i] = OWN;
        } else
          OS [i] = FREE;
      }
      delete sm;
    }
    route ();
    for (i = 0; i < 2; i++) {
      if (OS [i] == FREE)
        clearFlag (SMarker -> Flags, FULL);
      else
        setFlag (SMarker -> Flags, FULL);
      OPorts [i] -> transmit (SMarker, SDone);
    };
  state SDone:
    RTime = Time;
    for (i = 0; i < 2; i++) {
      OPorts [i] -> stop ();
      if (OS [i] == FREE)
        Timer->wait (SegmWindow, Wait2);
      else
        OPorts [i] -> transmit (OP [i], PDone);
    }
  state PDone:
    for (i = 0; i < 2; i++) {
      if (OS [i] != FREE) {
        OPorts [i] -> stop ();
        if (OS [i] == OWN)
          OP [i] -> release ();
        else
          delete OP [i];
      }
    }
    Timer->wait (SegmWindow - (Time - RTime), Wait2);
}

static void findPreferred (Long snd, Long dst, Long ngh [2], int &prf) {
  Long i, sc, sr, dc, dr, nc [2], nr [2];
  // Transform coordinates
  sc = col (snd);
  sr = row (snd);
  dc = col (dst);
  dr = row (dst);
/*@@@trace ("Src: %1d %1d, Dst: %1d %1d", sc, sr, dc, dr);*/
  transform (sc, sr, dc, dr);
/*@@@trace ("Transf src: %1d %1d", sc, sr);*/
  for (i = 0; i < 2; i++) {
    nc [i] = col (ngh [i]);
    nr [i] = row (ngh [i]);
    transform (nc [i], nr [i], dc, dr);
/*@@@trace ("Transf nei: %1d %1d", nc [i], nr [i]);*/
  }
  // Examine cases
  if (sc <= 0) {
    // Left half including the destination column
    if (sr <= 0) {
      // Left upper quadrant
      if (sc > -NCols05+1) {
        // Not the leftmost column
        if (sr > -NRows05+1)
          // Not the upmost row
          setpref (NO, YES, NO, YES);
        else
          // The upmost row
          setpref (NO, YES, YES, YES);
      } else if (sr > -NRows05+1)
        // Not the left upper corner
        setpref (YES, YES, NO, YES);
      else
        // The left upper corner
        setpref (YES, YES, YES, YES);
    } else {
      // Left bottom quadrant
      if (sc < 0) {
        // Not the destination column
        if (sr > 1)
          // Not the first row
          setpref (NO, YES, YES, NO);
        else
          // The first row
          setpref (NO, YES, NO, NO);
      } else
        // The destination column
        setpref (NO, NO, YES, NO);
    }
  } else {
    // Right half excluding the destination column
    if (sr <= 0) {
      // Right upper quadrant
      if (sr < 0) {
        // Not the first row
        if (sc > 1)
          // Not the first column
          setpref (YES, NO, NO, YES);
        else
          // The first column
          setpref (NO, NO, NO, YES);
      } else
        // The first row
        setpref (YES, NO, NO, NO);
    } else {
      // Right bottom quadrant
      if (sc < NCols05) {
        // Not the last column
        if (sr < NRows05)
          // Not the last row
          setpref (YES, NO, YES, NO);
        else
          // The last row, but not the last column
          setpref (YES, NO, YES, YES);
      } else {
        // The last column
        if (sr < NRows05)
          // Not the right bottom corner
          setpref (YES, YES, YES, NO);
        else
          // The corner
          setpref (YES, YES, YES, YES);
      }
    }
  }
}

void Router::route () {
  int i, prf;
  prf = 0;
  for (i = 0; i < 2; i++) {
    if (OS [i] != FREE)
      findPreferred (S -> getId (), OP [i] -> Receiver, S -> Neighbors, prf);
    else
      prf <<= 2;
  }
/*@@@
  { int q, z, d;
    q = prf >> 1;
    for (i = 0; i < 2; i++) {
      z = q >> ((1 - i) * 2);
      z &= 3;
      if (z == 1) d = S->Neighbors [1]; else if (z == 2) d = S->Neighbors [0];
        else d = NONE;
      if (OS [i] != FREE)
      trace ("Loc: %1d, dest: %1d, pref: %1d, %1d", S->getId(),
        OP [i]->Receiver, z, d);
    }
  }
@@@*/
  switch (prf) {
    case  0:  // 00 00   0 --> ANY    1 --> ANY
    case  3:  // 00 11   0 --> ANY    1 --> BOTH
    case  5:  // 01 01   0 --> 1      1 --> 1
    case 10:  // 10 10   0 --> 0      1 --> 0
    case 12:  // 11 00   0 --> BOTH   1 --> ANY
    case 15:  // 11 11   0 --> BOTH   1 --> BOTH
              OPorts [i = flip ()] = S->OPorts [0];
              OPorts [1 - i]       = S->OPorts [1];
              break;
    case  1:  // 00 01   0 --> ANY    1 --> 1
    case  8:  // 10 00   0 --> 0      1 --> ANY
    case  9:  // 10 01   0 --> 0      1 --> 1
    case 11:  // 10 11   0 --> 0      1 --> BOTH
    case 13:  // 11 01   0 --> BOTH   1 --> 1
              OPorts [0] = S->OPorts [0];
              OPorts [1] = S->OPorts [1];
              break;
    case  2:  // 00 10   0 --> ANY    1 --> 0
    case  4:  // 01 00   0 --> 1      1 --> ANY
    case  6:  // 01 10   0 --> 1      1 --> 0
    case  7:  // 01 11   0 --> 1      1 --> BOTH
    case 14:  // 11 10   0 --> BOTH   1 --> 0
              OPorts [0] = S->OPorts [1];
              OPorts [1] = S->OPorts [0];
              break;
    default:
              excptn ("Illegal preference pattern");
  }
}
