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
