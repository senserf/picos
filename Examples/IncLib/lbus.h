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

#ifndef __lbus_h__
#define __lbus_h__

// Type announcements for 'lbus.cc': a strictly linear bus with stations spaced
// equally along it.

station BusInterface virtual {
  Port *Bus;
  void configure ();
};

void initBus (RATE, DISTANCE, int, TIME at);

// Note: the following declaration is only for the ATT compiler which doesn't
// accept this: void initBus (RATE, TIME, int, TIME at = TIME_0);

inline void initBus (RATE r, DISTANCE l, int n) {
  initBus (r, l, n, TIME_0);
};

#endif
