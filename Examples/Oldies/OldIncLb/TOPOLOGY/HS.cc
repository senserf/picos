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
/* Topology  initialization  code  for  a  network  with  two */
/* unidirectional segmented busses                            */
/* ---------------------------------------------------------- */

void    initHSTopology (DISTANCE d, RATE tr, TIME at = TIME_0) {

	//   d  - distance between two consecutive stations (segment length)
	//  tr  - transmission rate
	//  at  - link archival time

	int             i;
	// Asssuming that type Node describes a network station and that
	// stations have been created. After all we cannot know what
	// kind of arguments they may need
	Node	        *sa, *sb;

	Assert (NStations > 0, "initHTopology: stations not created");

	// Create the link arrays
	LRLink = new LINKTYPE* [NStations+1];
	RLLink = new LINKTYPE* [NStations+1];

	// Now create the segments and connect them to the stations
	for (i = 0; i < NStations; i++) {
		TheStation = idToStation (i);
		((HSPORTS*)((Node*)idToStation (i)))->mkPorts (tr);
	}
	for (i = 0; i < NStations+1; i++) {
	    LINKTYPE  *l;

	    if (i == 0) {
	      l = create LINKTYPE, "LR:xxx->000", (1, at);
	      sb = (Node*) idToStation (i);
	      sb->ILRPort->connect (l);
	    } else if (i == NStations) {
	      l = create LINKTYPE, form ("LR:%03d->xxx", i-1), (2, at);
	      sa = (Node*) idToStation (i-1);
	      sa->SLRPort->connect (l);
	      sa->OLRPort->connect (l);
	      sa->SLRPort->setDTo (sa->OLRPort, 0);
            } else {
	      l = create LINKTYPE, form ("LR:%03d->%03d", i-1, i), (3, at);
	      sa = (Node*) idToStation (i-1);
	      sb = (Node*) idToStation (i);
	      sa->SLRPort->connect (l);
	      sa->OLRPort->connect (l);
	      sb->ILRPort->connect (l);
	      sa->SLRPort -> setDTo (sa->OLRPort, 0);
	      sa->SLRPort -> setDTo (sb->ILRPort, d);
	      sa->OLRPort -> setDTo (sb->ILRPort, d);
	    }
	    LRLink [i] = l;

	    if (i == 0) {
	      l = create LINKTYPE, form ("RL:xxx->%03d", NStations-1), (1, at);
	      sb = (Node*) idToStation (NStations-i-1);
	      sb->IRLPort->connect (l);
	    } else if (i == NStations) {
	      l = create LINKTYPE, "RL:000->xxx", (2, at);
	      sa = (Node*) idToStation (NStations-i);
	      sa->SRLPort->connect (l);
	      sa->ORLPort->connect (l);
	      sa->SRLPort->setDTo (sa->ORLPort, 0);
            } else {
	      l = create LINKTYPE, form ("RL:%03d->%03d", NStations-i,
	        NStations-i-1), (3, at);
	      sa = (Node*) idToStation (NStations-i);
	      sb = (Node*) idToStation (NStations-i-1);
	      sa->SRLPort->connect (l);
	      sa->ORLPort->connect (l);
	      sb->IRLPort->connect (l);
	      sa->SRLPort -> setDTo (sa->ORLPort, 0);
	      sa->SRLPort -> setDTo (sb->IRLPort, d);
	      sa->ORLPort -> setDTo (sb->IRLPort, d);
	    }
	    RLLink [i] = l;
	}
}
