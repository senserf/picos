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

#ifndef __mring_h__
#define __mring_h__

// Type announcements for 'mring.cc': Metaring configuration consisting of four
// independent rings. Two of those rings represent virtual channels used for
// SAT communicates.

#define CWRing 0   // Clockwise ring
#define CCRing 1   // Counter-clockwise ring

station MRingInterface virtual {
  Port *IRing [2], *ORing [2], *ISat [2], *OSat [2];
  void configure ();
};

void initMRing (RATE, TIME, int);

#endif
