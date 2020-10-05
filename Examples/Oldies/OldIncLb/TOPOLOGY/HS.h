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
/* with two unidirectional busses which are segmented in such */
/* a way that two neigbouring stations are connected by a se- */
/* parate link.                                               */
/* ---------------------------------------------------------- */

station HSPORTS virtual {

    Port *ILRPort,      // Incoming port for the left-to-right bus
         *SLRPort,	// Switching port for the left-to-right bus
	 *OLRPort,      // Outgoing port for the left-to-right bus
         *IRLPort,      // Incoming port for the right-to-left bus
         *SRLPort,	// Switching port for the right-to-left bus
	 *ORLPort;      // Outgoing port for the right-to-left bus

    void mkPorts (RATE r) {
	 ILRPort = create Port, "ILR", (r);
	 SLRPort = create Port, "SLR", (r);
	 OLRPort = create Port, "OLR", (r);
	 IRLPort = create Port, "IRL", (r);
	 SRLPort = create Port, "SRL", (r);
	 ORLPort = create Port, "ORL", (r);
    };
};

#ifndef	LINKTYPE
#define	LINKTYPE PLink
#endif

LINKTYPE   **LRLink, **RLLink;	// Link arrays

void    initHSTopology (DISTANCE, RATE, TIME at = TIME_0);

#define	LEFT	0
#define	RIGHT	1	// Direction indication
