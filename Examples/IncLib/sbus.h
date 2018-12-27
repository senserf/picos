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

#ifndef __sbus_h__
#define __sbus_h__

// Type announcements for 'sbus.cc': a single S-shaped unidirectional bus with
// two ports per station.

station SBusInterface virtual {
  Port *OBus,    // Output port
       *IBus;    // Input port
  void configure ();
};

void initSBus (RATE, DISTANCE, DISTANCE, int);

#endif
