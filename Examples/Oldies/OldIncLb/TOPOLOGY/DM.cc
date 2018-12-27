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

/* -------------------------------- */
/* Dumpling topology initialization */
/* -------------------------------- */

#ifdef	XMITPORT
	// Ports per segment
#define	PPS	3
#else
#define	PPS	2
#endif

void    initDMTopology (DISTANCE d, DISTANCE s, 
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

	int             i;
	// Asssuming that type Node describes a network station and that
	// stations have been created. After all we cannot know what
	// kind of arguments they may need
	Node            *sa, *sb;

	Assert (NStations > 0, "initDMopology: stations not created");

	// Create the link arrays
	USegments = new LINKTYPE* [NStations-1];
	LSegments = new LINKTYPE* [NStations-1];

	for (i = 0; i < NStations; i++) {
		// Create the ports
		TheStation = idToStation (i);
		((DMPORTS*)((Node*)TheStation))->mkPorts (tr);
	}
	for (i = 0; i < NStations-1; i++) {
	    // Take care of the bus segments
	    LINKTYPE  *l;

	    l = create LINKTYPE, form ("US:%03d->%03d", i+1, i), (PPS, at);
	    sa = (Node*) idToStation (i+1);
	    sb = (Node*) idToStation (i);

	    sa->ULPort->connect (l);
#ifdef	XMITPORT
	    sa->UXPort->connect (l);
#endif
	    sb->URPort->connect (l);
#ifdef	XMITPORT
	    sa->ULPort->setDTo (sa->UXPort, x);
	    sa->UXPort->setDTo (sb->URPort, d);
#else
	    sa->ULPort->setDTo (sb->URPort, d);
#endif
	    USegments [i] = l;

	    l = create LINKTYPE, form ("LS:%03d->%03d", i+1, i), (PPS, at);
	    sa->LLPort->connect (l);
#ifdef	XMITPORT
	    sa->LXPort->connect (l);
#endif
	    sb->LRPort->connect (l);
#ifdef	XMITPORT
	    sa->LLPort->setDTo (sa->LXPort, x);
	    sa->LXPort->setDTo (sb->LRPort, d);
#else
	    sa->LLPort->setDTo (sb->LRPort, d);
#endif
	    LSegments [i] = l;
	}

	// Now the connector segments
	UConnect = create LINKTYPE, form ("UC:000->%03d", NStations-1),
								(PPS, at);
	LConnect = create LINKTYPE, form ("LC:000->%03d", NStations-1),
								(PPS, at);

	sa = (Node*) idToStation (0);
	sb = (Node*) idToStation (NStations-1);

	sa->ULPort->connect (UConnect);
#ifdef	XMITPORT
        sa->UXPort->connect (UConnect);
#endif
	sb->LRPort->connect (UConnect);

	sa->LLPort->connect (LConnect);
#ifdef	XMITPORT
        sa->LXPort->connect (LConnect);
#endif
	sb->URPort->connect (LConnect);

#ifdef	XMITPORT
	sa->ULPort->setDTo (sa->UXPort, x);
        sa->UXPort->setDTo (sb->LRPort, s);

	sa->LLPort->setDTo (sa->LXPort, x);
        sa->LXPort->setDTo (sb->URPort, s);
#else
	sa->ULPort->setDTo (sb->LRPort, s);
	sa->LLPort->setDTo (sb->URPort, s);
#endif
}
