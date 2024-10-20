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

#ifndef __cobserver_c__
#define __cobserver_c__

// This is the code of an observer verifying that no packet collides
// more than a specified number of times. This observer can be used
// to validate the collision protocols that enforce a limit on the
// maximum number of collisions per packet, e.g., TCR, DP, and VT.

#include "cobsrvr.h"

void CObserver::setup (int max) {
  CCount = new int [NStations];
  for (int i = 0; i < NStations; i++) CCount [i] = 0;
  MaxCollisions = max;
};

CObserver::perform {
  state Monitoring:
    inspect (ANY, Transmitter, XDone, EndTransfer);
    inspect (ANY, Transmitter, EndJam, Collision);
  state EndTransfer:
    CCount [TheStation->getId ()] = 0;
    proceed Monitoring;
  state Collision:
    if (++(CCount [TheStation->getId ()]) > MaxCollisions)
      excptn ("Too many collisions");
    proceed Monitoring;
};

#endif
