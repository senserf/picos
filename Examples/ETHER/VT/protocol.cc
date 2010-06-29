identify "Virtual Token";

#include "types.h"

TIME SlotLength,      // The length of a virtual slot (2L + epsilon)
     GuardDelay,      // To compensate for non-zero clock tolerance
     TPSpace,         // Precomputed TIME versions of PSpace and JamL
     TJamL;

RATE TRate;           // Time granularity (read from data file)

#include "cxmitter.cc"

void Transmitter::onEndSlot () {
  if (--Priority < 0) Priority = NStations - 1;
  TournamentInProgress = NO;
};

void Transmitter::onCollision () {
  TournamentInProgress = YES;
};

Boolean Transmitter::participating () {
  return Priority == 0 && S->ready (MinPL, MaxPL, FrameL);
};
