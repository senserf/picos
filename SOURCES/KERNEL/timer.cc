/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-06   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

/* ------------ */
/* The Timer AI */
/* ------------ */

#include        "system.h"

zz_timer::zz_timer () {

/* --------------- */
/* The constructor */
/* --------------- */

	Class = AIC_timer;
	Id = NONE;
        WList = NULL;
}

#if  ZZ_TAG
void    zz_timer::wait (TIME ev, int pstate, LONG tag) {
#else
void    zz_timer::wait (TIME ev, int pstate) {
#endif

/* ---------------------- */
/* The Timer wait request */
/* ---------------------- */

	TIME    t;

#if     ZZ_TOL
	double  p, y;
	int     i;
#endif
#if     ZZ_TAG
	int     q;
#endif
	if_from_observer ("Timer->wait: called from an observer");

	if (zz_c_first_wait) {

		zz_c_other = NULL;    // No requests so far

		// Create a new simulation event
		zz_c_wait_event = new ZZ_EVENT;
		zz_c_wait_event -> station = TheStation;
		zz_c_wait_event -> process = TheProcess;

#if     ZZ_TOL

		if (TheStation->CQuality) {

			for (p = 1.0, i = 1; i <= TheStation->CQuality;
				i++, p *= 1.0 - rnd (SEED_delay));

			y = -log (p);

			for (p = 1.0, i = 1; i <= TheStation->CQuality;
				i++, p *= 1.0 - rnd (SEED_delay));

			y = y / (y - log (p));

			if ((y = TheStation->CTolerance * (y+y-1.0)) < 0.0)
				ev = ev - (BIG) ((ev * -y) + 0.5);
			else
				ev = ev + (BIG) ((ev * y) + 0.5);
		}
#endif
		t = Time + ev;
	
#if     BIG_precision == 1
		new ZZ_REQUEST (&WList, this, (LPointer) t, pstate);
#else
		new ZZ_REQUEST (&WList, this, NONE, pstate);
#endif
		assert (zz_c_other != NULL,
			"Timer->wait: internal error -- null request");

		zz_c_wait_event -> pstate = pstate;
		zz_c_wait_event -> ai = this;
		zz_c_wait_event -> event_id = zz_c_other->event_id;
#if  ZZ_TAG
		zz_c_wait_tmin . set (t, tag);
		zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
		zz_c_wait_event -> waketime = zz_c_wait_tmin = t;
#endif
		zz_c_wait_event -> chain = zz_c_other;
		zz_c_wait_event -> Info01 = NULL;
		zz_c_wait_event -> Info02 = NULL;

		zz_c_whead = zz_c_other;
		zz_c_other -> when = zz_c_wait_tmin;
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;

		zz_c_first_wait = NO;
		zz_c_wait_event->enqueue ();

	} else {

#if     ZZ_TOL

		if (TheStation->CQuality) {

			for (p = 1.0, i = 1; i <= TheStation->CQuality;
				i++, p *= 1.0 - rnd (SEED_delay));

			y = -log (p);

			for (p = 1.0, i = 1; i <= TheStation->CQuality;
				i++, p *= 1.0 - rnd (SEED_delay));

			y = y / (y - log (p));

			if ((y = TheStation->CTolerance * (y+y-1.0)) < 0.0)
				ev = ev - (BIG) ((ev * -y) + 0.5);
			else
				ev = ev + (BIG) ((ev * y) + 0.5);
		}
#endif
		t = Time + ev;
	
#if     BIG_precision == 1
		new ZZ_REQUEST (&WList, this, (LPointer) t, pstate);
#else
		new ZZ_REQUEST (&WList, this, NONE, pstate);
#endif
		assert (zz_c_other != NULL,
			"Timer->wait: internal error -- null request");

#if     ZZ_TAG
		if ((q = zz_c_wait_tmin . cmp (t, tag)) > 0) {
#else
		if (zz_c_wait_tmin > t) {
#endif
			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = zz_c_other->event_id;
#if     ZZ_TAG
			zz_c_wait_tmin . set (t, tag);
			zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
			zz_c_wait_event -> waketime = zz_c_wait_tmin = t;
#endif
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = NULL;
			zz_c_wait_event -> Info02 = NULL;
			zz_c_wait_event -> reschedule ();
                }
#if	ZZ_DET
#else

#if     ZZ_TAG
		else if (q == 0 && FLIP) {
#else
		else if (zz_c_wait_tmin == t && FLIP) {
#endif
			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = zz_c_other->event_id;
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = NULL;
			zz_c_wait_event -> Info02 = NULL;
		}
#endif

#if     ZZ_TAG
		zz_c_other -> when . set (t, tag);
#else
		zz_c_other -> when = t;
#endif
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;
	}
}

#if  ZZ_TAG
void    zz_timer::zz_proceed (int pstate, LONG tag) {
	int q;
#else
void    zz_timer::zz_proceed (int pstate) {
#endif

/* --------------------------------------- */
/* The implementation of 'proceed (state)' */
/* --------------------------------------- */

	if_from_observer ("proceed: called from an observer");

	if (zz_c_first_wait) {

		zz_c_other = NULL;    // No requests so far

		// Create new simulation event
		zz_c_wait_event = new ZZ_EVENT;
		zz_c_wait_event -> station = TheStation;
		zz_c_wait_event -> process = TheProcess;

#if     BIG_precision == 1
		new ZZ_REQUEST (this, (LPointer) Time,
				pstate, NONE, Info01, Info02);
#else
		new ZZ_REQUEST (this, NONE, pstate, NONE, Info01, Info02);
#endif
		assert (zz_c_other != NULL,
			"proceed: internal error -- null request");

		zz_c_wait_event -> pstate = pstate;
		zz_c_wait_event -> ai = this;
		zz_c_wait_event -> event_id = zz_c_other->event_id;
#if     ZZ_TAG
		zz_c_wait_tmin . set (Time, tag);
		zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
		zz_c_wait_event -> waketime = zz_c_wait_tmin = Time;
#endif
		zz_c_wait_event -> chain = zz_c_other;
		zz_c_wait_event -> Info01 = Info01;
		zz_c_wait_event -> Info02 = Info02;

		zz_c_whead = zz_c_other;
		zz_c_other -> when = zz_c_wait_tmin;
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;

		zz_c_first_wait = NO;
		zz_c_wait_event->enqueue ();

	} else {

#if     BIG_precision == 1
		new ZZ_REQUEST (this, (LPointer) Time, pstate, NONE,
			Info01, Info02);
#else
		new ZZ_REQUEST (this, NONE, pstate, NONE, Info01, Info02);
#endif
		assert (zz_c_other != NULL,
			"proceed: internal error -- null request");

#if     ZZ_TAG
		if ((q = zz_c_wait_tmin . cmp (Time, tag)) > 0) {
#else
		if (zz_c_wait_tmin > Time) {
#endif
			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = zz_c_other->event_id;
#if     ZZ_TAG
			zz_c_wait_tmin . set (Time, tag);
			zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
			zz_c_wait_event -> waketime = zz_c_wait_tmin = Time;
#endif
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = Info01;
			zz_c_wait_event -> Info02 = Info02;
			zz_c_wait_event -> reschedule ();
                }
#if	ZZ_DET
#else

#if     ZZ_TAG
		else if (q == 0 && FLIP) {
#else
		else if (FLIP) {
#endif
			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = zz_c_other->event_id;
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = Info01;
			zz_c_wait_event -> Info02 = Info02;
		}
#endif

#if     ZZ_TAG
		zz_c_other -> when . set (Time, tag);
#else
		zz_c_other -> when = Time;
#endif
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;
	}
}

#if  ZZ_TAG
void    zz_timer::zz_skipto (int pstate, LONG tag) {
int     q;
#else
void    zz_timer::zz_skipto (int pstate) {
#endif

/* -------------------------------------- */
/* The implementation of 'skipto (state)' */
/* -------------------------------------- */

	TIME    t;

	if_from_observer ("skipto: called from an observer");

	t = Time + 1;

	if (zz_c_first_wait) {

		zz_c_other = NULL;    // No requests so far

		// Create new simulation event
		zz_c_wait_event = new ZZ_EVENT;
		zz_c_wait_event -> station = TheStation;
		zz_c_wait_event -> process = TheProcess;

#if     BIG_precision == 1
		new ZZ_REQUEST (this, (LPointer) t, pstate);
#else
		new ZZ_REQUEST (this, NONE, pstate);
#endif
		assert (zz_c_other != NULL,
			"skipto: internal error -- null request");

		zz_c_wait_event -> pstate = pstate;
		zz_c_wait_event -> ai = this;
		zz_c_wait_event -> event_id = zz_c_other->event_id;
#if  ZZ_TAG
		zz_c_wait_tmin . set (t, tag);
		zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
		zz_c_wait_event -> waketime = zz_c_wait_tmin = t;
#endif
		zz_c_wait_event -> chain = zz_c_other;
		zz_c_wait_event -> Info01 = NULL;
		zz_c_wait_event -> Info02 = NULL;

		zz_c_whead = zz_c_other;
		zz_c_other -> when = zz_c_wait_tmin;
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;

		zz_c_first_wait = NO;
		zz_c_wait_event->enqueue ();

	} else {

#if     BIG_precision == 1
		new ZZ_REQUEST (this, (LPointer) t, pstate);
#else
		new ZZ_REQUEST (this, NONE, pstate);
#endif
		assert (zz_c_other != NULL,
			"proceed: internal error -- null request");

#if     ZZ_TAG
		if ((q = zz_c_wait_tmin . cmp (t, tag)) > 0) {
#else
		if (zz_c_wait_tmin > t) {
#endif
			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = zz_c_other->event_id;
#if     ZZ_TAG
			zz_c_wait_tmin . set (t, tag);
			zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
			zz_c_wait_event -> waketime = zz_c_wait_tmin = t;
#endif
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = NULL;
			zz_c_wait_event -> Info02 = NULL;
			zz_c_wait_event -> reschedule ();
                }
#if	ZZ_DET
#else

#if     ZZ_TAG
		else if (q == 0 && FLIP) {
#else
		else if (zz_c_wait_tmin != Time && FLIP) {
#endif
			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = zz_c_other->event_id;
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = NULL;
			zz_c_wait_event -> Info02 = NULL;
		}
#endif

#if     ZZ_TAG
		zz_c_other -> when . set (t, tag);
#else
		zz_c_other -> when = t;
#endif
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;
	}
}

#if     BIG_precision != 1

#if  ZZ_TAG
void    zz_timer::wait (LONG ev, int pstate, LONG tag) {
	int  q;
#else
void    zz_timer::wait (LONG ev, int pstate) {
#endif

/* ------------------------------------ */
/* Wait request with LONG first operand */
/* ------------------------------------ */

	TIME    t;

#if     ZZ_TOL
	double  p, y;
	int     i;
#endif
	if_from_observer ("Timer->wait: called from an observer");

	if (zz_c_first_wait) {

		zz_c_other = NULL;    // No requests so far

		// Create new simulation event
		zz_c_wait_event = new ZZ_EVENT;
		zz_c_wait_event -> station = TheStation;
		zz_c_wait_event -> process = TheProcess;

#if     ZZ_TOL

		if (TheStation->CQuality) {

			for (p = 1.0, i = 1; i <= TheStation->CQuality;
				i++, p *= 1.0 - rnd (SEED_delay));

			y = -log (p);

			for (p = 1.0, i = 1; i <= TheStation->CQuality;
				i++, p *= 1.0 - rnd (SEED_delay));

			y = y / (y - log (p));

			if ((y = TheStation->CTolerance * (y+y-1.0)) < 0.0)
				ev = (LONG) (ev - ((ev * -y) + 0.5));
			else
				ev = (LONG) (ev + ((ev * y) + 0.5));
		}
#endif
		t = Time + ev;
	
		new ZZ_REQUEST (&WList, this, NONE, pstate);

		assert (zz_c_other != NULL,
			"Timer->wait: internal error -- null request");

		zz_c_wait_event -> pstate = pstate;
		zz_c_wait_event -> ai = this;
		zz_c_wait_event -> event_id = NONE;
#if  ZZ_TAG
		zz_c_wait_tmin . set (t, tag);
		zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
		zz_c_wait_event -> waketime = zz_c_wait_tmin = t;
#endif
		zz_c_wait_event -> chain = zz_c_other;
		zz_c_wait_event -> Info01 = NULL;
		zz_c_wait_event -> Info02 = NULL;

		zz_c_whead = zz_c_other;
		zz_c_other -> when = zz_c_wait_tmin;
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;

		zz_c_first_wait = NO;
		zz_c_wait_event->enqueue ();

	} else {

#if     ZZ_TOL

		if (TheStation->CQuality) {

			for (p = 1.0, i = 1; i <= TheStation->CQuality;
				i++, p *= 1.0 - rnd (SEED_delay));

			y = -log (p);

			for (p = 1.0, i = 1; i <= TheStation->CQuality;
				i++, p *= 1.0 - rnd (SEED_delay));

			y = y / (y - log (p));

			if ((y = TheStation->CTolerance * (y+y-1.0)) < 0.0)
				ev = (LONG) (ev - ((ev * -y) + 0.5));
			else
				ev = (LONG) (ev + ((ev * y) + 0.5));
		}
#endif
		t = Time + ev;
	
		new ZZ_REQUEST (&WList, this, NONE, pstate);

		assert (zz_c_other != NULL,
			"Timer->wait: internal error -- null request");

#if     ZZ_TAG
		if ((q = zz_c_wait_tmin . cmp (t, tag)) > 0) {
#else
		if (zz_c_wait_tmin > t) {
#endif
			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = NONE;
#if     ZZ_TAG
			zz_c_wait_tmin . set (t, tag);
			zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
			zz_c_wait_event -> waketime = zz_c_wait_tmin = t;
#endif
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = NULL;
			zz_c_wait_event -> Info02 = NULL;
			zz_c_wait_event -> reschedule ();
                }
#if	ZZ_DET
#else

#if     ZZ_TAG
		else if (q == 0 && FLIP) {
#else
		else if (zz_c_wait_tmin == t && FLIP) {
#endif
			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = NONE;
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = NULL;
			zz_c_wait_event -> Info02 = NULL;
		}
#endif

#if     ZZ_TAG
		zz_c_other -> when . set (t, tag);
#else
		zz_c_other -> when = t;
#endif
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;
	}
}
#endif

TIME zz_timer::zz_getdelay (Process *p, int state) {

/* ----------------------------------------------------------------- */
/* Gets the delay for a timer wait transiting to the specified state */
/* ----------------------------------------------------------------- */

  ZZ_REQUEST *rq;
  TIME minwt;
#if ZZ_TAG
  TIME t;
#endif
  minwt = TIME_inf;
  for (rq = WList; rq != NULL; rq = rq -> next) {
    // Locate this process and this state
    if (rq->pstate == state && rq->event->process == p) {
#if ZZ_TAG
      rq->when . get (t);
      if (t < minwt) minwt = t;
#else
      if (rq->when < minwt) minwt = rq->when;
#endif
    }
  }
  if (minwt != TIME_inf) minwt -= Time;
  return minwt;
};

int zz_timer::zz_setdelay (Process *p, int state, TIME nd) {

/* -------------------------------------------------- */
/* Resets the timer transiting to the specified state */
/* -------------------------------------------------- */

  ZZ_REQUEST *rq, *sr;
  ZZ_EVENT *e;

  sr = NULL;
  nd += Time;
  for (rq = WList; rq != NULL; rq = rq->next) {
    // Locate this process and this state
    if (rq->pstate == state && (e = rq->event)->process == p) {
#if ZZ_TAG
      rq->when . set (nd);
#else
      rq->when = nd;
#endif
      sr = rq;
    }
  }
  if (sr != NULL) {
    sr = sr->min_request ();
    e->new_top_request (sr);
    return OK;
  } else {
    return ERROR;
  }
};

sexposure (zz_timer)

/* ---------------------------------- */
/* Defines exposures for the Timer AI */
/* ---------------------------------- */

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

void     zz_timer::exPrint0 (const char *hdr, int sid) {

/* ------------------------------------- */
/* Print the full list of Timer requests */
/* ------------------------------------- */

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
		Ouf << "       Time   St";
	else
		Ouf << "           Time ";

	Ouf << "    Process/Idn     TState         AI/Idn" <<
		"      Event      State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && (e->station == NULL ||
			e->chain == NULL || ident (e->station) != sid))
				continue;

		if ((r = e->chain) == NULL) {
			// System event
			if (zz_flg_nosysdisp || e->ai->Class != AIC_timer)
				continue;
			ptime (e->waketime, 11);
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

				if (r->ai->Class == AIC_timer) {

					if (!isStationId (sid))
						ptime (r->when, 11);
					else
						ptime (r->when, 15);

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

void    zz_timer::exPrint1 (const char *hdr, int sid) {

/* -------------------------------------------- */
/* Print the abbreviated list of Timer requests */
/* -------------------------------------------- */

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
		Ouf << "                Time     St";
	else
		Ouf << "                      Time ";
	Ouf << "      Process/Ident        State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if ((r = e->chain) == NULL || e->station == NULL) continue;
		if (isStationId (sid) && ident (e->station) != sid)
				continue;

		while (1) {

			if (r->ai->Class == AIC_timer) {

				if (!isStationId (sid))
					ptime (r->when, 20);
				else
					ptime (r->when, 26);

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

void     zz_timer::exDisplay0 (int sid) {

/* ---------------------------- */
/* Display the full Timer queue */
/* ---------------------------- */

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;
	Long               l;

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && (e->station == NULL ||
			e->chain == NULL || ident (e->station) != sid))
				continue;

		if ((r = e->chain) == NULL) {

			// System event

			if (zz_flg_nosysdisp || e->ai->Class != AIC_timer)
				continue;
			dtime (e->waketime);
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
				if (r->ai->Class == AIC_timer) {
					dtime (r->when);
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

void    zz_timer::exDisplay1 (int sid) {

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

			if (r->ai->Class == AIC_timer) {

				dtime (r->when);
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
