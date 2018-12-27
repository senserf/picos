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

#ifndef __cobserver_h__
#define __cobserver_h__

// This file contains the type declaration of an observer whose code
// is provided in 'cobsrvr.cc' This observer verifies that no packet
// collides more than a specified number of times. It can be used to
// validate the collision protocols that enforce a limit on the
// maximum number of collisions per packet, e.g., TCR, DP, and VT.

observer CObserver {
  int *CCount,              // Collision counters per stations
      MaxCollisions;        // Maximum number of collisions per packet
  void setup (int);
  states {Monitoring, EndTransfer, Collision};
  perform;
};

#endif
