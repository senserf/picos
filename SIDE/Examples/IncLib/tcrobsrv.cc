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

#ifndef __tcrobserver_c__
#define __tcrobserver_c__

// This is the code of the 'tournament observer' for TCR (included by 'root.c'
// in TCR1 and TCR2).

#include "tcrobsrv.h"

void TCRObserver::setup () {
  int i, j, ml;
  // Calculate the maximum number of levels
  for (ml = 1, i = 1; i < NStations; i += i, ml++);
  Tree = new Boolean [ml];
  Players = new Boolean* [ml];
  for (i = 0; i < ml; i++) {
    Players [i] = new Boolean [NStations];
    for (j = 0; j < NStations; j++)
      Players [i][j] = NO;
  }
  CurrentLevel = 0;
  TournamentInProgress = NO;
};

void TCRObserver::newPlayer () {
  Players [CurrentLevel][TheStation->getId ()] = YES;
};

void TCRObserver::removePlayer () {
  Long lv, sid;
  sid = TheStation->getId ();
  for (lv = CurrentLevel; lv >= 0; lv--)
    if (Players [lv][sid])
      Players [lv][sid] = NO;
    else
      excptn ("Unknown player leaves the tournament");
};

void TCRObserver::validateTransmissionRights () {
  Long lv, sid;
  if (TournamentInProgress) {
    sid = TheStation->getId ();
    for (lv = 0; lv < CurrentLevel; lv++)
      if (((sid >> lv) & 01) != Tree [lv]) 
        excptn ("Illegal transmission");
  }
};

void TCRObserver::descend () {
  Tree [CurrentLevel++] = Left;
  TournamentInProgress = YES;
};

void TCRObserver::advance () {
  if (Tree [CurrentLevel - 1] == Left)
    Tree [CurrentLevel - 1] = Right;
  else {
    CurrentLevel--;
    for (int sid = 0; sid < NStations; sid++)
      if (Players [CurrentLevel][sid])
        excptn ("Some players have not transmitted");
    if (CurrentLevel)
      advance ();
    else
      TournamentInProgress = NO;
  }
};
 
TCRObserver::perform  {
  state SlotStarted:
    inspect (ANY, Transmitter, Transmit, CleanBOT);
    if (TournamentInProgress)
      timeout (SlotLength, EmptySlot);
  state EmptySlot:
    advance ();
    proceed SlotStarted;
  state CleanBOT:
    validateTransmissionRights ();
    newPlayer ();
    inspect (ANY, Transmitter, Transmit, CleanBOT);
    inspect (ANY, Transmitter, XDone, Success);
    inspect (ANY, Transmitter, SenseCollision, StartCollision);
  state Success:
    removePlayer ();
    if (TournamentInProgress)
      advance ();
    proceed SlotStarted;
  state StartCollision:
    CollisionCleared = Time + TJamL + SlotLength;
    proceed ClearCollision;
  state ClearCollision:
    timeout (CollisionCleared - Time, Descend);
    inspect (ANY, Transmitter, Transmit, CollisionBOT);
  state CollisionBOT:
    validateTransmissionRights ();
    newPlayer ();
    proceed ClearCollision;
  state Descend:
    descend ();
    proceed SlotStarted;
};

#endif
