/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-08   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

/* -------------- */
/* The Monitor AI */
/* -------------- */

#include        "system.h"

zz_monitor::zz_monitor () {

/* --------------- */
/* The constructor */
/* --------------- */
	int i;

	Class = AIC_monitor;
	Id = NONE;

	for (i = 0; i < ZZ_MONITOR_HASH_SIZE; i++)
	        WList [i] = NULL;
}

#if  ZZ_TAG
void    zz_monitor::wait (void *ev, int pstate, LONG tag) {
#else
void    zz_monitor::wait (void *ev, int pstate) {
#endif

/* ------------------------ */
/* The Monitor wait request */
/* ------------------------ */

	if_from_observer ("Monitor->wait: called from an observer");

	if (zz_c_first_wait) {

		zz_c_other = NULL;    // No requests so far

		// Create new simulation event
		zz_c_wait_event = new ZZ_EVENT;
		zz_c_wait_event -> station = TheStation;
		zz_c_wait_event -> process = TheProcess;
	}

	// Info01 is TheSignal
	new ZZ_REQUEST (&(WList [hash (ev)]), this, (int) ev, pstate, NONE, ev);

	assert (zz_c_other != NULL,
		"Monitor->wait: internal error -- null request");

	if (zz_c_first_wait) {
		zz_c_wait_event -> pstate = pstate;
		zz_c_wait_event -> ai = this;
		zz_c_wait_event -> event_id = (int) ev;
#if  ZZ_TAG
		zz_c_wait_tmin . set (TIME_inf, tag);
		zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
		zz_c_wait_event -> waketime = zz_c_wait_tmin = TIME_inf;
#endif
		zz_c_wait_event -> chain = zz_c_other;
		zz_c_wait_event -> Info01 = ev;
		zz_c_wait_event -> Info02 = NULL;
		zz_c_whead = zz_c_other;
		zz_c_first_wait = NO;
		zz_c_wait_event->store ();
	}

#if     ZZ_TAG
	zz_c_other -> when . set (TIME_inf, tag);
#else
	zz_c_other -> when = TIME_inf;
#endif
	zz_c_other -> event = zz_c_wait_event;
	zz_c_other -> other = zz_c_whead;
}

int zz_monitor::signal (void *ev) {

	ZZ_REQUEST *rq;
	ZZ_EVENT *e;
	int na;
#if ZZ_TAG
	int q;
#endif

	for (na = 0, rq = WList [hash (ev)]; rq != NULL; rq = rq->next) {
		if (rq->event_id == (int) ev) {
			na++;
			// TheSender
			rq->Info02 = (void*) TheProcess;
#if ZZ_TAG
			rq->when . set (Time);
			if ((q = (e = rq->event)->waketime . cmp (rq->when)) >
			  0 || (q == 0 && FLIP))
#else
			rq->when = Time;
			if ((e = rq->event)->waketime > Time || FLIP)
#endif
				e->new_top_request (rq);
		}
	}

	return na;
}

int zz_monitor::signalP (void *ev) {

	ZZ_REQUEST *rq, *rs;

	rs = NULL;

	// The other party must be waiting
	for (rq = WList [hash (ev)]; rq != NULL; rq = rq -> next) {
	    if (rq -> event_id == (int) ev) {
		Assert (rs == NULL,
		    "Monitor->signalP: more than one wait request");
		rs = rq;
		rq->Info02 = (void*) TheProcess;
	    }
	}
		
	if (rs == NULL) return (REJECTED);

#if  ZZ_TAG
	rs -> when . set (Time);
#else
	rs -> when = Time;
#endif
	assert (zz_pe == NULL,
		"Monitor->signalP: more than one pending priority event");

	zz_pe = (zz_pr = rs) -> event;

	return (ACCEPTED);
};

sexposure (zz_monitor)

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr, (int) SId);

		sexmode (1)

			exPrint1 (Hdr, (int) SId);
	}

	sonscreen {

		sfxmode (0)

			exDisplay0 ((int) SId);

		sexmode (1)

			exDisplay1 ((int) SId);
	}
}

void     zz_monitor::exPrint0 (const char *hdr, int sid) {

	ZZ_REQUEST      *r;
	ZZ_EVENT        *e;
	Long            l;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Full AI wait list";
		if (isStationId (sid))
			Ouf << " for " << idToStation (sid) -> getOName ();
		Ouf << ":\n\n";
	}

	if (!isStationId (sid))
		Ouf << " When   St";
	else
		Ouf << " When ";

	Ouf << "    Process/Idn     MState         AI/Idn" <<
		"      Event      State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && (e->station == NULL ||
			e->chain == NULL || ident (e->station) != sid))
				continue;

		if ((r = e->chain) == NULL) {
			// System event
			if (zz_flg_nosysdisp || e->ai->Class != AIC_monitor)
				continue;
			print (def (e->waketime) ? "NOW" : "PND", 5);
			Ouf << "* Sys ";
			print (e->process->getTName (), 10);
			Ouf << form ("/%03d ", zz_trunc (e->process->Id, 3));
			// State name
			print (e->process->zz_sn (e->pstate), 10);

			Ouf << ' ';
			print (e->ai->getTName (), 10);
			if (e->ai->Id == NONE)
				Ouf << "     ";
			else
				Ouf << form ("/%03d ", zz_trunc (e->ai->Id, 3));

			print (e->ai->zz_eid (e->event_id), 10);

			Ouf << ' ';
			print (e->process->zz_sn (e->pstate), 10);

			Ouf << '\n';
		} else {
			// A station event

			while (1) {

				if (r->ai->Class == AIC_monitor) {
					print (def (r->when) ? "NOW" : "PND",
					    5);

					if (pless (e->waketime, r->when))
						Ouf << ' ';     // Obsolete
					else if (e->chain == r)
						Ouf << '*';     // Current
					else
						Ouf << '?';

					if (!isStationId (sid)) {
						// Identify the station
						if (e->station != NULL &&
							ident (e->station) >= 0)
							Ouf << form ("%4d ",
					zz_trunc (ident (e->station), 3));
						else
							Ouf << " Sys ";
					}
					print (e->process->getTName (), 10);
					Ouf << form ("/%03d ",
						zz_trunc (e->process->Id, 3));
					print (e->process->zz_sn (r->pstate),
						10);
					Ouf << ' ';

					print (e->ai->getTName (), 10);
					if ((l = e->ai->zz_aid ()) != NONE)
						Ouf << form ("/%03d ",
							zz_trunc (l, 3));
					else
						Ouf << "     ";

					print (e->ai->zz_eid (e->event_id),
						10);
					Ouf << ' ';

					print (e->process->zz_sn
							(e->chain->pstate), 10);
					Ouf << '\n';
				}

				if ((r = r->other) == e->chain) break;
			}
		}
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    zz_monitor::exPrint1 (const char *hdr, int sid) {

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () <<") Abbreviated AI wait list";
		if (isStationId (sid))
			Ouf << " for " << idToStation (sid)->getOName ();
		Ouf << ":\n\n";
	}

	if (!isStationId (sid))
		Ouf << " When     St";
	else
		Ouf << " When ";
	Ouf << "      Process/Ident        State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if ((r = e->chain) == NULL || e->station == NULL) continue;
		if (isStationId (sid) && ident (e->station) != sid)
				continue;

		while (1) {

			if (r->ai->Class == AIC_monitor) {

				print (def (r->when) ? "NOW" : "PND", 5);

				if (pless (e->waketime, r->when))
					Ouf << ' ';     // Obsolete
				else if (e->chain == r)
					Ouf << '*';     // Active
				else
					Ouf << '?';

				if (!isStationId (sid)) {
					// Identify the station
					if (e->station != NULL &&
						ident (e->station) >= 0)
						Ouf << form ("%6d ",
					zz_trunc (ident (e->station), 5));
					else
						Ouf << "   Sys ";
				}
				print (e->process->getTName (), 10);
				Ouf << form ("/%05d ",
					zz_trunc (e->process->Id, 5));
				print (e->process->zz_sn (r->pstate), 10);
				Ouf << '\n';
			}

			if ((r = r->other) == e->chain) break;
		}
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void     zz_monitor::exDisplay0 (int sid) {

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;
	Long               l;

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && (e->station == NULL ||
			e->chain == NULL || ident (e->station) != sid))
				continue;

		if ((r = e->chain) == NULL) {

			// System event

			if (zz_flg_nosysdisp || e->ai->Class != AIC_monitor)
				continue;
			display (def (e->waketime) ? "NOW" : "PND");
			display ('*');
			if (!isStationId (sid))
				display ("Sys");  // Note that sid must be NONE
			display (e->process->getTName ());
			display (e->process->Id);
			// State name
			display (e->process->zz_sn (e->pstate));
			display (e->ai->getTName());
			display (e->ai->Id);
			display (e->ai->zz_eid (e->event_id));
			display (e->process->zz_sn (e->pstate));

		} else {
			// A station event

			while (1) {
				if (r->ai->Class == AIC_monitor) {
					display (def (r->when) ? "now" : "pnd");
					if (pless (e->waketime, r->when))
						display (' ');  // Obsolete
					else if (e->chain == r)
						display ('*');  // Current
					else
						display ('?');  // Unknown

					if (!isStationId (sid)) {
						// Identify the station
						if (e->station != NULL &&
							ident (e->station) >= 0)
						   display (ident (e->station));
						else
						   display ("Sys");
					}

					display (e->process->getTName ());
					display (e->process->Id);
					display (e->process->zz_sn (r->pstate));
					display (e->ai->getTName ());
					if ((l = e->ai->zz_aid ()) != NONE)
						display (l);
					else
						display (' ');

					display (e->ai->zz_eid (e->event_id));
					display (
					  e->process->zz_sn (e->chain->pstate));
				}
				if ((r = r->other) == e->chain) break;
			}
		}
	}
}

void    zz_monitor::exDisplay1 (int sid) {

/* ---------------------------------------------- */
/* Display the abbreviated list of Timer requests */
/* ---------------------------------------------- */

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if ((r = e->chain) == NULL || e->station == NULL) continue;
		if (isStationId (sid) && ident (e->station) != sid)
				continue;

		while (1) {

			if (r->ai->Class == AIC_monitor) {

				display (def (r->when) ? "now" : "pnd");
				if (pless (e->waketime, r->when))
					display (' ');   // Obsolete
				else if (e->chain == r)
					display ('*');   // Current
				else
					display ('?');

				if (!isStationId (sid)) {
					// Identify the station
					if (e->station != NULL &&
						ident (e->station) >= 0)
						   display (ident (e->station));
					else
						   display ("Sys");
				}
				display (e->process->getTName ());
				display (e->process->Id);
				display (e->process->zz_sn (r->pstate));
			}

			if ((r = r->other) == e->chain) break;
		}
	}
}
