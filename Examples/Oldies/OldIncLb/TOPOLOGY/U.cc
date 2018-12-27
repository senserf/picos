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

/* ---------------------------------------------------------- */
/* Topology  initialization  code  for  a  network  with  one */
/* U-shaped unidirectional bus.                               */
/* ---------------------------------------------------------- */

void    initUTopology (DISTANCE d, DISTANCE t, RATE tr, TIME at) {

	//   d  - distance between two consecutive stations
	//   t  - turn length
	//  tr  - transmission rate
	//  at  - link archival time

	long            i, j;
	// Asssuming that type Node describes a network station and that
	// stations have been created. After all we cannot know what
	// kind of arguments they may need
	Node	        *Curr;

	Assert (NStations > 0, "initUTopology: stations not created");

	Bus = create LINKTYPE (NStations + NStations, at);
		
	// Station 0 is most removed from the turn

	for (i = 0; i < NStations; i++) {
	    TheStation = Curr = (Node*) idToStation (i);
	    // TheStation is set so that ports can be created
	    ((UPORTS*)Curr)->mkPorts (tr);
	    Curr->OPort->connect (Bus);
	    for (j = 0; j < i; j++)
		((Node*)idToStation (j))->OPort->setDTo (Curr->OPort,
			d * (i - j));
	}

	for (i = NStations - 1; i >= 0; i--) {
	    Curr = (Node*) idToStation (i);
	    Curr->IPort->connect (Bus);
	    for (j = 0; j < NStations; j++)
		((Node*)idToStation (j))->OPort->setDTo (Curr->IPort,
			d * (NStations - 1 - j) + t + d * (NStations - 1 - i));
	    for (j = NStations - 1; j > i; j--)
		((Node*)idToStation (j))->IPort->setDTo (Curr->IPort,
			d * (j - i));
	}
}
