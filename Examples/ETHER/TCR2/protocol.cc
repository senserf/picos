identify TCR2;

#include "types.h"

TIME SlotLength,      // The length of a virtual slot (2L + epsilon)
     GuardDelay,      // To compensate for non-zero clock tolerance
     TPSpace,         // Precomputed TIME versions of PSpace and JamL
     TJamL;

RATE TRate;           // Time granularity (read from data file)

#include "cxmitter.cc"

Boolean Transmitter::loser () {
  return (S->getId () >> Ply) & 01;
};

void Transmitter::onCollision () {
  if (!TournamentInProgress) {
    Ply = DelayCount = 0;
    DeferCount = 1;
    TournamentInProgress = YES;
  }
  if (Competing) {
    if (DelayCount == 0) {
      if (loser ()) DelayCount = 1;
      Ply++;
    } else
      DelayCount++;
  }
  DeferCount++;
};

void Transmitter::onEndSlot () {
  if (--DeferCount == 0) 
    TournamentInProgress = NO;
  else if (Competing)
    DelayCount--;
};

Boolean Transmitter::participating () {
  return Competing && DelayCount == 0;
};
