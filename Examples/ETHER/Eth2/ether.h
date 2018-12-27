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

#define MinPL      368    // Minimum payload length in bits (frame excluded)
#define MaxPL    12000    // Maximum payload length in bits (frame excluded)
#define FrameL     208    // The combined length of packet header and trailer

#define PSpace      96    // Inter-packet space length in bits
#define JamL        32    // Length of the jamming signal in bits
#define TwoL       512    // Maximum round-trip delay in bits

#define CTolerance 0.0001 // Clock tolerance

#define TRate        1    // Transmission rate: 1 ITU per bit
