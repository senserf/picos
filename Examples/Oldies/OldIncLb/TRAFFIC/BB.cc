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

/* ------------------------------------------ */
/* Bursty traffic against uniform background. */
/* ------------------------------------------ */

int        BstStation;
BUBTraffic *BstTraffic;
long	   RemainingBurstSize;

RVariable  *BStat;	// Burst statisctics
TIME	   BSTime;	// Starting time of a burst

#define	EndBurst 137	// End of burst signal

process BurstGen {

	TIME	BurstSpace;
	long	BurstSize;
	int	NBursts;

	states {Start, GenBurst, EBurst};

	void	setup (double bsz, double bsp, int nb) {
		BurstSize = (long) bsz;
		BurstSpace = (TIME) (bsp * Etu);
		NBursts = nb;
	};
};

BurstGen::perform {

  state Start:

	if (NBursts--) {
	  Timer->wait (BurstSpace, GenBurst);
        } else {
	  // Terminate simulation
	  Kernel->terminate ();
	}

  state GenBurst:

	BSTime = Time;
	for (int i = 0; i < BurstSize; i++) {
	  long s = BstTraffic->genSND ();
	  long r = BstTraffic->genRCV ();
	  BstTraffic->genMSG (s, r, BstTraffic->genMLE ());
	}
	RemainingBurstSize = BurstSize;
	Signal->wait (EndBurst, EBurst);

  state EBurst:

	BStat->update ((double)(Time - BSTime));
	proceed Start;
};

void    initBUTraffic (double uit, double ule, int bs, double bsi, double ble,
	int nbs, double bsp) {

	int     i;
	Traffic *t;

	// uit - uniform traffic message interarrival time (exponential)
	// ule - uniform traffic message length (exponential or fixed)
	// bs  - bursty station Id
	// bsi - burst size (fixed)
	// ble - bursty traffic message length (exponential or fixed)
	// nbs - number of bursts (simulation stops after that many bursts
	//	 have been processed)
	// bsp - the amount of time (in ETU) elapsing between the end of
	//       the previous burst and the beginning of a new one. This
	//	 time also elapses before the first burst is generated

	assert (NStations > 0, "initBUTraffic: stations must be created first");
	assert (NStations > 1, "initBUTraffic: at most two stations required");

	BstStation = bs;

#ifdef	FIXED_MESSAGE_LENGTH
	t = create BUUTraffic (MIT_exp+MLE_unf, uit, ule, ule);
#else
	t = create BUUTraffic (MIT_exp+MLE_exp, uit, ule);
#endif
	for (i = 0; i < NStations; i++)
		if (i != BstStation) t->addSender (i);
	t->addReceiver ();

#ifdef	FIXED_MESSAGE_LENGTH
	BstTraffic = create BUBTraffic (SCL_off+MLE_unf, ble, ble);
#else
	BstTraffic = create BUBTraffic (SCL_off+MLE_exp, ble);
#endif
	BstTraffic->addSender (BstStation);
	BstTraffic->addReceiver ();

	BStat = create RVariable;

	create (System) BurstGen (bsi, bsp, nbs);
		
#ifdef  LOCAL_MEASURES
	for (i = 0; i < NStations; i++) {
	    Node *s;
	    s = (Node*) idToStation (i);
	    s->BUUAMDel = create (s) RVariable;
	    s->BUUAPAcc = create (s) RVariable;
	    s->BUUWPAcc = create (s) RVariable;
	}
#endif
}

BUBTraffic::BUBTraffic () {

	BUBWPAcc = create (BstStation) RVariable;
};

void BUBTraffic::pfmPTR (Packet *p) {

	double  d;

	d = Itu * (double) (Time - p->TTime);
	BUBWPAcc -> update (d/p->ILength);
};

void BUBTraffic::pfmMTR (Packet*) {

	assert (RemainingBurstSize > 0, "bursty message not from burst");
	if (--RemainingBurstSize == 0)
		Signal->send (EndBurst, System);
};

#ifdef  LOCAL_MEASURES
void BUUTraffic::pfmMRC (Packet *p) {

	double  d;

	d = Itu * (double) (Time - p->QTime);
	((Node*) idToStation (p->Sender)) -> BUUAMDel -> update (d);
};
	
void BUUTraffic::pfmPTR (Packet *p) {

	double  d;
	Node    *s;

	s = ((Node*) idToStation (p->Sender));
	d = Itu * (double) (Time - p->TTime);
	s -> BUUAPAcc -> update (d);
	s -> BUUWPAcc -> update (d/p->ILength);
};
#endif

void    printBUPFM () {         // Print performance measures

#ifdef  LOCAL_MEASURES

	Node *s;
	int  i;

	print ("BUU Local packet access time:\n\n");
	for (i = 0; i < NStations; i++) {
		s = (Node*)(idToStation (i));
		if (i == 0)
			s->BUUAPAcc->printACnt ();
		else
			s->BUUAPAcc->printSCnt ();
	}
	print ("\n\n");

	print ("BUU Local weighted packet access time:\n\n");
	for (i = 0; i < NStations; i++) {
		s = (Node*)(idToStation (i));
		if (i == 0)
			s->BUUWPAcc->printACnt ();
		else
			s->BUUWPAcc->printSCnt ();
	}
	print ("\n\n");

	print ("BUU Local message delay:\n\n");
	for (i = 0; i < NStations; i++) {
		s = (Node*)(idToStation (i));
		if (i == 0)
			s->BUUAMDel->printACnt ();
		else
			s->BUUAMDel->printSCnt ();
	}
	print ("\n\n");
#endif
	BstTraffic->printPfm ("BUB Traffic performance measures");
	BstTraffic->BUBWPAcc->printCnt ("WPA - Weighted packet access time");

	BStat->printCnt ("BST - Burst dispersion time");
}
