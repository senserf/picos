identify CSMA/CD-DP;

#include "types.h"

TIME SlotLength,      // The length of a virtual slot (2L + epsilon)
     GuardDelay,      // To compensate for non-zero clock tolerance
     TPSpace,         // Precomputed TIME versions of PSpace and JamL
     TJamL;

RATE TRate;           // Time granularity (read from data file)

#include "cxmitter.cc"

void Transmitter::onCollision () {
  assert (!TournamentInProgress, "Collision in controlled mode");
  TournamentInProgress = YES;
  DelayCount = Priority;
  DeferCount = NStations;
};

void Transmitter::onEOT () {
  if (--Priority < 0) Priority = NStations - 1;
  DelayCount = Priority;
  DeferCount = NStations;
};

void Transmitter::onEndSlot () {
  if (--DeferCount == 0) 
    TournamentInProgress = NO;
  else
    DelayCount--;
};

Boolean Transmitter::participating () {
  return DelayCount == 0 && S->ready (MinPL, MaxPL, FrameL);
};
