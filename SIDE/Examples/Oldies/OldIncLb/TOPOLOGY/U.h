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
/* with an U-shaped single unidirectional link.               */
/* ---------------------------------------------------------- */

station UPORTS virtual {

    Port *OPort,        // Output port: towards the turn
	 *IPort;        // Input port: from the turn

    void mkPorts (RATE r) {
	 OPort = create Port (r);
	 IPort = create Port (r);
    };
};

#ifndef	LINKTYPE
#define	LINKTYPE PLink
#endif

LINKTYPE   *Bus;

void    initUTopology (DISTANCE, DISTANCE, RATE, TIME at);
inline   void    initUTopology (DISTANCE d, DISTANCE e, RATE r) {
	initUTopology (d, e, r, TIME_0);
};
