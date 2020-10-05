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

/* ------------- */
/* Ring topology */
/* ------------- */

void    initRTopology (DISTANCE d, RATE tr, TIME at) {

	//   d  - distance between two neighboring stations
	//  tr  - transmission rate
	//  at  - link archival time

	int             i;
	// Asssuming that type Node describes a network station and that
	// stations have been created. After all we cannot know what
	// kind of arguments they may need

	Assert (NStations > 0, "initRTopology: stations not created");

	// Create the ports
	for (i = 0; i < NStations; i++) {
		TheStation = idToStation (i);
		((RPORTS*)((Node*)TheStation)) -> mkPorts (tr);
	}
	// Create the links
	RLinks = new LINKTYPE* [NStations];
	for (i = 0; i < NStations; i++) {
		Port *p1, *p2;
		int  j = (int) ((i+1) % NStations);
		RLinks [i] = create LINKTYPE,
			form ("Lk%03d-->%03d", i, j), (2, at);
		p1 = ((RPORTS*)(((Node*)idToStation (i)))) -> OPort;
		p2 = ((RPORTS*)(((Node*)idToStation (j)))) -> IPort;
		p1->connect (RLinks [i]);
		p2->connect (RLinks [i]);
		p1->setDTo (p2, d);
	}
}
