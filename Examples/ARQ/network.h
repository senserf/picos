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

// Network types

packet	Frame {
	// One extra attribute in the header -> the serial number
	int	SN;
};

packet	Ack {
	// Just the serial number
	int	SN;
};

// The traffic generator and absorber
traffic	TrGen (Message, Frame);

extern	PLink	*Link1, *Link2;

extern	TrGen	*UpperLayer;

extern	int	MinFrameLength, MaxFrameLength, AckLength;
