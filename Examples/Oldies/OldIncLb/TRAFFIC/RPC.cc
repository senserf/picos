/* --------------------------- */
/* Uniform RPC traffic pattern */
/* --------------------------- */

void    initRPCTraffic (double rqit, double rqle, double srvt, double rple) {

	// rqit  - mean request interarrival time (bits) (individual per sttn)
	// rqle  - mean request length (bits)
	// srvt  - mean service time (etu)
	// rple  - mean reply length (bits)

	RQTPattern = create RQTraffic (MIT_exp+MLE_exp+SCL_off+SPF_off,
		rqit, rqle);
	RQTPattern -> addSender ();
	RQTPattern -> addReceiver ();

	RQTPattern->printDef ("RPC Request traffic parameters:");

	RPTPattern = create RPTraffic (MIT_exp+MLE_exp+SCL_off, srvt, rple);

	RPTPattern->printDef ("RPC Reply traffic parameters:");

	// No station groups are defined: messages of this pattern are
	// never generated explicitly
		
	for (int i = 0; i < NStations; i++) {
	    Node *s;
	    TheStation = s = (Node*) idToStation (i);
	    // This will calculate the service delay
	    s->RPCSRDel = create RVariable;
	}
	TheStation = System;
	RPCSRDel = create RVariable;       // Global service delay

	for (i = 0; i < NStations; i++) {
		TheStation = idToStation (i);
		// Start the non-standard client process
		create RPCClient;
		create RPCServer;
	}
}

int     getRPCPacket (Packet *buffer, long min, long max, long frame) {

	// Acquires a new packet for transmission

#ifdef  RESPONSE_PRIORITY
	// Try to acquire a response packet first
	if (RPTPattern->getPacket (buffer, min, max, frame) == NO) {
#endif
		if (Client->getPacket (buffer, min, max, frame)) {
			if (buffer->TP == RQTPattern->getId ()) {
				// A request: initialize the start time
				((Node*)TheStation)->RPCSTime =
					buffer -> QTime;
			}
			return (YES);
		} else {
			return (NO);
		}
#ifdef  RESPONSE_PRIORITY
	}
	return (YES);
#endif
}

int     getRPCPacket (Packet *buffer, QUALTYPE qq, long min, long max,
	long frame) {

	// A qualified version of the above

#ifdef  RESPONSE_PRIORITY
	// Try to acquire a response packet first
	if (RPTPattern->getPacket (buffer, qq, min, max, frame) == NO) {
#endif
		if (Client->getPacket (buffer, qq, min, max, frame)) {
			if (buffer->TP == RQTPattern->getId ()) {
				// A request: initialize the start time
				((Node*)TheStation)->RPCSTime =
					buffer -> QTime;
			}
			return (YES);
		} else {
			return (NO);
		}
#ifdef  RESPONSE_PRIORITY
	}
	return (YES);
#endif
}

RPCClient::perform {

	// The code of the non-standard client process

	state Wait:
		// Generate inter-arrival time and sleep
		Timer->wait (RQTPattern->genMIT (), NewRequest);
	state NewRequest:
		RQTPattern->genCGR (S);
		// Generate the request message
		RQTPattern->genMSG (S->getId (), RQTPattern->genRCV (),
			RQTPattern->genMLE ());
		// And wait until the request has been processed
		Signal->wait (ResponseReceived, Wait);
}

RPCServer::perform {

	// The code of the request processor

	state Wait:
		// Wait for a new request
		if (Queue->get (RQUEUE) == NULL) {
			Queue->wait (RQUEUE, Wait);
		} else {
			S -> RPCSender = (Node*) TheItem;
			Timer->wait (RPTPattern->genMIT (), Done);
		}
	state Done:
		// The request has been processed
		RPTPattern->genMSG (S, S -> RPCSender, RPTPattern->genMLE ());
		proceed (Wait);
}

void	RQTraffic::pfmMRC (Packet *p) {

	// Upon reception of a request message

	Queue->put (idToStation (p->Sender), RQUEUE);
}

void	RPTraffic::pfmMRC (Packet*) {

	// Upon reception of a response message

	double t;

	Signal->send (ResponseReceived);
	t = (Time - ((Node*)TheStation) -> RPCSTime) * Itu;
	RPCSRDel->update (t);
	((Node*)TheStation)->RPCSRDel->update (t);
}

void    printRPCPFM () {

// Prints performance measures

	Node *s;

	print ("Local service delay time:\n\n");

	for (int i = 0; i < NStations; i++) {
		s = (Node*)(idToStation (i));
		if (i == 0)
			s->RPCSRDel->printACnt ();
		else
			s->RPCSRDel->printSCnt ();
	}
	print ("\n");

	RPCSRDel->printCnt ("Global service delay:");
};
