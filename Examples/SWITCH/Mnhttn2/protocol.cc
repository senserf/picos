#include "types.h"

// This is the Manhattan-Street protocol based on the 'mesh' configuration,
// similarly as MNA and Floodnet

Long SlotML,        // Slot marker length (in bits)
     SegmPL,        // Segment payload length
     SegmFL;        // The length of the segment header and trailer

TIME SegmWindow;    // Segment window length in ITUs
     
RATE TRate;         // Time granularity

Long NCols,         // The number of columns
     NRows;         // The number of rows

#define	FREE       0     // Status of a slot to be relayed
#define OWN        1
#define INCOMING   2

void assignPortRanks () {
  short **IM,   // Mesh graph incidence matrix
        t1, t2, t;
  Long i, j, k;
  MStation *S;
  IM = new short* [NStations];
  for (i = 0; i < NStations; i++) IM [i] = new short [NStations];
  // Initialize the incidence matrix
  for (i = 0; i < NStations; i++) {
    for (j = 0; j < NStations; j++)
      IM [i][j] = (i == j) ? 0 : MAX_short;
    S = (MStation*) idToStation (i);
    for (j = 0; j < 2; j++)
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
    S = (MStation*) idToStation (i);
    S->Pref = new unsigned char [NStations];
    for (j = 0; j < NStations; j++)
      if ((t1 = IM [S->Neighbors [0]][j]) < (t2 = IM [S->Neighbors [1]][j]))
        S->Pref [j] = 2;
      else if (t1 > t2)
        S->Pref [j] = 1;
      else
        S->Pref [j] = 0;
  }
  // Done, now deallocate the distance matrix which is no longer needed
  for (i = 0; i < NStations; i++)
    delete IM [i];
  delete IM;
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

void Router::route () {
  int i, prf, d0, d1;
  prf = 0;
  for (i = 0; i < 2; i++) {
    prf <<= 2;
    if (OS [i] != FREE) prf |= S->Pref [OP [i] -> Receiver];
  }
/*@@@
  { int q, z, d;
    q = prf;
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
    case  0:  // 0 --> ANY    1 --> ANY
    case  3:  // 0 --> ANY    1 --> BOTH
    case  5:  // 0 --> 1      1 --> 1
    case 10:  // 0 --> 0      1 --> 0
    case 12:  // 0 --> BOTH   1 --> ANY
    case 15:  // 0 --> BOTH   1 --> BOTH
              OPorts [i = flip ()] = S->OPorts [0];
              OPorts [1 - i]       = S->OPorts [1];
              break;
    case  1:  // 0 --> ANY    1 --> 1
    case  8:  // 0 --> 0      1 --> ANY
    case  9:  // 0 --> 0      1 --> 1
    case 11:  // 0 --> 0      1 --> BOTH
    case 13:  // 0 --> BOTH   1 --> 1
              OPorts [0] = S->OPorts [0];
              OPorts [1] = S->OPorts [1];
              break;
    case  2:  // 0 --> ANY    1 --> 0
    case  4:  // 0 --> 1      1 --> ANY
    case  6:  // 0 --> 1      1 --> 0
    case  7:  // 0 --> 1      1 --> BOTH
    case 14:  // 0 --> BOTH   1 --> 0
              OPorts [0] = S->OPorts [1];
              OPorts [1] = S->OPorts [0];
              break;
    default:
              excptn ("Illegal preference indicator");
  }
}
