/* ---------------------------------------------------------- */
/* Two    uniform    traffic    patterns   with   exponential */
/* inter-arrival time and length distributions                */
/* ---------------------------------------------------------- */

void    initU2Traffic (double mit0, double mle0, double mit1, double mle1) {

	U0TPattern = create U0Traffic (MIT_exp+MLE_exp, mit0, mle0);
	U0TPattern->addSender ();
	U0TPattern->addReceiver ();
	U0TPattern->printDef ("U0 traffic pattern parameters:");

	U1TPattern = create U1Traffic (MIT_exp+MLE_exp, mit1, mle1);
	U1TPattern->addSender ();
	U1TPattern->addReceiver ();
	U1TPattern->printDef ("U1 traffic pattern parameters:");
		
#ifdef  LOCAL_MEASURES
	for (int i = 0; i < NStations; i++) {
	    Node *s;
	    TheStation = s = (Node*) idToStation (i);
	    s->U0AMDel = create RVariable;
	    s->U0APAcc = create RVariable;
	    s->U1AMDel = create RVariable;
	    s->U1APAcc = create RVariable;
	    s->UAMDel = create RVariable;
	    s->UAPAcc = create RVariable;
	}
#endif
}

#ifdef  LOCAL_MEASURES
void U0Traffic::pfmMRC (Packet *p) {

	double  d;
	Node    *s;

	d = Itu * (double) (Time - p->QTime);
	s = ((Node*) idToStation (p->Sender));
	s -> UAMDel -> update (d);
	s -> U0AMDel -> update (d);
};
	
void U0Traffic::pfmPTR (Packet *p) {

	double  d;
	Node    *s;

	d = Itu * (double) (Time - p->TTime);
	s = ((Node*) idToStation (p->Sender));
	s -> UAPAcc -> update (d);
	s -> U0APAcc -> update (d);
};

void U1Traffic::pfmMRC (Packet *p) {

	double  d;
	Node    *s;

	d = Itu * (double) (Time - p->QTime);
	s = ((Node*) idToStation (p->Sender));
	s -> UAMDel -> update (d);
	s -> U1AMDel -> update (d);
};
	
void U1Traffic::pfmPTR (Packet *p) {

	double  d;
	Node    *s;

	d = Itu * (double) (Time - p->TTime);
	s = ((Node*) idToStation (p->Sender));
	s -> UAPAcc -> update (d);
	s -> U1APAcc -> update (d);
};
#endif

void    printU2PFM () {

	int     i;
	Node    *s;

	U0TPattern->printPfm ("U0 traffic pattern global performance measures");
	U1TPattern->printPfm ("U1 traffic pattern global performance measures");
	Client->printPfm ("Combined performance measures");

#ifdef  LOCAL_MEASURES

	print ("U0 Local packet access time:\n\n");
	for (i = 0; i < NStations; i++) {
	    s = (Node*)(idToStation (i));
	    if (i == 0)
		s->U0APAcc->printACnt ();
	    else
		s->U0APAcc->printSCnt ();
	}
	print ("\n\n");
	print ("U0 Local message delay:\n\n");
	for (i = 0; i < NStations; i++) {
	    s = (Node*)(idToStation (i));
	    if (i == 0)
		s->U0AMDel->printACnt ();
	    else
		s->U0AMDel->printSCnt ();
	}
	print ("\n\n");
	print ("U1 Local packet access time:\n\n");
	for (i = 0; i < NStations; i++) {
	    s = (Node*)(idToStation (i));
	    if (i == 0)
		s->U1APAcc->printACnt ();
	    else
		s->U1APAcc->printSCnt ();
	}
	print ("\n\n");
	print ("U1 Local message delay:\n\n");
	for (i = 0; i < NStations; i++) {
	    s = (Node*)(idToStation (i));
	    if (i == 0)
		s->U1AMDel->printACnt ();
	    else
		s->U1AMDel->printSCnt ();
	}
	print ("\n\n");
	print ("Combined local packet access time:\n\n");
	for (i = 0; i < NStations; i++) {
	    s = (Node*)(idToStation (i));
	    if (i == 0)
		s->UAPAcc->printACnt ();
	    else
		s->UAPAcc->printSCnt ();
	}
	print ("\n\n");
	print ("Combined local message delay:\n\n");
	for (i = 0; i < NStations; i++) {
	    s = (Node*)(idToStation (i));
	    if (i == 0)
		s->UAMDel->printACnt ();
	    else
		s->UAMDel->printSCnt ();
	}
	print ("\n\n");

	print ("U0 Message queues at the end of simulation:\n\n");
	print ("Station    Messages        Bits\n");
	for (i = 0; i < NStations; i++) {
	    long bcnt = 0L;
	    int  mcnt = 0;
	    Message **mq = idToStation (i)->MQHead, *m;

	    print (i, 7);
	    if (mq != NULL) {
		for (m = mq [U0TPattern->getId ()]; m != NULL; mcnt++,
			bcnt += m->Length, m = m->next);
	    }
	    print (mcnt, 12); print (bcnt, 12); print ("\n");
	}
	print ("\n");

	print ("U1 Message queues at the end of simulation:\n\n");
	print ("Station    Messages        Bits\n");
	for (i = 0; i < NStations; i++) {
	    long bcnt = 0L;
	    int  mcnt = 0;
	    Message **mq = idToStation (i)->MQHead, *m;

	    print (i, 7);
	    if (mq != NULL) {
		for (m = mq [U1TPattern->getId ()]; m != NULL; mcnt++,
			bcnt += m->Length, m = m->next);
	    }
	    print (mcnt, 12); print (bcnt, 12); print ("\n");
	}
	print ("\n");

	print ("Combined message queues at the end of simulation:\n\n");
	print ("Station    Messages        Bits\n");
	for (i = 0; i < NStations; i++) {
	    long bcnt = 0L;
	    int  mcnt = 0, tp;
	    Message **mq = idToStation (i)->MQHead, *m;

	    print (i, 7);
	    if (mq != NULL) {
	      for (tp = 0; tp < NTraffics; tp++) {
		for (m = mq [tp]; m != NULL; mcnt++, bcnt += m->Length,
			m = m->next);
	      }
	    }
	    print (mcnt, 12); print (bcnt, 12); print ("\n");
	}
	print ("\n");
#endif
}
