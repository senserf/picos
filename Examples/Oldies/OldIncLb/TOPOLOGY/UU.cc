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
/* U-shaped unidirectional busses.                            */
/* ---------------------------------------------------------- */

void    initUUTopology (DISTANCE d, DISTANCE t, RATE tr, TIME at = TIME_0) {

	//   d  - distance between two consecutive stations
	//   t  - turn length
	//  tr  - transmission rate
	//  at  - link archival time

	int             i, j;
	// Asssuming that type Node describes a network station and that
	// stations have been created. After all we cannot know what
	// kind of arguments they may need
	Node	        *Curr;

	Assert (NStations > 0, "initUUTopology: stations not created");

	for (i = 0; i < 2; i++)
		Bus [i] = create LINKTYPE (NStations + NStations, at);

	// Station 0 is most removed from the turn on BUS0 and the closest
	// to the turn on BUS1

	// BUS0

	for (i = 0; i < NStations; i++) {
	    TheStation = Curr = (Node*) idToStation (i);
	    // TheStation is set so that ports can be created
	    ((UUPORTS*)Curr)->mkPorts (tr);
	    Curr->OPort [0] -> connect (Bus [BUS0]);
	    for (j = 0; j < i; j++)
		((Node*)idToStation (j))->OPort[0]->setDTo (Curr->OPort[0],
			d * (i - j));
	}

	for (i = NStations - 1; i >= 0; i--) {
	    Curr = (Node*) idToStation (i);
	    Curr->IPort [0] -> connect (Bus [BUS0]);
	    for (j = 0; j < NStations; j++)
		((Node*)idToStation (j))->OPort[0]->setDTo (Curr->IPort[0],
			d * (NStations - 1 - j) + t + d * (NStations - 1 - i));
	    for (j = NStations - 1; j > i; j--)
		((Node*)idToStation (j))->IPort[0]->setDTo (Curr->IPort[0],
			d * (j - i));
	}

	// BUS1

	for (i = NStations - 1; i >= 0; i--) {
	    Curr = (Node*) idToStation (i);
	    Curr->OPort [1] -> connect (Bus [BUS1]);
	    for (j = NStations - 1; j > i; j--)
		((Node*)idToStation (j))->OPort[1]->setDTo (Curr->OPort[1],
			d * (j - i));
	}

	for (i = 0; i < NStations; i++) {
	    Curr = (Node*) idToStation (i);
	    Curr->IPort [1] -> connect (Bus [BUS1]);
	    for (j = NStations - 1; j >= 0; j--)
		((Node*)idToStation (j))->OPort[1]->setDTo (Curr->IPort[1],
			d * j + t + d * i);
	    for (j = 0; j < i; j++)
		((Node*)idToStation (j))->IPort[1]->setDTo (Curr->IPort[1],
			d * (i - j));
	}
}
