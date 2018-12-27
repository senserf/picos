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
/* Simple  bursty traffic. Messages of fixed length arrive in */
/* bursts of fixed size. The specified size of a burst  tells */
/* how  many messages arrive per station. All messages of one */
/* burst arrive at the same moment to all stations. The burst */
/* inter-arrival time is fixed. The receiver for each message */
/* is determined at  random,  from  among  all  the  stations */
/* except the sender.                                         */
/* ---------------------------------------------------------- */

#ifdef	FLUSH_TIME
static	short *tpi;	// An index array telling which Traffic belongs to
			// which station
#endif

void    initBSTraffic (double bit, double bsi, double mle) {

	int     i;
	Traffic *t;

	// bit - burst inter-arrival time (fixed)
	// bsi - burst size per station (fixed)
	// mle - message length (fixed)

	assert (NStations > 0, "initBSTraffic: stations must be created first");
	assert (NStations > 1, "initBSTraffic: at most two stations required");

#ifdef	FLUSH_TIME
	tpi = new short [NStations];
#endif
	for (i = 0; i < NStations; i++) {
		// Each station gets its private traffic pattern
		t = create BSTraffic (MIT_unf+MLE_unf+BIT_unf+BSI_unf,
			0.0, 0.0,       // Message inter-arrival time
			mle, mle,       // Message length
			bit, bit,       // Burst inter-arrival time
			bsi, bsi);      // Burst size
		t->addSender (i);
		t->addReceiver ();
#ifdef	FLUSH_TIME
		tpi [i] = t->getId ();
#endif
	}
		
#ifdef  LOCAL_MEASURES
	for (i = 0; i < NStations; i++) {
	    Node *s;
	    TheStation = s = (Node*) idToStation (i);
	    s->BSAMDel = create RVariable;
	    s->BSAPAcc = create RVariable;
#ifdef	FLUSH_TIME
	    s->BSFlushTime = TIME_0;
#endif
	}
#endif
}

#ifdef  LOCAL_MEASURES
void BSTraffic::pfmMRC (Packet *p) {

	double  d;

	d = Itu * (double) (Time - p->QTime);
	((Node*) idToStation (p->Sender)) -> BSAMDel -> update (d);
};
	
void BSTraffic::pfmPTR (Packet *p) {

	double  d;
	Node	*s;

	s = ((Node*) idToStation (p->Sender));
	d = Itu * (double) (Time - p->TTime);
	s -> BSAPAcc -> update (d);
#ifdef	FLUSH_TIME
	if (s->MQHead [tpi [s->getId ()]] == NULL) s->BSFlushTime = Time;
#endif
};

void    printBSPFM () {         // Print performance measures

	Node *s;
	int  i;

	print ("BS Local packet access time:\n\n");
	for (i = 0; i < NStations; i++) {
		s = (Node*)(idToStation (i));
		if (i == 0)
			s->BSAPAcc->printACnt ();
		else
			s->BSAPAcc->printSCnt ();
	}
	print ("\n\n");

	print ("BS Local message delay:\n\n");
	for (i = 0; i < NStations; i++) {
		s = (Node*)(idToStation (i));
		if (i == 0)
			s->BSAMDel->printACnt ();
		else
			s->BSAMDel->printSCnt ();
	}
	print ("\n\n");

#ifdef	FLUSH_TIME
	print ("BS Flush times:\n\n");
	for (i = 0; i < NStations; i++) {
		s = (Node*)(idToStation (i));
		print (s->BSFlushTime, form ("Station%03d     ", i));
	}
	print ("\n\n");
#endif
}
#endif
