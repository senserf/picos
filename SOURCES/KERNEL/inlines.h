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

/* --- */

/* ---------------------------------------------------------- */
/* Some inline functions (AT&T Cfront doesn't like them being */
/* inline)                                                    */
/* ---------------------------------------------------------- */

#if     BIG_precision > 1

/* ---------------------------------- */
/* Multiple precision integer package */
/* ---------------------------------- */

/* ----------- */
/* Conversions */
/* ----------- */

INLINE BIG::BIG (LONG i) {      // Conversion from LONG integer
		x [0] = i;
		for (int k = 1; k < BIG_precision;) x [k++] = 0L;
};
#if ZZ_LONG_is_not_long
INLINE BIG::BIG (long i) {
		x [0] = (LONG) i;
		for (int k = 1; k < BIG_precision;) x [k++] = 0L;
};
#endif
//#if ZZ_SOL > 4
INLINE BIG::BIG (int i) {
		x [0] = (LONG) i;
		for (int k = 1; k < BIG_precision;) x [k++] = 0L;
};
//#endif

/* ----------------------------------------- */
/* Basic arithmetic relations and operations */
/* ----------------------------------------- */

INLINE  int  operator== (const BIG &a, const BIG &b) {
	for (int k = 0; k < BIG_precision; k++)
		if (a.x [k] != b.x [k]) return (0);
	return (1);
};

#if  ZZ_TAG
INLINE  int  operator== (const ZZ_TBIG &a, const ZZ_TBIG &b) {
	for (int k = 0; k < BIG_precision; k++)
		if (a.x [k] != b.x [k]) return (0);
	return (a.tag == b.tag);
};
#endif

INLINE  int  operator!= (const BIG &a, const BIG &b) {
	for (int k = 0; k < BIG_precision; k++)
		if (a.x [k] != b.x [k]) return (1);
	return (0);
};
#endif

INLINE ZZ_REQUEST *ZZ_REQUEST::min_request () {

/* ---------------------------------------------- */
/* Finds the request with the minimum 'when' time */
/* ---------------------------------------------- */

	ZZ_REQUEST      *rm, *rc;
#if  ZZ_TAG
	ZZ_TBIG         t;
	int             q;
#else
	TIME            t;
#endif


#if	ZZ_DET

	for (t = (rm = this) -> when, rc = this -> other;
		rc != this; rc = rc -> other) {

#if  ZZ_TAG
		if ((q = rc -> when . cmp (t)) >= 0) continue;
#else
		if (rc -> when >= t) continue;
#endif
		t = (rm = rc) -> when;
	}
        return rm;


#else                                             // DET


	int             rcount;

	for (t = (rm = this) -> when, rc = this -> other, rcount = 1;
		rc != this; rc = rc -> other) {

#if  ZZ_TAG
		if ((q = rc -> when . cmp (t)) > 0) continue;
		if (q == 0) {
#else
		if (rc -> when > t) continue;
		if (rc -> when == t) {
#endif
			rcount++;
			continue;
		}

		t = (rm = rc) -> when;
		rcount = 1;
	}

	if (rcount == 1) return (rm);

	// More than one request with the minimum time
	// Select one at random

	t = rm -> when;

	for (rcount = (int) toss (rcount); rcount; rcount--)
		for (rm = rm -> other; !(rm->when == t); rm = rm -> other);

	return (rm);
#endif                                            // DET

};

INLINE void ZZ_EVENT::enqueue () {

/* ----------------------------------- */
/* Puts the event into the event queue */
/* ----------------------------------- */

	register ZZ_EVENT *ep;

	zz_npee++;              // Number of pending events

	assert (waketime >= Time,
       "EVENT->enqueue: attempt to change the past (BIG precision too small?)");

#if	ZZ_DET

        for (ep = zz_rsent.prev; ; ep = ep->prev) {
#if  ZZ_TAG
		if (ep->waketime . cmp (waketime) <= 0) {
#else
		if (ep->waketime <= waketime) {
#endif
                        ep->next->prev = this;
                        next = ep->next;
                        ep->next = this;                       
                        prev = ep;
                        return;
		}
        }


#else


	for (ep = zz_eq; ; ep = ep->next) {
#if  ZZ_TAG
		if (ep->waketime . cmp (waketime) >= 0) {
#else
		if (ep->waketime >= waketime) {
#endif
			ep->prev->next = this;
			prev = ep->prev;
			next = ep;
			ep->prev = this;
			return;
		}
	}

#endif
};

INLINE  void    ZZ_EVENT::enqueue (ZZ_EVENT *h) {

/* -------------------------------------------------- */
/* Puts the event into the event queue using "hint" h */
/* -------------------------------------------------- */

	zz_npee++;

	assert (waketime >= Time,
  "EVENT->enqueue(h): attempt to change the past (BIG precision too small?)");

#if  ZZ_TAG
	if (h->waketime . cmp (waketime) > 0) {
		// Examine prior events
		for (h = h->prev; h->waketime.cmp (waketime) > 0; h = h->prev);
#else
	if (h->waketime > waketime) {
		// Examine prior events
		for (h = h->prev; h->waketime > waketime; h = h->prev);
#endif

		h->next->prev = this;
		next = h->next;
		prev = h;
		h->next = this;
		return;
	}

	// Examine future events

#if	ZZ_DET

#if  ZZ_TAG
	for (; h != &zz_rsent && h->waketime.cmp (waketime) <= 0; h = h->next);
#else
	for (; h != &zz_rsent && h->waketime <= waketime; h = h->next);
#endif

#else           // DET

#if  ZZ_TAG
	for (; h->waketime . cmp (waketime) < 0; h = h->next);
#else
	for (; h->waketime < waketime; h = h->next);
#endif

#endif

	h->prev->next = this;
	prev = h->prev;
	next = h;
	h->prev = this;
};

INLINE  void    ZZ_EVENT::cancel () {

/* --------------------------------------------- */
/* Removes event and its associated request list */
/* --------------------------------------------- */

	ZZ_REQUEST *cp, *cq;

	if (chain != NULL) {
		for (cp = chain; ; ) {
			cq = cp -> other;
			pool_out (cp);
			delete (cp);
			if (cq == chain) break;
			cp = cq;
		}
	}

	remove ();
};

INLINE  ZZ_EVENT  *ZZ_EVENT::get () {

/* ----------------------------------- */
/* Get next event from the event queue */
/* ----------------------------------- */

	ZZ_EVENT        *e;
#if  ZZ_TAG
	ZZ_TBIG         t;
#else
	TIME            t;
#endif

#if	ZZ_DET
#else
        ZZ_EVENT        *eet [ZZ_EETSIZE];
	int             ne;
#endif

        if (undef (t = (e = zz_eq) -> waketime)) {
#if  ZZ_RSY
		if (ZZ_ResyncMsec != 0)
#endif
#if  ZZ_RSY || ZZ_REA
			return zz_sentinel_event;
#endif
#if  ZZ_REA == 0
		if (zz_noevents ())
		  return (zz_sentinel_event);
                else
                  // Retry - somebody was waiting for that
                  t = (e = zz_eq) -> waketime;
#endif
	}

#if	ZZ_DET
#else
	for (eet [ne = 0] = e; t == (e = e->next)->waketime;) {

		if (ne >= ZZ_EETSIZE - 1)
			excptn ("EVENT->get: stack overflow");
		eet [++ne] = e;
	}

	e = ne ? eet [toss (ne+1)] : eet [0];

	assert (t >= Time, "EVENT->get: time going backward");
#endif
	e -> remove ();
	return (e);
};

INLINE  ZZ_EVENT::ZZ_EVENT (
                                Station *st,
                                void *i_1,
                                void *i_2,
                                Process *prc,
                                AI   *a,
                                int  e,
                                int  pst,
                                ZZ_REQUEST *ch) {

        // Generates a new "stored" event with undefined waketime

#if  ZZ_TAG
                waketime.set (TIME_inf, zz_sysevent_tag);
#else
                waketime = TIME_inf;
#endif
                station  = st;
                Info01   = i_1;
                Info02   = i_2;
                process  = prc;
                ai       = a;
                event_id = e;
                pstate   = pst;
                chain    = ch;

                store ();
};

#if	ZZ_NOL
INLINE Port *Station::idToPort (int id) {

	register Port *p;
	register int sid = id;

	for (p = Ports; p != NULL && sid--; p = p->nextp);
	Assert (p != NULL, "Station->idToPort: %s, illegal port Id %1d",
		getSName (), id);
	return (p);
};

INLINE Port *idToPort (int id) {

	register Port *p;
	register int sid = id;

	Assert (TheStation != NULL,
		"idToPort: current station undefined");
	for (p = TheStation->Ports; p != NULL && sid--; p = p->nextp);
	Assert (p != NULL, "idToPort: %s, illegal port Id %1d",
		TheStation->getSName (), id);
	return (p);
};

INLINE TIME Port::bitsToTime (Long b) {
	return ((TIME)XRate * b);
};
#endif	/* NOL */

#if	ZZ_NOR
INLINE Transceiver *Station::idToTransceiver (int id) {

	register Transceiver *p;
	register int sid = id;

	for (p = Transceivers; p != NULL && sid--; p = p->nextp);
	Assert (p != NULL, "Station->idToTransceiver: %s illegal transceiver "
		"Id %1d", getSName (), id);
	return (p);
};

INLINE Transceiver *idToTransceiver (int id) {

	register Transceiver *p;
	register int sid = id;

	Assert (TheStation != NULL,
		"idToTransceiver: current station undefined");
	for (p = TheStation->Transceivers; p != NULL && sid--; p = p->nextp);
	Assert (p != NULL, "idToTransceiver: %s, illegal transceiver Id %1d",
		TheStation->getSName (), id);
	return (p);
};

#if	ZZ_R3D
INLINE void Station::getLocation (double &x, double &y, double &z) {
	Assert (Transceivers != NULL, "Station->getLocation: no transceivers");
	Transceivers->getLocation (x, y, z);
};
#else
INLINE void Station::getLocation (double &x, double &y) {
	Assert (Transceivers != NULL, "Station->getLocation: no transceivers");
	Transceivers->getLocation (x, y);
};
#endif

#endif	/* NOR */

INLINE Mailbox *Station::idToMailbox (int id) {

	register Mailbox *q;
	register int sid = id;

	for (q = Mailboxes; q != NULL && sid--; q = q->nextm);
	Assert (q != NULL, "Station->idToMailbox: %s, illegal mailbox Id %1d",
		getSName (), id);
	return (q);
};

INLINE Mailbox *idToMailbox (int id) {

	register Mailbox *q;
	register int sid = id;

	Assert (TheStation != NULL,
		"idToMailbox: current station undefined");
	for (q = TheStation->Mailboxes; q != NULL && sid--; q = q->nextm);
	Assert (q != NULL, "idToMailbox: %s, illegal mailbox Id %1d",
		TheStation->getSName (), id);
	return (q);
};

/* ------------------------ */
/* Random number generators */
/* ------------------------ */

INLINE LONG lRndTolerance (LONG a, LONG b, int q) {

/* ---------------------------------------------------------- */
/* Generates  a  number between a and b according to the Beta */
/* (q,q) distribution (LONG version)                          */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("lRndTolerance");
	return 0;
#else
	double                  y;
	register        double  p;
	register        int     i;

	assert (q > 0, "lRndTolerance: q (%1d) must be > 0", q);
	for (p = 1.0, i = 1; i <= q; i++, p *= 1.0 - rnd (SEED_delay));
	y = -log (p);
	for (p = 1.0, i = 1; i <= q; i++, p *= 1.0 - rnd (SEED_delay));
	y = y / (y - log (p));
	return ((LONG) (a + (b - a + 1) * y));
#endif
};

INLINE LONG lRndTolerance (double a, double b, int q) {

/* ---------------------------------------------------------- */
/* Generates  a  number between a and b according to the Beta */
/* (q,q)  distribution  (LONG   version   based   on   double */
/* arguments)                                                 */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("lRndTolerance");
	return 0;
#else
	double                  y;
	register        double  p;
	register        int     i;

	assert (q > 0, "lRndTolerance: q (%1d) must be > 0", q);
	for (p = 1.0, i = 1; i <= q; i++, p *= 1.0 - rnd (SEED_delay));
	y = -log (p);
	for (p = 1.0, i = 1; i <= q; i++, p *= 1.0 - rnd (SEED_delay));
	y = y / (y - log (p));
	return ((LONG)(a + (b - a + 1) * y));
#endif
};

INLINE TIME tRndTolerance   (double a, double b, int q) {

/* ---------------------------------------------------------- */
/* Generates  a  number between a and b according to the Beta */
/* (q,q) distribution (BIG version based on  double  or  LONG */
/* arguments)                                                 */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("tRndTolerance");
	return 0;
#else

	double                  y;
	register        double  p;
	register        int     i;

	assert (q > 0, "tRndTolerance: q (%1d) must be > 0", q);
	for (p = 1.0, i = 1; i <= q; i++, p *= 1.0 - rnd (SEED_delay));
	y = -log (p);
	for (p = 1.0, i = 1; i <= q; i++, p *= 1.0 - rnd (SEED_delay));
	y = y / (y - log (p));
	return ((TIME)(a + (((b - a) + 1.0) * y)));
#endif
};

INLINE double dRndTolerance   (double a, double b, int q) {

/* ---------------------------------------------------------- */
/* Generates  a  number between a and b according to the Beta */
/* (q,q) distribution (BIG version based on  double  or  LONG */
/* arguments)                                                 */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("dRndTolerance");
	return 0;
#else

	double                  y;
	register        double  p;
	register        int     i;

	assert (q > 0, "tRndTolerance: q (%1d) must be > 0", q);
	for (p = 1.0, i = 1; i <= q; i++, p *= 1.0 - rnd (SEED_delay));
	y = -log (p);
	for (p = 1.0, i = 1; i <= q; i++, p *= 1.0 - rnd (SEED_delay));
	y = y / (y - log (p));

	return a + (b - a) * y;
#endif
};

INLINE double dRndGauss (double mean, double sigma) {

/* ----------------------------------------------------------------------- */
/* Box-Mueller dual variate. This generates a Gauss (normal) - distributed */
/* random number around the specified mean with the specified sigma.       */
/* ----------------------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("dRndGauss");
	return 0.0;
#else
	static double extra = HUGE;
	double r, t;

	if (extra != HUGE) {
		r = extra;
		extra = HUGE;
	} else {
		do {
			r = -1.0 + 2.0 * rnd (SEED_toss);
			extra = -1.0 + 2.0 * rnd (SEED_toss);
			t = (r * r + extra * extra);
		} while (t > 1.0 || t == 0.0);

		t = sqrt (-2.0 * log (t) / t);

		r *= t;
		extra *= t;
	}

	return sigma * r + mean;
#endif
};
