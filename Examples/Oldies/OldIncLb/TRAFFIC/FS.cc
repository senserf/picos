/* ----------- */
/* File server */
/* ----------- */

void    initFSTraffic (double rqit, double rdle, double srvt, double wrle,
		double wrrt, int srid) {

	// rqit  - mean request interarrival time (bits) (individual per sttn)
	// rdle  - fixed read request length (bits)
	// srvt  - mean service time (etu)
	// wrle  - mean file length (bits)
	// wrrt  - write/read ratio
	// srid  - file server id

	ServSttn = srid;
	WRRatio = wrrt;

	FRQTPattern = create FRQTraffic (MIT_exp+MLE_unf+SCL_off+SPF_off,
		rqit, rdle, rdle);

	for (int i = 0; i < NStations; i++)
		if (i != ServSttn) FRQTPattern -> addSender (i);
	FRQTPattern -> addReceiver (ServSttn);

	FRQTPattern->printDef ("FS Request traffic parameters:");

	FRPTPattern = create FRPTraffic (MIT_exp+MLE_exp+SCL_off, srvt, wrle);

	FRPTPattern->printDef ("FS Reply traffic parameters:");

	// No station groups are defined: messages of this pattern are
	// never generated explicitly
		
	for (i = 0; i < NStations; i++) {
	    Node *s;
	    if (i != ServSttn) {
		    TheStation = s = (Node*) idToStation (i);
		    // This will calculate the service delay
		    s->FSRDel = create RVariable;
	    }
	}

	TheStation = System;
	FSRDel = create RVariable;

	for (i = 0; i < NStations; i++) {
		TheStation = idToStation (i);
		// Start the non-standard client process
		if (i != ServSttn) {
			create FSClient;
		} else {
			// And the request processor
			create FServer;
		}
	}
}

int     getFSPacket (Packet *buffer, long min, long max, long frame) {

	// Acquires a new packet for transmission

#ifdef  RESPONSE_PRIORITY
	// Try to acquire a response packet first
	if (FRPTPattern->getPacket (buffer, min, max, frame) == NO) {
#endif
		if (Client->getPacket (buffer, min, max, frame)) {
			if (buffer->TP == FRQTPattern->getId ()) {
				// A request: initialize the start time
				((Node*)TheStation) -> FSTime =
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

int     getFSPacket (Packet *buffer, QUALTYPE qq, long min, long max,
		long frame) {

	// A qualified version of the above

#ifdef  RESPONSE_PRIORITY
	// Try to acquire a response packet first
	if (FRPTPattern->getPacket (buffer, qq, min, max, frame) == NO) {
#endif
		if (Client->getPacket (buffer, qq, min, max, frame)) {
			if (buffer->TP == FRQTPattern->getId ()) {
				// A request: initialize the start time
				((Node*)TheStation) -> FSTime =
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

FSClient::perform {

	// The code of the non-standard client process

    long  ml;

	state Wait:
		// Generate inter-arrival time and sleep
		Timer->wait (FRQTPattern->genMIT (), NewRequest);
	state NewRequest:
		if (rnd (SEED_traffic) < WRRatio) {
			// A write request
			ml = FRPTPattern->genMLE ();
			S->FSRQType = RQST_WR;
		} else {
			ml = FRQTPattern->genMLE ();
			S->FSRQType = RQST_RD;
		}

		FRQTPattern->genCGR (S);
		// Generate the request message
		FRQTPattern->genMSG (S->getId (), FRQTPattern->genRCV (), ml);
		// And wait until the request has been processed
		Signal->wait (FResponseReceived, Wait);
}

FServer::perform {

	// The code of the request processor

    Node  *rqs;
    long  ml;

	state Wait:
		// Wait for a new request
		if (Queue->get (FRQUEUE) == NULL) {
			Queue->wait (FRQUEUE, Wait);
		} else {
			S->FSSender = (Node*) TheItem;
			Timer->wait (FRPTPattern->genMIT (), Done);
		}
	state Done:
		// The request has been processed
		rqs = S->FSSender;
		if (rqs->FSRQType == RQST_RD) {
			ml = FRPTPattern->genMLE ();
		} else {
			ml = FRQTPattern->genMLE ();
		}

		FRPTPattern->genMSG (S, rqs, ml);
		proceed (Wait);
}

void	FRQTraffic::pfmMRC (Packet *p) {

	// Upon reception of a request message

	Queue->put (idToStation (p->Sender), FRQUEUE);
}

void	FRPTraffic::pfmMRC (Packet*) {

	// Upon reception of a response message

	double t;

	Signal->send (FResponseReceived);
	t = (Time - ((Node*)TheStation)->FSTime) * Itu;
	FSRDel->update (t);
	((Node*)TheStation)->FSRDel->update (t);
}

void	printFSPFM () {

// Prints performance measures

	Node *s;

	print ("Local file service delay time:\n\n");

	for (int i = 0; i < NStations; i++) {
		if (i == ServSttn) continue;
		s = (Node*)(idToStation (i));
		if (i == 0)
			s->FSRDel->printACnt ();
		else
			s->FSRDel->printSCnt ();
	}
	print ("\n");

	FSRDel->printCnt ("Global file service delay:");
};
