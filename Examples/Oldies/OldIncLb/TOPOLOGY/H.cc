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
