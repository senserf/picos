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

#ifndef __utrafficn_h__
#define __utrafficn_h__

// Uniform traffic (all stations) with Poisson arrival and exponentially
// distributed message length. Each station has the requested number of
// buffers which are filled independently. This traffic pattern is used
// in mesh networks (e.g., MNA) in which a single station runs a number
// of independent transmitters.


// Note: If you want to use non-standard message/packet types, just make
// sure that the two constants below are defined before this file is
// included.

#ifndef PACKET_TYPE
#define PACKET_TYPE Packet
#endif

#ifndef MESSAGE_TYPE
#define MESSAGE_TYPE Message
#endif

station ClientInterface virtual {
  PACKET_TYPE **Buffer;
  Boolean ready (int, Long, Long, Long);
  void configure (int);
};

void initTraffic (int fml = NO);

#endif
