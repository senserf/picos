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

/* ----------------------------- */
/* Dumpling topology (segmented) */
/* ----------------------------- */

	// By defining XMITPORT you can chose a variant with a separate
	// port for transmission. Then you can set the length of the link
	// connecting this port to the "Left" (outgoing) port.

station DMPORTS virtual {

    Port *ULPort,       // Upper left
	 *URPort,       // Upper right
	 *LLPort,       // Lower left
	 *LRPort;       // Lower right

#ifdef	XMITPORT
    Port *UXPort,	// Upper transmission port
	 *LXPort;	// Lower transmission port
#endif

    void mkPorts (RATE r) {
	 URPort = create Port, "UR", (r);
	 ULPort = create Port, "UL", (r);
#ifdef	XMITPORT
	 UXPort = create Port, "UX", (r);
#endif

	 LRPort = create Port, "LR", (r);
	 LLPort = create Port, "LL", (r);
#ifdef	XMITPORT
	 LXPort = create Port, "LX", (r);
#endif
    };
};

#ifndef LINKTYPE
#define LINKTYPE PLink
#endif

LINKTYPE   **USegments, // Upper segments (array)
	   **LSegments, // Lower segments (array)
	   *UConnect,   // Upper connecting segment
	   *LConnect;   // Lower connecting segment

void    initDMTopology (DISTANCE, DISTANCE,
#ifdef	XMITPORT
                                       DISTANCE,
#endif
						RATE, TIME);
inline void initDMTopology (DISTANCE b, DISTANCE c, RATE r) {
	initDMTopology (b, c, 
#ifdef	XMITPORT
			DISTANCE_0,
#endif
				r, TIME_0);
};

#ifdef	XMITPORT
inline void initDMTopology (DISTANCE b, DISTANCE c, DISTANCE d, RATE r) {
	initDMTopology (b, c, d, r, TIME_0);
};
#endif
