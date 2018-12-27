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

/* ------------------------------ */
/* Spiral topology initialization */
/* ------------------------------ */

#ifdef	XMITPORT
	// Ports per segment
#define	PPS	3
#else
#define	PPS	2
#endif

void    initSPTopology (DISTANCE d, DISTANCE s, 
#ifdef	XMITPORT
					     DISTANCE x,
#endif
							RATE tr, TIME at) {

	//   d  - distance between two consecutive stations (segment length)
	//   s  - length of a connector segment
#ifdef	XMITPORT
	//   x  - the distance between the transmission port and the "Left"
	//        (outgoing) port. We assume that the transmission port is
	//        connected to the "Left" port.
#endif
	//  tr  - transmission rate
	//  at  - link archival time

	int             i, j;
	// Asssuming that type Node describes a network station and that
	// stations have been created
	Node            *sa, *sb;
	LINKTYPE  	*l;

	Assert (NStations > 0, "initSPopology: stations not created");

	// Create the link arrays
	for (i = 0; i < NLOOPS; i++)
		Segments [i] = new LINKTYPE* [NStations-1];

	for (i = 0; i < NStations; i++) {
		// Create the ports
		TheStation = idToStation (i);
		((SPPORTS*)((Node*)TheStation))->mkPorts (tr);
	}

	for (i = 0; i < NStations-1; i++) {
	  // Take care of the bus segments
	  sa = (Node*) idToStation (i+1);
	  sb = (Node*) idToStation (i);
	  for (j = 0; j < NLOOPS; j++) {
	    l = create LINKTYPE, form ("S%1d:%03d->%03d", j, i+1, i), (PPS, at);
	    sa->LPort [j]->connect (l);
#ifdef	XMITPORT
	    sa->XPort [j]->connect (l);
#endif
	    sb->RPort [j]->connect (l);
#ifdef	XMITPORT
	    sa->LPort [j]->setDTo (sa->XPort  [j], x);
	    sa->XPort [j]->setDTo (sb->RPort [j], d);
#else
	    sa->LPort [j]->setDTo (sb->RPort [j], d);
#endif
	    (Segments [j])[i] = l;
	  }
	}

	// Now the connector segments
	sa = (Node*) idToStation (0);
	sb = (Node*) idToStation (NStations-1);
	for (j = 0; j < NLOOPS; j++) {
	  Connect [j] = 
	    l = create LINKTYPE, form ("C%1d:000->%03d", j, NStations-1),
	      (PPS, at);
	  // Source port index is 'j', destination port index is:
	  i = (j + 1) % NLOOPS;

 	  sa->LPort [j]->connect (l);
#ifdef	XMITPORT
          sa->XPort [j]->connect (l);
#endif
	  sb->RPort [i]->connect (l);

#ifdef	XMITPORT
	  sa->LPort [j]->setDTo (sa->XPort [j], x);
          sa->XPort [j]->setDTo (sb->RPort [i], s);
#else
	  sa->LPort [j]->setDTo (sb->RPort [i], s);
#endif
	}
}
