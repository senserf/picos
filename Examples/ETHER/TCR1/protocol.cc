identify TCR1;

#include "types.h"

TIME SlotLength,      // The length of a virtual slot (2L + epsilon)
     GuardDelay,      // To compensate for non-zero clock tolerance
     TPSpace,         // Precomputed TIME versions of PSpace and JamL
     TJamL;

RATE TRate;           // Time granularity (read from data file)

Transmitter:: perform {
  state NPacket:
    if (S->ready (MinPL, MaxPL, FrameL))
      proceed Transmit;
    else {
      Client->wait (ARRIVAL, NPacket);
      Bus->wait (ACTIVITY, Busy);
    }
  state Transmit:
    Transmitting = YES;
    Competing = YES;
    Bus->transmit (Buffer, XDone);
    Bus->wait (COLLISION, SenseCollision);
  state Busy:
    Bus->wait (EOT, SenseEOT);
    Bus->wait (COLLISION, SenseCollision);
  state XDone:
    Bus->stop ();
    Transmitting = NO;
    Competing = NO;
    Buffer->release ();
    proceed SenseEOT;
  state SenseCollision:
    if (!TournamentInProgress) {
      Ply = DelayCount = 0;
      DeferCount = 1;
      TournamentInProgress = YES;
    }
    if (Competing) {
      if (Transmitting) {
        if (loser ()) DelayCount = 1;
        Ply++;
      } else
        DelayCount++;
    }
    DeferCount++;
    if (Transmitting) {
      Bus->abort ();
      Transmitting = NO;
      Bus->sendJam (TJamL, EndJam);
    } else
      Timer->wait (TJamL + SlotLength, NewSlot);
  state EndJam:
    Bus->stop ();
    Timer->wait (SlotLength, NewSlot);
  state NewSlot:
    if (Competing && DelayCount == 0)
      Timer->wait (GuardDelay, Transmit);
    else {
      Timer->wait (SlotLength, EndSlot);
      Bus->wait (ACTIVITY, Busy);
    }
  state EndSlot:
    if (--DeferCount == 0) {
      TournamentInProgress = NO;
      Timer->wait (GuardDelay, NPacket);
    } else {
      if (Competing) DelayCount--;
      proceed NewSlot;
    }
  state SenseEOT:
    if (TournamentInProgress)
      Timer->wait (TPSpace, EndSlot);
    else
      Timer->wait (TPSpace, NPacket);
};

Boolean Transmitter::loser () {
  return (S->getId () >> Ply) & 01;
};
