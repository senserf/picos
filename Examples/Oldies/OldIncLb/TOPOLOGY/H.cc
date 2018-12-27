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
/* Topology  initialization  code  for  a  network  with  two */
/* unidirectional busses                                      */
/* ---------------------------------------------------------- */

void    initHTopology (DISTANCE d, RATE tr, TIME at) {

	//   d  - distance between two consecutive stations
	//  tr  - transmission rate
	//  at  - link archival time

	long            i;
	// Asssuming that type Node describes a network station and that
	// stations have been created. After all we cannot know what
	// kind of arguments they may need
	Node	        *Prev, *Curr;

	Assert (NStations > 0, "initHTopology: stations not created");

	// Create the links
	LRLink = create LINKTYPE (NStations, at);
	RLLink = create LINKTYPE (NStations, at);
		
	for (Prev = NULL, i = 0; i < NStations; i++, Prev = Curr) {
	    TheStation = Curr = (Node*) idToStation (i);
	    // TheStation is set so that ports can be created
	    ((HPORTS*)Curr)->mkPorts (tr);
	    // Connect the LR port to the corresponding link
	    Curr->LRPort -> connect (LRLink);
	    // And set up the distance
	    if (i > 0) {
	       Prev->LRPort -> setDTo (Curr->LRPort, d);
	    }
	}
	for (i = NStations - 1; i >= 0; i--, Prev = Curr) {
	    Curr = (Node*) idToStation (i);
	    // Connect the RL port
	    Curr->RLPort -> connect (RLLink);
	    // And set up the distance
	    if (i < NStations - 1) {
	       Prev->RLPort -> setDTo (Curr->RLPort, d);
            }
	}
}
