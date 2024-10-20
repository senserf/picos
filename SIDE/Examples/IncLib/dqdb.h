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

#ifndef __dqdb_h__
#define __dqdb_h__

// Constants associated with commercial DQDB

#define SlotML 8                    // Slot marker length in bits
#define SegmPL 384                  // Segment payload length in bits
#define SegmFL 32                   // Segment header length in bits
#define SegmWL (SegmPL+SegmFL+2)    // Segment window length in bits

#define CTolerance  0.0001          // Clock tolerance

#define SLOT NONE       // The type of the packet representing slot markers
#define FULL PF_usr0    // The full/empty status of the slot
#define RQST PF_usr1    // The request flag in the slot marker

#endif


