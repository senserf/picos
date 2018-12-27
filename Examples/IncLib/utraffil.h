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

#ifndef __utrafficl_h__
#define __utrafficl_h__

// Uniform traffic (all stations) with Poisson arrival and exponentially
// distributed message length. The same as utraffic.[ch], except that here
// we calculate message access time individually for each station -- to
// see how fair/unfair the protocol is.

#define MAXSTATIONS 256   // Increase if SMURPH starts complaining

traffic UTraffic {
  void pfmMTR (Packet*);
  exposure;
};

station ClientInterface virtual {
  Packet Buffer;
  RVariable *MAT;
  Boolean ready (Long, Long, Long);
  void configure ();
};

void initTraffic ();
void printLocalMeasures ();

#endif
