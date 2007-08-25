/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-07   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

/* ---------------------- */
/* Station initialization */
/* ---------------------- */

#include        "system.h"

void    Station::zz_start () {

/* ------------------------------------------ */
/* The intermediate-level station constructor */
/* ------------------------------------------ */

	int             i;
	static  int     asize = 15;     // A small power of two - 1 (initial
					// size of zz_st)
	Station         **scratch;
	Mailbox         *m, *n;

#if	ZZ_NOL
	Port            *p, *q;
#endif

#if	ZZ_NOR
	Transceiver	*t, *u;
#endif

	Assert (!zz_flg_started,
	  "Station: cannot create new stations after the protocol has started");

	TheStation = this;

#if	ZZ_NOC
	CWList = NULL;
#endif

	ChList = NULL;
	Class  = OBJ_station;

	Id = sernum++;

	if (zz_st == NULL) {
		// The first time around -- create the array of stations
		zz_st = new Station* [asize];
	} else if (Id >= asize) {
		// The array must be enlarged
		scratch = new Station* [asize];
		for (i = 0; i < asize; i++)
			// Backup copy
			scratch [i] = zz_st [i];

		delete (zz_st);         // Deallocate previous array
		zz_st = new Station* [asize = (asize+1) * 2 - 1];
		while (i--) zz_st [i] = scratch [i];
		delete (scratch);
	}
	zz_st [Id] = this;
	NStations = sernum;

	Mailboxes = NULL;

#if	ZZ_NOC

	MQHead = MQTail = NULL;
#if     ZZ_QSL
	QSLimit = MAX_Long;
	QSize = 0;
#endif
	Buffers = zz_ncpframe;
	zz_ncpframe = NULL;

#endif	/* NOC */

#if	ZZ_NOL

	// Move statically-defined ports and transceivers to the station's list
	Ports = NULL;
	for (p = zz_ncport; p != NULL; p = q) {
		q = p->nextp;
		zz_ncport = p;
		p->zz_start (); // Execute the nonstandard constructor
	}

#endif	/* NOL */

#if	ZZ_NOR

	Transceivers = NULL;
	for (t = zz_nctrans; t != NULL; t = u) {
		u = t->nextp;
		zz_nctrans = t;
		t->zz_start (); // Execute the nonstandard constructor
	}

#endif	/* NOR */

	// Move statically-defined mailboxes to the station's list
	for (m = zz_ncmailbox; m != NULL; m = n) {
		n = m->nextm;
		zz_ncmailbox = m;
		m->zz_start (); // Execute the nonstandard constructor
	}

#if     ZZ_TOL
	// Clock tolerance parameters
	CQuality = zz_quality;
	CTolerance = zz_tolerance;
#endif

	// Add the station to the owner's list

	if (TheProcess != NULL)
		pool_in (this, TheProcess->ChList);
}

#if	ZZ_TOL

void    Station::setTolerance (double t, int q) {

	if ((CTolerance = t) == 0.0)
		CQuality = 0;
	else
		CQuality = q;
	Assert ((q >=0) && (q <= 10) && (t >= 0.0) && (t < 1.0),
		"Station->setTolerance: illegal clock tolerance parameters: "
			"%f, %1d", t, q);
}

double	Station::getTolerance (int *q) {

	if (q)
		*q = CQuality;
	return CTolerance;
}

#endif

#if	ZZ_NOC

void    Station::setQSLimit (Long lim) {
/* ---------------------------------------------------------- */
/* Sets  the  limit  on  the  number  of messages that can be */
/* queued at this station awaiting  transmission.  When  this */
/* limit is reached, new generated messages will be ignored.  */
/* ---------------------------------------------------------- */
#if     ZZ_QSL
	QSLimit = lim;
#else
	lim++;
	excptn ("Station->setQSLimit illegal: smurph not created with '-q'");
#endif
}

#endif	/* NOC */

void	ZZ_SYSTEM::makeTopology () {

#if	ZZ_NOL
	makeTopologyL ();
#endif
#if	ZZ_NOR
	makeTopologyR ();
#endif

}

sexposure (Station)

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr);         // Processes
#if	ZZ_NOC
		sexmode (1)

			exPrint1 (Hdr);         // Packet buffers
#endif
		sexmode (2)

			exPrint2 (Hdr);         // Mailboxes
#if	ZZ_NOL
		sexmode (3)

			exPrint3 (Hdr);         // Activities

		sexmode (4)

			exPrint4 (Hdr);         // Ports' status
#endif

#if	ZZ_NOR
		sexmode (5)

			exPrint5 (Hdr);		// RF Activities

		sexmode (6)

			exPrint6 (Hdr);		// Transceivers
#endif
	}

	sonscreen {

		sfxmode (0)

			exDisplay0 ();          // Processes
#if	ZZ_NOC
		sexmode (1)

			exDisplay1 ();          // Buffers
#endif
		sexmode (2)

			exDisplay2 ();          // Mailboxes
#if	ZZ_NOL
		sexmode (3)

			exDisplay3 ();          // Activities

		sexmode (4)

			exDisplay4 ();          // Ports' status
#endif

#if	ZZ_NOR
		sexmode (5)

			exDisplay5 ();		// RF Activities

		sexmode (6)

			exDisplay6 ();
#endif
	}
	USESID;
}

static  Station    *tstation;      // The current station for pttrav
static  ZZ_REQUEST *trq;           // Made global to save some stack space
static  ZZ_EVENT   *tev;
static  int     tii;
static  Long    tjj;

void Station::pttrav (ZZ_Object *pr) {

/* --------------------- */
/* A helper for exPrint0 */
/* --------------------- */

	for ( ; (pr != NULL) && (pr->Class != AIC_process ||
		((Process*)pr)->Owner != tstation); pr = pr->next);

	if (pr == NULL) return;

	for (tev = zz_eq; tev != zz_sentinel_event; tev = tev -> next)
		if (tev->process == pr) break;

	if (tev != zz_sentinel_event && (trq = tev->chain) != NULL) {

		for (tii = 0; ; tii++) {

			if (tii == 0)
				// First time for this process
				print (pr->getOName (), 20);
			else
				Ouf << "                    ";
			Ouf << ' ';
			print (trq->ai->getTName (), 10);
			if ((tjj = trq->ai->zz_aid ()) != NONE)
				Ouf << form ("/%03d ", zz_trunc (tjj, 3));
			else
				Ouf << "     ";

			print (trq->ai->zz_eid (trq->event_id), 10);
			Ouf << ' ';
			print (((Process*)pr)->zz_sn (trq->pstate), 11);

			Ouf << ' ';
			if (undef (trq->when)) {
				print ("undefined", 15);
				Ouf << ' ';
			} else {
				ptime (trq->when, 15);
				if (pless (tev->waketime, trq->when))
					Ouf << ' ';
				else if (trq == tev->chain)
					Ouf << '*';
				else
					Ouf << '?';
			}
			Ouf << '\n';

			if ((trq = trq->other) == tev->chain) break;
		}
	}

	pttrav (pr->next);
	pttrav (((Process*)pr)->ChList);
}

void     Station::exPrint0 (const char *hdr) {

/* --------------------------------- */
/* Print information about processes */
/* --------------------------------- */

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Sleeping processes:\n\n";
	}

	Ouf << "             Process         AI/Idn      Event       State"
		<< "            Time\n";

	tstation = this;
	if (zz_flg_started)
		pttrav (ChList);
	else
		// Ownership not adjusted ?
		pttrav (Kernel->ChList);

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

#if	ZZ_NOC

void     Station::exPrint1 (const char *hdr) {

/* -------------------------------------- */
/* Print information about packet buffers */
/* -------------------------------------- */

	int                             i;
	Packet                          *p;
	ZZ_PFItem                       *pfi;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Packet buffers:\n\n";
	}

	Ouf << "Buf           QTime           TTime Rcvr   TP   ILength" <<
#if ZZ_DBG
		"   TLength Fg  Signature\n";
#else
		"   TLength Fg\n";
#endif

	for (i = 0, pfi = Buffers; pfi != NULL; pfi = pfi->next, i++) {

		p = pfi->buf;
		print (zz_trunc (i,3), 3);
		Ouf << ' ';
		if (flagCleared (p->Flags, PF_full)) {
			Ouf << "empty --------- ---------------- ---- ----" <<
#if ZZ_DBG
				" --------- --------- -- ----------\n";
#else
				" --------- --------- --\n";
#endif
			continue;
		}
		print (p->QTime, 15); Ouf << ' ';
		print (p->TTime, 15); Ouf << ' ';
		if (isStationId (p->Receiver))
			print (zz_trunc (p->Receiver, 4), 4);
		else if (p->Receiver == NONE)
			print ("none", 4);
		else
			print ("bcst", 4);
		Ouf << ' ';
		print (zz_trunc (p->TP, 4), 4);
		Ouf << ' ';
		print (zz_trunc (p->ILength, 9), 9);
		Ouf << ' ';
		print (zz_trunc (p->TLength, 9), 9);
		Ouf << ' ';
		print (p->zz_pflags (), 2);
#if ZZ_DBG
		Ouf << ' ';
		print (zz_trunc (p->Signature, 10), 10);
#endif
		Ouf << '\n';
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

#endif	/* NOC */

void     Station::exPrint2 (const char *hdr) {

/* ---------------------------------------- */
/* Print information about mailbox contents */
/* ---------------------------------------- */

	Mailbox *m;
	char    *mh;
	int     first;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Mailbox contents:\n\n";
	}

	for (first = 0, m = Mailboxes; m != NULL; m = m->nextm) {
		if (m->zz_nickname != NULL)
			mh = m->zz_nickname;
		else
			mh = form ("%16d", GYID (m->Id));
		m->printOut (2 + first, mh);
		first = 1;
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

#if	ZZ_NOL

void     Station::exPrint3 (const char *hdr) {

/* --------------------------------------- */
/* Print information about link activities */
/* --------------------------------------- */

	int                             l;
	ZZ_LINK_ACTIVITY                *a, *cl;
	Port                            *pp;
	Link                            *lk;

	if (hdr != NULL) {
		Ouf << hdr;
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Activities in links";
		Ouf << ':';
	}

	Ouf << "\n\n";

	Ouf << "Port Link           STime           FTime T  Rcvr   TP"
#if ZZ_DBG
		<< "     Length  Signature";
#else
		<< "     Length";
#endif

	for (l = 0, pp = Ports; pp != NULL; pp = pp->nextp, l++) {
		lk = pp->Lnk;
		for (cl = a = lk->Archived; ; a = a -> next) {
			if (a == NULL) {
				if (cl == lk->Alive) break;
				if ((cl = a = lk->Alive) == NULL) break;
			}
			if (a->GPort != pp) continue;

			print (zz_trunc (l, 4), 4);
			print (zz_trunc (lk->Id, 4), 5);
			Ouf << ' ';
			print (a->STime, 15);
			Ouf << ' ';
			if (def (a->FTime))
				print (a->FTime, 15);
			else
				print ("undefined", 15);
			Ouf << ' ';
			if (a->Type != JAM)
				Ouf << 'T';
			else
				Ouf << 'J';
			if (cl == lk->Archived)
				Ouf << '*';
			else
				Ouf << ' ';
			Ouf << ' ';
			if (a->Type != JAM) {
				if (isStationId (a->Pkt.Receiver))
					print (zz_trunc (a->Pkt.Receiver,
						4), 4);
				else if (a->Pkt.Receiver == NONE)
					print ("none", 4);
				else
					print ("bcst", 4);
				print (zz_trunc (a->Pkt.TP, 4), 5);
				print (zz_trunc (a->Pkt.TLength, 10), 11);
#if ZZ_DBG
				print (zz_trunc (a->Pkt.Signature, 10), 11);
#endif
			} else
#if ZZ_DBG
				Ouf << "---- ---- ---------- ----------";
#else
				Ouf << "---- ---- ----------";
#endif

			Ouf << '\n';
		}
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void     Station::exPrint4 (const char *hdr) {

/* ----------------------------------------------------- */
/* Print information about the status of station's ports */
/* ----------------------------------------------------- */

	Port    *p;

	if (hdr != NULL) {
		Ouf << hdr;
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Port status";
		Ouf << ':';
	}

	Ouf << "\n\n";

	Ouf << "            Port  ACT BOT EOT BMP EMP BOJ EOJ COL ANY\n";

	for (p = Ports; p != NULL; p = p -> nextp) {
		if (p->zz_nickname != NULL)
			print (p->zz_nickname, 16);
		else
			print (GYID (p->Id), 16);

		if (p->busy ())
			print ("***", 4); else print ("...", 4);
		if (p->botTime () == Time)
			print ("***", 4); else print ("...", 4);
		if (p->eotTime () == Time)
			print ("***", 4); else print ("...", 4);
		if (p->bmpTime () == Time)
			print ("***", 4); else print ("...", 4);
		if (p->empTime () == Time)
			print ("***", 4); else print ("...", 4);
		if (p->bojTime () == Time)
			print ("***", 4); else print ("...", 4);
		if (p->eojTime () == Time)
			print ("***", 4); else print ("...", 4);
		if (p->collisionTime (YES) == Time)
			print ("***", 4); else print ("...", 4);
		if (p->aevTime () == Time)
			print ("***", 4); else print ("...", 4);
		Ouf << '\n';
	}
	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

#endif	/* NOL */

#if	ZZ_NOR

void    Station::exPrint5 (const char *hdr) {

/* ------------------------------------- */
/* Print information about RF activities */
/* ------------------------------------- */

	if (hdr != NULL) {
		Ouf << hdr;
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Activities in RFChannels";
		Ouf << ':';
	}

	Ouf << "\n\n";

	RFChannel::exPrtRfa (Id, NULL);

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Station::exPrint6 (const char *hdr) {

/* ------------------------------------------------------------ */
/* Print information about the status of station's transceivers */
/* ------------------------------------------------------------ */

	Transceiver *p;
	int i, elist [11];

	if (hdr != NULL) {
		Ouf << hdr;
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Transceiver status";
		Ouf << ':';
	}

	Ouf << "\n\n";

	Ouf << "Tcv          R B O NAC BOP BOT EOT BMP EMP ANY PRE PAC KIL\n";

	for (p = Transceivers; p != NULL; p = p -> nextp) {
		if (p->zz_nickname != NULL)
			print (p->zz_nickname, 12);
		else
			print (GYID (p->Id), 12);
		Ouf << ' ';

		if (p->RxOn)
			Ouf << '+';
		else
			Ouf << '-';

		Ouf << ' ';

		if (p->busy ())
			Ouf << 'Y';
		else
			Ouf << 'N';

		Ouf << ' ';

		p->dspEvnt (elist);

		if (elist [10]) {
			if (elist [10] > 1)
				Ouf << 'T';
			else
				Ouf << 'P';
		} else {
			Ouf << ' ';
		}
				
		for (i = 0; i < 10; i++)
			print (zz_trunc (elist [i], 3), 4);

		Ouf << '\n';
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

#endif	/* NOR */

void Station::dttrav (ZZ_Object *pr) {

/* ----------------------- */
/* A helper for exDisplay0 */
/* ----------------------- */

	for ( ; (pr != NULL) && (pr->Class != AIC_process ||
		((Process*)pr)->Owner != tstation); pr = pr->next);

	if (pr == NULL) return;

	for (tev = zz_eq; tev != zz_sentinel_event; tev = tev -> next)
		if (tev->process == pr) break;
	if (tev != zz_sentinel_event && (trq = tev->chain) != NULL) {

		for (tii = 0; ; tii++) {

			if (tii == 0)
				// First time for this process
				display (pr->getOName ());
			else
				display (' ');

			display (trq->ai->getTName ());
			if ((tjj = trq->ai->zz_aid ()) != NONE)
				display (tjj);
			else
				display (' ');

			display (trq->ai->zz_eid (trq->event_id));
			display (((Process*)pr)->zz_sn (trq->pstate));
			if (undef (trq->when)) {
				display ("undefined");
				display (' ');
			} else {
				dtime (trq->when);
				if (pless (tev->waketime, trq->when))
					display (' ');
				else if (trq == tev->chain)
					display ('*');
				else
					display ('?');
			}

			if ((trq = trq->other) == tev->chain) break;
		}
	}

	dttrav (pr->next);
	dttrav (((Process*)pr)->ChList);
}

void     Station::exDisplay0 () {

/* ---------------------------------------------------------- */
/* Display  information  about  processes  belonging  to this */
/* station                                                    */
/* ---------------------------------------------------------- */

	tstation = this;
	if (zz_flg_started)
		dttrav (ChList);
	else
		// Ownership not adjusted ?
		dttrav (Kernel->ChList);
}

#if	ZZ_NOC

void     Station::exDisplay1 () {

/* ---------------------------------------- */
/* Display information about packet buffers */
/* ---------------------------------------- */

	int                             i, j;
	Packet                          *p;
	ZZ_PFItem                       *pfi;

	for (i = 0, pfi = Buffers; pfi != NULL; pfi = pfi->next, i++) {

		p = pfi->buf;
		display (i);            // Buffer number

		if (flagCleared (p->Flags, PF_full)) {
			display ("EMPTY");
			for (j = 0; j < 7; j++) display ("--");
			continue;
		}
		display (p->QTime);
		display (p->TTime);

		if (isStationId (p->Receiver))
			display (p->Receiver);
		else if (p->Receiver == NONE)
			display ("none");
		else
			display ("bcst");

		display (p->TP);
		display (p->ILength);
		display (p->TLength);
		display (p->zz_pflags ());
#if ZZ_DBG
		display (p->Signature);
#else
		display ("---");
#endif
	}
}

#endif	/* NOC */

void     Station::exDisplay2 () {

/* ------------------------------------------ */
/* Display information about mailbox contents */
/* ------------------------------------------ */

	Mailbox *m;

	for (m = Mailboxes; m != NULL; m = m->nextm)  {
		if (m->zz_nickname != NULL)
			display (m->zz_nickname);
		else
			display (GYID (m->Id));
		m->displayOut (2);
	}
}

#if	ZZ_NOL

void     Station::exDisplay3 () {

/* ----------------------------------------- */
/* Display information about link activities */
/* ----------------------------------------- */

	int                             l;
	ZZ_LINK_ACTIVITY                *a, *cl;
	Port                            *pp;
	Link                            *lk;

	for (l = 0, pp = Ports; pp != NULL; pp = pp->nextp, l++) {
		lk = pp->Lnk;
		for (cl = a = lk->Archived; ; a = a -> next) {
			if (a == NULL) {
				if (cl == lk->Alive) break;
				if ((cl = a = lk->Alive) == NULL) break;
			}
			if (a->GPort != pp) continue;

			display (l);            // Port number
			display (lk->Id);       // Link number

			display (a->STime);

			if (def (a->FTime))
				display (a->FTime);
			else
				display ("undefined");

			if (a->Type != JAM)
				display ('T');
			else
				display ('J');

			if (cl == lk->Archived)
				display ('*');
			else
				display (' ');

			if (a->Type != JAM) {
				if (isStationId (a->Pkt.Receiver))
					display (a->Pkt.Receiver);
				else if (a->Pkt.Receiver == NONE)
					display ("none");
				else
					display ("bcst");

				display (a->Pkt.TP);
				display (a->Pkt.TLength);
#if ZZ_DBG
				display (a->Pkt.Signature);
#else
				display ("---");
#endif
			} else {
				display ("---");
				display ("---");
				display ("---");
				display ("---");
			}
		}
	}
}

void     Station::exDisplay4 () {

/* -------------------------------------------------------- */
/* Displays information about the status of station's ports */
/* -------------------------------------------------------- */

	Port *p;

	for (p = Ports; p != NULL; p = p -> nextp) {
		if (p->zz_nickname != NULL)
			display (p->zz_nickname);
		else
			display (GYID (p->Id));

		if (p->busy ())
			display ("*"); else display (".");
		if (p->botTime () == Time)
			display ("*"); else display (".");
		if (p->eotTime () == Time)
			display ("*"); else display (".");
		if (p->bmpTime () == Time)
			display ("*"); else display (".");
		if (p->empTime () == Time)
			display ("*"); else display (".");
		if (p->bojTime () == Time)
			display ("*"); else display (".");
		if (p->eojTime () == Time)
			display ("*"); else display (".");
		if (p->collisionTime (YES) == Time)
			display ("*"); else display (".");
		if (p->aevTime () == Time)
			display ("*"); else display (".");
	}
}

#endif	/* NOL */

#if	ZZ_NOR

void	Station::exDisplay5 () {

/* --------------------------------------- */
/* Display information about RF activities */
/* --------------------------------------- */

	RFChannel::exDspRfa (Id, NULL);
}

void	Station::exDisplay6 () {

/* -------------------------------------------------------------- */
/* Display information about the status of station's transceivers */
/* -------------------------------------------------------------- */

	Transceiver *p;
	int i, elist [11];

	for (p = Transceivers; p != NULL; p = p -> nextp) {

		if (p->zz_nickname != NULL)
			display (p->zz_nickname);
		else
			display (GYID (p->Id));

		if (p->RxOn)
			display ('+');
		else
			display ('-');

		if (p->busy ())
			display ('Y');
		else
			display ('N');

		p->dspEvnt (elist);

		if (elist [10]) {
			if (elist [10] > 1)
				display ('T');
			else
				display ('P');
		} else {
			display (' ');
		}

		for (i = 0; i < 10; i++)
			display (elist [i]);

	}
}

#endif	/* NOR */

sexposure (ZZ_SYSTEM)

/* ---------------------------------------------------------- */
/* SYSTEM  station  exposure is used to print out information */
/* about network topology                                     */
/* ---------------------------------------------------------- */

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr);         // Full topology

		sexmode (1)

			exPrint1 (Hdr);         // Abbreviated topology

		sexmode (2)

			exPrint2 (Hdr);         // Mailbox contents
	}

	sonscreen {

		sfxmode (2)

			exDisplay2 ();          // Mailboxes
	}
	USESID;
}

void    ZZ_SYSTEM::exPrint0 (const char *hdr) {

/* --------------------------------------------- */
/* Print full information about network topology */
/* --------------------------------------------- */

	int                             i;
	Station                         *s;
#if	ZZ_NOL || ZZ_NOR
	int				j;
#endif
#if	ZZ_NOL
	int				l, k;
	Link                            *lk;
	Port                            *p;
#endif
#if	ZZ_NOR
	RFChannel			*rc;
	Transceiver			*t;
	double				lx, ly, hx, hy, diam;
#if	ZZ_R3D
	double				lz, hz;
#endif
#endif
#if	ZZ_NOC
	ZZ_PFItem                       *pfi;
	int				m;
#endif
	buildNetwork ();

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << "(System) Full network configuration:\n\n";
	}

	Ouf << "Stations:\n\n";
	Ouf << "  NumId             Type";
#if	ZZ_NOL
        Ouf << "  NPorts";
#endif
#if	ZZ_NOR
	Ouf << "  NTrans";
#endif
#if	ZZ_NOC
        Ouf << "  NBuffs";
#endif
        Ouf << "            NName\n";

	for (i = 0; i < NStations; i++) {
		print (i, 7); Ouf << ' ';
		s = idToStation (i);
		print (s->getTName (), 16);
#if	ZZ_NOL
		for (j = 0, p = s->Ports; p != NULL; p = p->nextp, j++);
		print (j, 8);
#endif
#if	ZZ_NOR
		for (j = 0, t = s->Transceivers; t != NULL; t = t->nextp, j++);
		print (j, 8);
#endif
#if	ZZ_NOC
		for (m = 0, pfi = s->Buffers; pfi != NULL; pfi = pfi->next,m++);
		print (m, 8);
#endif
		Ouf << ' ';
		if (s->getNName () == NULL)
			print ("-----", 16);
		else
			print (s->getNName (), 16);
		Ouf << '\n';
	}

#if	ZZ_NOL
	Ouf << "\nLinks:\n\n";

	for (i = 0; i < NLinks; i++) {

		lk = idToLink (i);
		Ouf << form ("  Link %4d,    Type: %s\n\n", i, lk->getTName ());
		Ouf << "    Distance matrix:\n\n";

		for (j = 0; j < lk->NPorts; j++) {      // Go through the rows
			for (k = 0; k < NStations; k++) {
				s = idToStation (k);
				for (p = s->Ports; p != NULL; p = p->nextp)
					if (p->Lnk == lk &&
						p->LRId == j) goto PFND;
			}
		excptn ("System->exposure: internal error -- port not found");
PFND:
			Ouf << form (" %5d: ", zz_trunc (j, 5));
			for (k = 0; k < lk->NPorts; k++) {
				if (k && k % 8 == 0)
					Ouf << "\n   ...: ";
				if (k == j)
					Ouf << "     ---";
				else if (k < j && lk->Type >= LT_unidirectional)
					Ouf << "     ***";
				else
					print (ituToDu (p->DV [k]), 8);

				if ((k + 1) % 8 != 0 && (k + 1) < lk->NPorts)
					Ouf << " ";
			};
			Ouf << '\n';
			if ((j+1) < lk->NPorts && lk->NPorts > 8) Ouf << '\n';
		}
		Ouf << "\n    Ports:\n\n";
		Ouf << "      LRId    Station    SRId        Rate\n";

		for (j = 0; j < lk->NPorts; j++) {
			for (k = 0; k < NStations; k++) {
				s = idToStation (k);
				for (l = 0, p = s->Ports; p != NULL;
				    p = p->nextp, l++)
					if (p->Lnk == lk &&
						p->LRId == j) goto PFNE;
			}
		excptn ("System->exposure: internal error -- port not found");
PFNE:
			Ouf << form ("     %5d %10d %7d ", zz_trunc (j, 5),
				k, l);
			print (p->TRate, 11);
			Ouf << '\n';
		}
		Ouf << '\n';
	}
#endif	/* NOL */

#if	ZZ_NOR
	Ouf << "\nRFChannels\n\n";

	for (i = 0; i < NRFChannels; i++) {

		rc = idToRFChannel (i);
#if	ZZ_R3D
		diam = rc->getRange (lx, ly, lz, hx, hy, hz);
#else
		diam = rc->getRange (lx, ly, hx, hy);
#endif
		Ouf << form ("  RFChannel %4d, NTransceivers %4d [<",
			i, rc->NTransceivers);
		Ouf << lx << ", " << ly <<
#if	ZZ_R3D
			", " << lz <<
#endif
			">, <" << hx << ", " << hy <<
#if	ZZ_R3D
			", " << hz <<
#endif
			">] Diam " << diam << "\n\n";

		rc->exPrtTop (YES);
	}
#endif
	Ouf << "(System) End of list\n\n";
}

void    ZZ_SYSTEM::exPrint1 (const char *hdr) {

/* ---------------------------------------------------- */
/* Print abbreviated information about network topology */
/* ---------------------------------------------------- */

	int                             i;
	Station                         *s;
#if	ZZ_NOL || ZZ_NOR
	int				j;
#endif
#if	ZZ_NOL
	int				l, k;
	Link                            *lk;
	Port                            *p;
	DISTANCE                        d;
#endif
#if	ZZ_NOC
	int				m;
	ZZ_PFItem                       *pfi;
#endif
#if	ZZ_NOR
	RFChannel			*rc;
	Transceiver			*t;
	double				lx, ly, hx, hy, diam;
#if	ZZ_R3D
	double				lz, hz;
#endif
#endif

	buildNetwork ();
	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << "(System) Abbreviated network configuration:\n\n";
	}

	Ouf << "Stations:\n\n";
	Ouf << "  Numid             Type";
#if	ZZ_NOL
	Ouf << "  NPorts";
#endif
#if	ZZ_NOR
	Ouf << "  NTrans";
#endif
#if	ZZ_NOC
	Ouf << "  NBuffs";
#endif
	Ouf << "            NName\n";

	for (i = 0; i < NStations; i++) {
		print (i, 7); Ouf << ' ';
		s = idToStation (i);
		print (s->getTName (), 16);
#if	ZZ_NOL
		for (j = 0, p = s->Ports; p != NULL; p = p->nextp, j++);
		print (j, 8);
#endif
#if	ZZ_NOR
		for (j = 0, t = s->Transceivers; t != NULL; t = t->nextp, j++);
		print (j, 8);
#endif
#if	ZZ_NOC
		for (m = 0, pfi = s->Buffers; pfi != NULL; pfi = pfi->next,m++);
		print (m, 8);
#endif
                Ouf << ' ';
		if (s->getNName () == NULL)
			print ("-----", 16);
		else
			print (s->getNName (), 16);
		Ouf << '\n';
	}

#if	ZZ_NOL
	Ouf << "\nLinks:\n\n";

	for (i = 0; i < NLinks; i++) {

		lk = idToLink (i);

		// Determine the length of the link
		for (d = DISTANCE_0, k = 0; k < NStations; k++) {
			s = idToStation (k);
			for (p = s->Ports; p != NULL; p = p->nextp)
				if (p->Lnk == lk &&
					p->MaxDistance > d) d = p->MaxDistance;
		}

		Ouf << form ("  Link %4d,    Type: %s,    Length: ", i,
			lk->getTName ());
		print (ituToDu (d), 11);
		Ouf << "\n\n";

		Ouf << "    Ports:\n\n";
		Ouf << "      LRId    Station    SRId        Rate\n";

		for (j = 0; j < lk->NPorts; j++) {
			for (k = 0; k < NStations; k++) {
				s = idToStation (k);
				for (p = s->Ports, l = 0; p != NULL;
				    p = p->nextp, l++)
					if (p->Lnk == lk &&
						p->LRId == j) goto PFNF;
			}
		excptn ("System->exposure: internal error -- port not found");
PFNF:
			Ouf << form ("     %5d %10d %7d ", zz_trunc (j, 5),
				k, l);
			print (p->TRate, 11);
			Ouf << '\n';
		}
		Ouf << '\n';
	}
#endif	/* NOL */

#if	ZZ_NOR
	Ouf << "\nRFChannels\n\n";

	for (i = 0; i < NRFChannels; i++) {

		rc = idToRFChannel (i);
#if	ZZ_R3D
		diam = rc->getRange (lx, ly, lz, hx, hy, hz);
#else
		diam = rc->getRange (lx, ly, hx, hy);
#endif
		Ouf << form ("  RFChannel %4d, NTransceivers %4d [<",
			i, rc->NTransceivers);
		Ouf << lx << ", " << ly <<
#if	ZZ_R3D
			", " << lz <<
#endif
			">, <" << hx << ", " << hy <<
#if	ZZ_R3D
			", " << hz <<
#endif
			">] Diam " << diam << "\n\n";

		rc->exPrtTop (NO);

		Ouf << '\n';
	}
#endif
	Ouf << "(System) End of list\n\n";
}
