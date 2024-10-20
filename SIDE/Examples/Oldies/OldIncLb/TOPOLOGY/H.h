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

/* ---------------------------------------------------------- */
/* This  file  contains  declarations  for a network topology */
/* with two unidirectional links, e.g., FASNET, DQDB          */
/* ---------------------------------------------------------- */

station HPORTS virtual {

    Port *LRPort,       // Port for the left-to-right bus
	 *RLPort;       // Port for the right-to-left bus

    void mkPorts (RATE r) {
	 LRPort = create Port (r);
	 RLPort = create Port (r);
    };
};

#ifndef	LINKTYPE
#define	LINKTYPE PLink
#endif

LINKTYPE   *LRLink, *RLLink;

void   		initHTopology (DISTANCE, RATE, TIME);
inline	void    initHTopology (DISTANCE d, RATE r) {
	initHTopology (d, r, TIME_0);
};

#define	LEFT	0
#define	RIGHT	1	// Direction indication
