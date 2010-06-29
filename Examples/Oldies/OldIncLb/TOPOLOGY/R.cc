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
