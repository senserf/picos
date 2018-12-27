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
/* Simple  uniform  traffic  pattern,  standard  messages and */
/* packets,   message   length   and    inter-arrival    time */
/* exponentially distributed                                  */
/* ---------------------------------------------------------- */

#ifdef	SNAPSHOT_THROUGHPUT

void UTraffic::pfmPRC (PACKET_TYPE *p) {
	
	STNRcvdBits += p->ILength;
};

SThpMeter::perform {

	state AddSample:
		STBuffer [STBIndex % STBUFSIZE] = Etu *
			(float)((double)STNRcvdBits/ (double)Interval);
		STBIndex++;
	transient Wait:
		STNRcvdBits = TIME_0;
		Timer->wait (Interval, AddSample);
};

void    initUTraffic (double mit, double mle, TIME stintvl, int *sl) {
#else
void    initUTraffic (double mit, double mle, int *sl) {
#endif

	TheStation = System;

#ifdef	SNAPSHOT_THROUGHPUT
	create SThpMeter (stintvl);
#endif

#ifdef	FIXED_MESSAGE_LENGTH
	UTPattern = create UTraffic (MIT_exp+MLE_unf, mit, mle, mle);
#else
	UTPattern = create UTraffic (MIT_exp+MLE_exp, mit, mle);
#endif
	if (sl == NULL) {
	    UTPattern->addSender ();
	    UTPattern->addReceiver ();
	} else {
	    while (*sl != NONE) {
		UTPattern->addSender (*sl);
		UTPattern->addReceiver (*sl);
		sl++;
	    }
	}
	UTPattern->printDef ("Uniform traffic pattern parameters:");
		
#ifdef	LOCAL_MEASURES
	for (int i = 0; i < NStations; i++) {
	    STATION_TYPE *s;
	    TheStation = s = (STATION_TYPE*) idToStation (i);
	    s->UAMDel = create RVariable;
            s->UAPAcc = create RVariable;
	}
#endif
}

#ifdef	LOCAL_MEASURES
#ifdef	LOCAL_MESSAGE_ACCESS_TIME
void UTraffic::pfmMTR (PACKET_TYPE *p) {
#else
void UTraffic::pfmMRC (PACKET_TYPE *p) {
#endif

	double	d;

	d = Itu * (double) (Time - p->QTime);
	((STATION_TYPE*) idToStation (p->Sender)) -> UAMDel -> update (d);
};
	
void UTraffic::pfmPTR (PACKET_TYPE *p) {

	double	d;

	d = Itu * (double) (Time - p->TTime);
	((STATION_TYPE*) idToStation (p->Sender)) -> UAPAcc -> update (d);
};

UTraffic::exposure {
	STATION_TYPE *s;
	int  i, mcnt;
	long bcnt;
	Message **mq, *m;

	Traffic::expose;

	onpaper {

	  exmode 8:

	    print ("Local packet access time:\n\n");
	    for (i = 0; i < NStations; i++) {
		s = (STATION_TYPE*)(idToStation (i));
		if (i == 0)
			s->UAPAcc->printACnt ();
		else
			s->UAPAcc->printSCnt ();
	    }
	    print ("\n\n");

	  exmode 9:

#ifdef	LOCAL_MESSAGE_ACCESS_TIME
	    print ("Local message access time:\n\n");
#else
	    print ("Local message delay:\n\n");
#endif
	    for (i = 0; i < NStations; i++) {
		s = (STATION_TYPE*)(idToStation (i));
		if (i == 0)
			s->UAMDel->printACnt ();
		else
			s->UAMDel->printSCnt ();
	    }
	    print ("\n\n");

	  exmode 10:

	    print ("UT Message queues at the end of simulation:\n\n");
	    print ("Station    Messages        Bits\n");
	    for (i = 0; i < NStations; i++) {
	        bcnt = 0L;
	        mcnt = 0;
	        mq = idToStation (i)->MQHead;

	        print (i, 7);
	        if (mq != NULL) {
	    	    for (m = mq [UTPattern->getId ()]; m != NULL; mcnt++,
			bcnt += m->Length, m = m->next);
	        }
	        print (mcnt, 12); print (bcnt, 12); print ("\n");
	    }
	    print ("\n");

#ifdef	SNAPSHOT_THROUGHPUT

	  exmode 11:

	    print ("UT Snapshot throughput:\n\n");
	    mcnt = STBIndex;
	    if (mcnt > STBUFSIZE) {
		mcnt = mcnt % STBUFSIZE;
		for (i = mcnt; i < STBUFSIZE; i++) 
		    print (STBuffer [i], " --> ", 15, 5);
	    }
	    for (i = 0; i < mcnt; i++)
	        print (STBuffer [i], " --> ", 15, 5);
	    print ("\n");
#endif
	}

	onscreen {

	  exmode 8:

	    // Local packet access time
	    {
		double min, max, mmnts [2]; long ns;
	        for (int i = 0; i < NStations; i++) {
		    s = (STATION_TYPE*)(idToStation (i));
		    s->UAPAcc->calculate (min, max, mmnts, ns);
		    display (i);
		    display (mmnts [0]);
		    display (min);
		    display (max);
		    display (ns);
	        }
	    }

	  exmode 9:

	    // Local message delay
	    {
		double min, max, mmnts [2]; long ns;
	        for (int i = 0; i < NStations; i++) {
		    s = (STATION_TYPE*)(idToStation (i));
		    s->UAMDel->calculate (min, max, mmnts, ns);
		    display (i);
		    display (mmnts [0]);
		    display (min);
		    display (max);
		    display (ns);
	        }
	    }

	  exmode 10:

	    // Pending messages
	    for (i = 0; i < NStations; i++) {
	        bcnt = 0L;
	        mcnt = 0;
	        mq = idToStation (i)->MQHead;

	        display (i);
	        if (mq != NULL) {
	    	    for (m = mq [UTPattern->getId ()]; m != NULL; mcnt++,
			bcnt += m->Length, m = m->next);
	        }
	        display (mcnt); display (bcnt);
	    }
#ifdef	SNAPSHOT_THROUGHPUT

	  exmode 11:

	    mcnt = STBIndex;
	    if (mcnt > STBUFSIZE) {
		mcnt = mcnt % STBUFSIZE;
		for (i = mcnt; i < STBUFSIZE; i++) display (STBuffer [i]);
	    }
	    for (i = 0; i < mcnt; i++) display (STBuffer [i]);
#endif
	}
}
#endif
