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

/* --- */

/* --------------- */
/* Process service */
/* --------------- */

#include        "system.h"
#include	"cdebug.h"

/* -------------------- */
/* Borrowed from main.c */
/* -------------------- */
extern  jmp_buf zz_mloop;                // Return to the main loop
extern  int     zz_setjmp_done;          // Flag == longjmp legal

#if  ZZ_TAG
static  ZZ_REQUEST  *CRList = NULL;      // START request list
#endif

void    Process::zz_start () {

	Process    *savecontext;
	int        cfv;
	ZZ_REQUEST *cwh, *cot;
	ZZ_EVENT   *cwe;
#if  ZZ_TAG
	ZZ_TBIG    cwm;
#else
	TIME       cwm;
#endif
	Id = sernum++;

	if_from_observer ("Process: observers cannot create processes");

	Owner = TheStation;
	Father = (TheProcess == Kernel) ? NULL : TheProcess;

	TWList = SWList = NULL;
	ChList = NULL;
	ISender = NULL;

	// Save the current context for a second
	savecontext = TheProcess;
	cfv = zz_c_first_wait;
	cwh = zz_c_whead;
	cot = zz_c_other;
	cwe = zz_c_wait_event;
	cwm = zz_c_wait_tmin;

	// Assume the context of the started process (formally nonexistent yet)
	TheProcess = this;
	zz_c_first_wait = YES;

	// Generate the launching event looking as a START request to itself
	wait (START, 0);
	// Restore previous context
	zz_c_first_wait = cfv;
	zz_c_whead = cwh;
	zz_c_other = cot;
	zz_c_wait_event = cwe;
	zz_c_wait_tmin = cwm;
	TheProcess = savecontext;

	Class = AIC_process;

	// Add the object to the owner's list

	if (TheProcess != NULL) {
		// TheProcess == NULL means that Kernel is being created,
		// otherwise, the process gets appended to the parent's
		// children list
		pool_in (this, TheProcess->ChList);
	}
};

#if  ZZ_TAG
void    Process::setSP (LONG tag) {

/* ------------------------- */
/* Sets the startup priority */
/* ------------------------- */

	ZZ_REQUEST *rq;
	ZZ_EVENT *ev;
	int q;

	assert (TheProcess != this, "Process->setSP: %s, called from own code",
		getSName ());
	for (rq = CRList; rq != NULL; rq = rq->next)
		if ((ev=rq->event)->process == this) {
			rq->when . set (Time, tag);
			if ((q = ev->waketime . cmp (rq->when)) > 0 ||
				(q == 0 && FLIP))
					ev->new_top_request (rq);
			return;
		}
	excptn ("Process->setSP illegal: %s, process not in startup phase",
		getSName ());
}
#endif

const char    *Process::zz_sn (int) {

/* -------------------------------------- */
/* Returns the name of the state number i */
/* -------------------------------------- */

	// This is a virtual function
	return ("undefined");
}

const char    *ZZ_SProcess::zz_sn (int i) {

/* ---------------------------------------------------------- */
/* Returns  the  name  of  the  state  number i (for a system */
/* process)                                                   */
/* ---------------------------------------------------------- */

	return ((i > zz_ns) ? "undefined" : zz_sl [i]);
}

#if  ZZ_TAG
void    Process::wait (int ev, int pstate, LONG tag) {
	int q;
#else
void    Process::wait (int ev, int pstate) {
#endif

/* ------------------------ */
/* The process wait request */
/* ------------------------ */

	TIME    t;

	if_from_observer ("Process->wait: called from an observer");

	if (zz_c_first_wait) {

		zz_c_other = NULL;    // No requests so far

		// Create new simulation event
		zz_c_wait_event = new ZZ_EVENT;
		zz_c_wait_event -> station = TheStation;
		zz_c_wait_event -> process = TheProcess;
	}

#if ZZ_REA
        assert (ev >= CLEAR, "Process->wait: %s, illegal event %1d",
		getSName (), ev);
#else
        if (ev <= STALL)
	  assert (ev == STALL && this == Kernel,
                             "Process->wait: %s, illegal event %1d",
		getSName (), ev);
#endif

	if (ev >= 0) {
		// State wait request
		new ZZ_REQUEST (&SWList, this, ev, pstate, RQTYPE_PRC);
		assert (zz_c_other != NULL,
			"Process->wait: internal error -- null request");
		t = TIME_inf;
		if (zz_c_first_wait) {
			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = ev;
#if  ZZ_TAG
			zz_c_wait_tmin . set (TIME_inf, tag);
			zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
			zz_c_wait_event -> waketime = zz_c_wait_tmin = TIME_inf;
#endif
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = NULL;
			zz_c_wait_event -> Info02 = NULL;
		}
	} else {
	  switch (ev) {
	     case DEATH:
		// Termination
	     case CHILD:
		// Child termination
#ifdef STALL
             case STALL:
		// Stall (no progress)
#endif
		new ZZ_REQUEST (&TWList, this, ev, pstate, RQTYPE_PRC);
		assert (zz_c_other != NULL,
			"Process->wait: internal error -- null request");
		t = TIME_inf;
		if (zz_c_first_wait) {
			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = ev;
#if  ZZ_TAG
			zz_c_wait_tmin . set (TIME_inf, tag);
			zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
			zz_c_wait_event -> waketime = zz_c_wait_tmin = TIME_inf;
#endif
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = NULL;
			zz_c_wait_event -> Info02 = NULL;
		}
		break;
	     case START:
		// START request
#if     ZZ_TAG
		new ZZ_REQUEST (&CRList, this, ev, pstate, RQTYPE_PRC);
#else
		new ZZ_REQUEST (this, ev, pstate, RQTYPE_PRC);
#endif
		assert (zz_c_other != NULL,
			"Process->wait: internal error -- null request");
		t = Time;       // Occurs immediately
#if     ZZ_TAG
		if (zz_c_first_wait || (q = zz_c_wait_tmin . cmp (t, tag)) > 0
			|| (q == 0 && FLIP)) {
#else
		if (zz_c_first_wait || zz_c_wait_tmin > t || FLIP) {
#endif
			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = START;
#if  ZZ_TAG
			zz_c_wait_tmin . set (t, tag);
			zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
			zz_c_wait_event -> waketime = zz_c_wait_tmin = t;
#endif
			zz_c_wait_tmin =
				zz_c_wait_event -> waketime = zz_c_wait_tmin;
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = NULL;
			zz_c_wait_event -> Info02 = NULL;

			if (!zz_c_first_wait)
				zz_c_wait_event -> reschedule ();
		}
		break;
	     case SIGNAL:
	     case CLEAR:
		// SIGNAL or CLEAR wait request
		new ZZ_REQUEST (&TWList, this, ev, pstate, RQTYPE_PRC,
                  (void*) this /* for CLEAR */ );
		assert (zz_c_other != NULL,
			"Process->wait: internal error -- null request");
		
                if (ev == SIGNAL) {
		  if (ISender != NULL) {
			t = Time;
			zz_c_other->Info01 = ISpec;
			zz_c_other->Info02 = ISender;
		  } else {
			t = TIME_inf;
		  }
                } else { /* CLEAR */
                  if (ISender == NULL) {
                        t = Time;
                        zz_c_other->Info01 = NULL;
                        zz_c_other->Info02 = (void*) this;
                  } else {
                        t = TIME_inf;
                  }
                }
#if     ZZ_TAG
		if (zz_c_first_wait ||
		    def (t) && (((q = zz_c_wait_tmin . cmp (t, tag)) > 0) ||
			(q == 0 && FLIP))) {
#else
		if (zz_c_first_wait ||
		    def (t) && (zz_c_wait_tmin > t || FLIP)) {
#endif
			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = ev;
#if  ZZ_TAG
			zz_c_wait_tmin . set (t, tag);
			zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
			zz_c_wait_event -> waketime = zz_c_wait_tmin = t;
#endif
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = zz_c_other -> Info01;
			zz_c_wait_event -> Info02 = zz_c_other -> Info02;

			if (!zz_c_first_wait)
				zz_c_wait_event -> reschedule ();
		}
		break;
	     default:
		excptn ("Process->wait: %s, illegal event %1d", getSName (),
			ev);
	  }
	}

	if (zz_c_first_wait) {
		zz_c_whead = zz_c_other;
		zz_c_first_wait = NO;

		if (def (t))
			zz_c_wait_event->enqueue ();
		else
			zz_c_wait_event->store ();
	}

#if     ZZ_TAG
	zz_c_other -> when . set (t, tag);
#else
	zz_c_other -> when = t;
#endif
	zz_c_other -> event = zz_c_wait_event;
	zz_c_other -> other = zz_c_whead;
}

int Process::nwait () {
//
// The number of events awaited by the process
//
	ZZ_REQUEST *rq, *re;
	ZZ_EVENT *ev;
	int n;

	if (TheProcess == this) {
		// Self query
		if (zz_c_first_wait)
			return 0;
		rq = zz_c_whead;
	} else {
		// have to look up the event
		for (ev = zz_eq; ev != zz_sentinel_event; ev = ev -> next) {
			if (ev->process == this) {
				rq = ev->chain;
				break;
			}
		}
		if (ev == zz_sentinel_event)
			// Impossible
			return 0;
	}

	re = rq;

	do {
		n++;

	} while ((rq = rq->other) != re);

	return n;
}

int Process::signal (void *spec) {

/* ------------------------------------- */
/* Sends a regular signal to the process */
/* ------------------------------------- */

	ZZ_REQUEST *rq;
	ZZ_EVENT *e;
	int na;
#if ZZ_TAG
	int q;
#endif
	if (ISender != NULL) return (REJECTED);

	ISender = TheProcess;
	ISpec = spec;

	for (na = 0, rq = TWList; rq != NULL; rq = rq->next) {
		if (rq->event_id == SIGNAL) {
			na++;
			// Note: Info01 and Info02 are set upon delivery
#if     ZZ_TAG
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
	return (na ? ACCEPTED : QUEUED);
};

int Process::signalP (void *spec) {

/* -------------------------------------- */
/* Sends a priority signal to the process */
/* -------------------------------------- */

	ZZ_REQUEST *rq, *rs;

	rs = NULL;

	// The process must be waiting for the signal
	for (rq = TWList; rq != NULL; rq = rq -> next) {
	    if (rq -> event_id == SIGNAL) {
		Assert (rs == NULL,
		    "Process->signalP: %s, more than one wait request",
			getSName ());
		rs = rq;
	    }
	}
		
	if (rs == NULL) return (REJECTED);

#if  ZZ_TAG
	rs -> when . set (Time);
#else
	rs -> when = Time;
#endif
	assert (zz_pe == NULL,
		"Process->signalP: %s, more than one pending priority event",
			getSName ());

	zz_pe = (zz_pr = rs) -> event;

	assert (ISender == NULL,
		"Process->signalP: %s, signal repository not empty",
			getSName ());

	ISender = TheProcess;
	ISpec = spec;

	return (ACCEPTED);
};

int Process::isSignal () {

/* ---------------------------------------- */
/* Checks if a pending interrupt is present */
/* ---------------------------------------- */

	if (ISender == NULL) return (NO);
	Info01 = ISpec;
	Info02 = ISender;
	return (YES);
};

int Process::erase () {

/* -------------------------- */
/* Erases a pending interrupt */
/* -------------------------- */

        ZZ_REQUEST *rq;
        ZZ_EVENT *e;
#if ZZ_TAG
        int q;
#endif
	if ((Info02 = ISender) == NULL) {
          Info01 = NULL; 
        } else {
          Info01 = ISpec;
          for (rq = TWList; rq != NULL; rq = rq->next) {
            if (rq->event_id == CLEAR) {
               // Note: Info01 and Info02 are set upon delivery
#if     ZZ_TAG
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
        }
	ISender = NULL;
	return (Info02 != NULL);
};

int     zz_noevents () {

/* -------------------------------------------------------- */
/* Processes the case when the simulator runs out of events */
/* -------------------------------------------------------- */

#if ZZ_REA
        // This is useless in the real mode
        return 1;
#else
        ZZ_REQUEST *cw;
        int found;
        // Check if somebody has been waiting for this
        for (found = 0, cw = Kernel->TWList; cw != NULL; cw = cw->next) {
          if (cw->event_id == STALL) {
             found = 1;
#if ZZ_TAG
             cw->when . set (Time);
#else
             cw->when = Time;
#endif
             // We know that this one is going to be on top no matter what
             cw->event->new_top_request (cw);
          }
        }
        if (found) return 0;
        // Nobody waiting - just quit
	zz_exit_code = EXIT_noevents;
	Kernel->terminate ();
        return 1;
#endif
}

void terminate (Process *p) {

/* ---------------------------------------------------------- */
/* Stops   and  destroys  the  process.  Wakes  up  processes */
/* awaiting its termination                                   */
/* ---------------------------------------------------------- */

ZZ_EVENT   *ev, *eh;
ZZ_REQUEST *cw, *rq, *rc;
#if ZZ_TAG
	int q;
#endif
	if (p == Kernel) {
		// Terminates the simulation
		if (ZZ_Kernel == NULL || zz_ofpp == NULL) {
                  cdebug ("Terminating in kernel 1");
                  zz_done ();
                }
		// Avoid it second time
		ZZ_Kernel = NULL;

		// Wake up processes waiting for the Kernel's termination
		for (cw = p->TWList; cw != NULL; cw = cw -> next) {
			if (cw -> event_id == DEATH) {
				cw -> Info01 = (void*) zz_exit_code;
				cw -> Info02 = NULL;
#if     ZZ_TAG
				cw->when . set (Time);
				if ((q = (ev = cw->event)->waketime .
				  cmp (cw->when)) > 0 || (q == 0 && FLIP))
#else
				cw->when = Time;
				if ((ev = cw->event)->waketime > Time || FLIP)
#endif
						ev->new_top_request (cw);
			}
		}

		// Remove all events and requests besides the processes waiting
		// for system termination

		for (ev = zz_eq; ev != zz_sentinel_event; ) {

			if (ev->chain == NULL) {
				// Remove without any further checks
				eh = ev -> next;
				ev -> cancel ();
				delete ev;
				ev = eh;
				continue;
			}

			for (rq = ev->chain; ;) {
				if (rq->ai == Kernel && rq->event_id == DEATH)
					goto HOLD;
				if ((rq = rq->other) == ev->chain) break;
			}

			// Irrelevant -- deallocate

			eh = ev -> next;
			ev -> cancel ();
			delete ev;
			ev = eh;
			continue;

HOLD:                   // Remove all requests except for the Kernel's DEATH

			Assert (rq->when == Time,
		"Kernel->terminate: internal error -- illegal restart time");
			ev->new_top_request (rq);
			for (rq = (rc = ev->chain)->other; rq != ev->chain; ) {
				if (rq->ai == Kernel && rq->event_id == DEATH) {
					rq = (rc = rq)->other;
					continue;
				}
				rc->other = rq->other;
				pool_out (rq);
				delete (rq);
				rq = rc->other;
			}

			ev = ev -> next;
		}

		// Process the termination request
		setLimit (MAX_Long, TIME_inf, HUGE); // Reset limits to infinity

		// Make sure the main loop has something to deallocate
		// Not needed any more
		// zz_CE = new ZZ_EVENT ();
		if (zz_setjmp_done) longjmp (zz_mloop, NONE);
		// Force unconditional termination
                cdebug ("Terminating in kernel 2");
		zz_done ();
		// Never returns
	}

	// Regular process: flag == regular (soft) termination 

	p->ISpec = NULL;
	delete (p);
}

/*
 * Here is the set of funtions to implement Station::terminate (). We need a
 * recursive traverser of the ownership tree.
 */
class	pttr_st_c {

// This is a recursive traverser of the ownership tree to locate processes with
// some properties and store their pointers in a table. Needed by
// Station::terminate and getproclist.

	friend void zz_pcslook (ZZ_Object*);

	Station	*OW;
	void	*TID;
	Boolean	Extend, Internal;

	public:

	Process	**PList;	// Process list
	int	PLL,		// Current length
		PLS;		// Size

	pttr_st_c (Station *ow, void *tid = NULL, Process **pl = NULL,
								Long sz = 0) {
		OW = ow;
		TID = tid;
		PLL = 0;

		if (pl == NULL) {
			Internal = YES;
			if (sz > 0) {
				PLS = sz;
				Extend = NO;
			} else {
				Extend = YES;
				PLS = 16;
			}
			PList = new Process* [PLS];
		} else {
			Internal = NO;
			Extend = NO;
			if (sz <= 0)
				sz = 1;
			PList = pl;
			PLS = (sz < 1) ? 1 : sz;
		}
	};

	~pttr_st_c () {
		if (Internal)
			delete [] PList;
	};

	inline Boolean add (Process *p) {

		Process **pl;
		int i;

		if (PLL == PLS) {
			if (!Extend)
				return NO;
			// Grow the array
			pl = new Process* [PLS += PLS];
			for (i = 0; i < PLL; i++)
				pl [i] = PList [i];
			delete [] PList;
			PList = pl;
		}

		PList [PLL++] = p;
		return YES;
	};
};

static pttr_st_c *pttr_st;

void zz_pcslook (ZZ_Object *o) {

// This is global to save on stack and time

    	ZZ_Object  *clist;

	if (o->Class == AIC_process) {
		if ((pttr_st->OW == NULL || ((Process*)o)->getOwner () ==
		  pttr_st->OW)
		   &&
		    (pttr_st->TID == NULL || ((Process*)o)->zz_typeid == 
		      pttr_st->TID)) {
			// Add this one
			if (pttr_st->add ((Process*)o) == NO)
				// No more
				return;
		}
		clist = ((Process*)o)->ChList;
	} else if (o->Class == OBJ_station) {
		// I don't think this is possible: aren't processes linked to
		// processes only? can they link stations?
		clist = ((Station*)o)->ChList;
	} else {
		return;
	}

	while (clist != NULL) {
		zz_pcslook (clist);
		clist = clist->next;
	}
}

void	Station::terminate () {
/*
 * This variant of terminate kills all the processes run by this station. The
 * killed processes trigger no events, and their dependants are deallocated.
 * The operation is a prerequisite to 'reset'.
 */
	ZZ_Object *ob;
	Process *p;
	ZZ_EVENT   *ev, *eh;
	ZZ_REQUEST *rq;
	int i;

	pttr_st = new pttr_st_c (this);
	zz_pcslook (zz_flg_started ? (ZZ_Object*)System : (ZZ_Object*)ZZ_Main);

	for (i = 0; i < pttr_st->PLL; i++) {
		p = pttr_st->PList [i];
		// Flag == hard termination: no events
		p->ISpec = p;
		delete p;
	}

	delete pttr_st;
}

Long zz_getproclist (Station *ow, void *pt, Process **list, Long len) {
/*
 * Produce the list of processes belonging to the given station and being
 * of the given type.
 */
	assert (list != NULL, "getproclist: the list pointer is NULL");

	pttr_st = new pttr_st_c (ow, pt, list, len);
	zz_pcslook (zz_flg_started ? (ZZ_Object*)System : (ZZ_Object*)ZZ_Main);
	len = pttr_st->PLL;
	delete pttr_st;

	return len;
}

Process::~Process () {
/*
 * Take care of your descendants and events when terminating. There is a 
 * difference between a regular termination and from Station::terminate.
 * In the latter case, we deallocate descendants and trigger no events.
 */
	ZZ_Object *ob;
	Process *f;
	ZZ_EVENT   *ev, *eh;
	ZZ_REQUEST *rq;
#if ZZ_TAG
	int q;
	TIME t;
#endif
	int dpc;


	// This will be used to replace the process pointer in those
	// places where this process pointer used to be present, and
	// which must linger beyond its termination
	f = Father == NULL ? Kernel : Father;

	if (ChList != NULL) {

		while (ChList != NULL) {
			ob = ChList;
			pool_out (ob);
			// This will make it temporarily disappear from the
			// display list, but it will soon reappear in the new
			// place.
			zz_DREM (ob);
			if (ob->Class == AIC_process) {
				// Children receive a new owner
				((Process*)ob)->Father = f;
			} else if (ISpec != NULL) {
				// ISpec is used as a flag (nonzero means that
				// we are being killed by Station::terminate).
				// In such a case, we deallocate non-process
				// descendants.
				if (ob->zz_nickname != NULL)
					delete [] ob->zz_nickname;
				delete ob;
				continue;
			}
			pool_in (ob, f->ChList);
		}
	}
	
	for (dpc = 0, ev = zz_eq; ev != zz_sentinel_event; ) {
		// Remove all events owned by this process; well, normally there
		// should be one
		if (ev -> process != this) {
			ev = ev -> next;
			continue;
		}
		eh = ev -> next;
		ev -> cancel ();
		delete ev;
		dpc++;
		ev = eh;
	}

	// If you remove this test, delete the variable (dpc) as well
	Assert (dpc < 2, "Process->terminate: multiple events %1d for this "
			"process", dpc);

	// Check if there is a signal reception pending from this process.
	// Note: it is enough to check events scheduled at this Time.

	ev = zz_eq;
	while (1) {
#if ZZ_TAG
		(ev->waketime).get (t);
		if (t > Time ||
#else
		if (ev->waketime > Time ||
#endif
		    ev == zz_sentinel_event)
			break;

		if ((rq = ev->chain) != NULL) {
			while (1) {
				if (rq->ai == this) {
					rq->ai = f;
					if (rq->Info02 == this)
						rq->Info02 = f;
				}
				if ((rq = rq->other) == ev->chain)
					break;
			}
			if (ev->ai == this) {
				ev->ai = f;
				if (ev->Info02 == this)
					ev->Info02 = f;
			}
		}
		ev = ev->next;
	}

	// Now take care of those still undelivered

	while (TWList != NULL) {

		rq = TWList;
		rq -> ai = f;
		if (rq->Info02 == this)
			rq->Info02 = f;

		if (rq->event->ai == this) {
			// In case this is the 'top' request
			rq->event->ai = f;
			if (rq->event->Info02 == this)
				rq->event->Info02 = f;
		}

		if (ISpec == NULL && rq->event_id == DEATH) {
			// Normal termination: wake up processes waiting for it
			rq -> Info01 = Info01;
			rq -> Info02 = Info02;
#if     ZZ_TAG
			rq->when . set (Time);
			if ((q = (ev = rq->event)->waketime.cmp (rq->when)) > 0
			    || (q == 0 && FLIP))
#else
			rq->when = Time;
			if ((ev = rq->event)->waketime > Time || FLIP)
#endif
				ev->new_top_request (rq);
		} 

		pool_out (rq);
		// Move these requests to the zombie list. They will be
		// deallocated when the respective processes get awakened.
		pool_in (rq, zz_orphans);
	}

	while (SWList != NULL) {
		// I almost forgot about these
		rq = SWList;
		pool_out (rq);
		pool_in (rq, zz_orphans);
	}

	if (ISpec == NULL && Father != NULL) {
		// Wake up the parent waiting for child's termination
		for (rq = Father->TWList; rq != NULL; rq = rq->next) {
			if (rq->event_id == CHILD) {
				rq->Info01 = Info01;
				rq->Info02 = Info02;
#if     ZZ_TAG
				rq->when . set (Time);
				if ((q = (ev = rq->event)->waketime .
				    cmp (rq->when)) > 0 || (q == 0 && FLIP))
#else
				rq->when = Time;
				if ((ev = rq->event)->waketime > Time || FLIP)
#endif
					ev->new_top_request (rq);
			}
		}
	}

	pool_out (this);
	zz_DREM (this);
	if (zz_nickname != NULL)
		delete [] zz_nickname;
	if (TheProcess == this)
		TheProcess = Kernel;
}

int Process::children () {

	int cc;
	ZZ_Object *de;

	for (cc = 0, de = ChList; de != NULL; de = de->next) {
		if (de->Class == AIC_process)
			cc++;
	}

	return cc;
}

sexposure (Process)

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr, (int) SId);    // Request queue

		sexmode (1)

			exPrint1 (Hdr);               // Wait list
	}

	sonscreen {

		sfxmode (0)

			exDisplay0 ((int) SId);       // Request queue

		sexmode (1)

			exDisplay1 ();                // Wait list
	}
}

void    Process::exPrint0 (const char *hdr, int sid) {

/* ----------------------- */
/* Print the request queue */
/* ----------------------- */

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;
	Long               l;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName ();
		if (Owner != NULL)
			Ouf << '/' << Owner->getOName ();
		Ouf << ") All processes waiting for " << getOName ();
		Ouf << ":\n\n";
	}

	if (isStationId (sid))
		Ouf << "           Time ";
	else
		Ouf << "       Time   St";

	Ouf << "    Process/Idn     PState      Event         AI/Idn" <<
		"      State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (e->chain == NULL || e->station == NULL ||
			isStationId (sid) && sid != e->station->Id) continue;

		for (r = e->chain;;) {

			if (r->ai == this) {

				if (!isStationId (sid))
					ptime (r->when, 11);
				else
					ptime (r->when, 15);

				if (pless (e->waketime, r->when))
					Ouf << ' ';     // Obsolete
				else if (e->chain == r)
					Ouf << '*';     // Uncertain
				else
					Ouf << '?';

				if (!isStationId (sid)) {
					// Identify the station
					Ouf << form ("%4d ",
				zz_trunc (ident (e->station), 3));
				}

				print (e->process->getTName (), 10);
				Ouf << form ("/%03d ",
					zz_trunc (e->process->Id, 3));

				print (zz_eid (r->event_id), 10);
				Ouf << ' ';

				print (e->process->zz_sn (r->pstate), 10);
				Ouf << ' ';

				print (e->ai->getTName (), 10);
				if ((l = e->ai->zz_aid()) != NONE)
					Ouf << form ("/%03d ",
						zz_trunc (l, 3));
				else
					Ouf << "     ";

				print (e->process->zz_sn (e->chain->pstate),10);
				Ouf << '\n';
			}

			if ((r = r->other) == e->chain) break;
		}
	}

	Ouf << "\n(" << getOName ();
	if (Owner != NULL)
		Ouf << '/' << Owner->getOName ();
	Ouf << ") End of list\n\n";
}

void    Process::exPrint1 (const char *hdr) {

/* ------------------- */
/* Print the wait list */
/* ------------------- */

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;
	Long               l;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName ();
		if (Owner != NULL)
			Ouf << '/' << Owner->getOName ();
		Ouf << ") Request list:\n\n";
	}

	Ouf << "        AI/Idn      Event       State            Time\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e -> next)
		if (e->process == this) break;

	if (e != zz_sentinel_event && (r = e->chain) != NULL) {

		while (1) {

			print (r->ai->getTName (), 10);
			if ((l = r->ai->zz_aid ()) != NONE)
				Ouf << form ("/%03d ", zz_trunc (l, 3));
			else
				Ouf << "     ";
			print (r->ai->zz_eid (r->event_id), 10);
			if (pless (e->waketime, r->when))
				Ouf << ' ';
			else if (r == e->chain)
				Ouf << '*';
			else
				Ouf << '?';
			Ouf << ' ';
			print (zz_sn (r->pstate), 10);
			Ouf << ' ';
			ptime (r->when, 15);
			Ouf << '\n';

			if ((r = r->other) == e->chain) break;
		}
	}

	Ouf << "\n(" << getOName ();
	if (Owner != NULL)
		Ouf << '/' << Owner->getOName ();
	Ouf << ") End of list\n\n";
}

void    Process::exDisplay0 (int sid) {

/* ------------------------- */
/* Display the request queue */
/* ------------------------- */

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;
	Long               l;

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (e->chain == NULL || e->station == NULL ||
			isStationId (sid) && sid != e->station->Id) continue;

		for (r = e->chain;;) {

			if (r->ai == this) {

				dtime (r->when);
				if (pless (e->waketime, r->when))
					display (' ');
				else if (e->chain == r)
					display ('*');
				else
					display ('?');

				if (!isStationId (sid))
				   display (ident (e->station));

				display (e->process->getTName ());
				display (e->process->Id);

				display (zz_eid (r->event_id));

				display (e->process->zz_sn (r->pstate));

				display (e->ai->getTName ());
				if ((l = e->ai->zz_aid ()) != NONE)
					display (l);
				else
					display (' ');

				display (e->process->zz_sn (e->chain->pstate));
			}
			if ((r = r->other) == e->chain) break;
		}
	}
}

void    Process::exDisplay1 () {

/* --------------------- */
/* Display the wait list */
/* --------------------- */

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;
	Long               l;

	for (e = zz_eq; e != zz_sentinel_event; e = e -> next)
		if (e->process == this) break;

	if (e != zz_sentinel_event && (r = e->chain) != NULL) {

		while (1) {

			display (r->ai->getTName ());
			if ((l = r->ai->zz_aid ()) != NONE)
				display (l);
			else
				display (' ');

			display (r->ai->zz_eid (r->event_id));
			if (pless (e->waketime, r->when))
				display (' ');
			else if (r == e->chain)
				display ('*');
			else
				display ('?');

			display (zz_sn (r->pstate));
			dtime (r->when);

			if ((r = r->other) == e->chain) break;
		}
	}
}

sexposure (ZZ_KERNEL)

/* ---------------------------------------------------------- */
/* The kernel exposure is used to display things not directly */
/* related to any specific AI                                 */
/* ---------------------------------------------------------- */

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr, (int) SId);  // Full request queue

		sexmode (1)

			exPrint1 (Hdr, (int) SId);  // Abbreviated request queue

		sexmode (2)

			exPrint2 (Hdr);             // Status
	}

	sonscreen {


		sfxmode (0)

			exDisplay0 ((int) SId);     // Full request queue

		sexmode (1)

			exDisplay1 ((int) SId);     // Abbreviated request queue

		sexmode (2)

			exDisplay2 ();              // Simulation status

		sexmode (3)

			exDisplay3 ();          // Last event
	}
}

void    ZZ_KERNEL::exPrint2 (const char *hdr) {

/* --------------------------- */
/* Print the simulation status */
/* --------------------------- */

#if	ZZ_NOC
	TIME    dt;
#endif

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << "(Kernel) Simulation status:\n\n";
	}

	print (zz_processId,  "Smurph process id:              ");
	print (cpuTime ()   , "CPU execution time:             ");
	if (Etu == 1.0) {
		print (Time         , "Simulated time:                 ");
	} else {
		print (Time         , "Simulated time (ITU):           ");
		Ouf << ::form (       "Simulated time (ETU):           %8.6f\n",
			ituToEtu (Time));
	}
	print (zz_npre      , "Number of processed events:     ");
	print (zz_npee      , "Number of pending events:       ");
#if	ZZ_NOC
	print (zz_NGMessages, "Number of generated messages:   ");
	print (zz_NRMessages, "Number of received messages:    ");
	print (zz_NQBits    , "Number of queued bits:          ");
#endif
#if	ZZ_NOC
	dt = Time - zz_GSMTime;
	print (Etu * (double)zz_NRBits/((dt == TIME_0) ? TIME_1 : dt) ,
		"Global throughput:              ");
#endif

	print ("Last station run:               ", 32);
	if (TheStation == NULL)
		print ("System", 15);
	else
		print (TheStation->getOName (), 15);
	Ouf << '\n';

	print ("Process:                        ", 32);
	if (TheProcess == NULL)
		print ("unknown", 15);
	else
		print (TheProcess->getOName (), 15);
	Ouf << '\n';

	print ("Awakened by:                    ", 32);
	if (zz_CE == NULL)
		print ("unknown AI", 15);
	else
		print (zz_CE->ai->getOName (), 15);
	Ouf << '\n';

	print ("Event:                          ", 32);
	if (zz_CE == NULL)
		print (zz_event_id, 15);
	else
		print (zz_CE->ai->zz_eid ((LPointer) zz_event_id), 15);
	Ouf << '\n';

	print ("Restarted at state:             ", 32);
	if (zz_CE == NULL)
		print (TheState, 15);
	else
		print (TheProcess->zz_sn (TheState), 15);
	Ouf << '\n';

	Ouf << "\n(Kernel) End of list\n\n";
}

void    ZZ_KERNEL::exPrint0 (const char *hdr, int sid) {

/* --------------------------------------------- */
/* Print the global request queue (full version) */
/* --------------------------------------------- */

	ZZ_REQUEST                      *r;
	ZZ_EVENT                        *e;
	Long                            l, i;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << "(Kernel) Full event queue";
		if (isStationId (sid))
			Ouf << " for " << idToStation (sid)->getOName ();
		Ouf << ":\n\n";
	}

	if (isStationId (sid))
		Ouf << "     ";
	else
		Ouf << " Stat";

	Ouf << "     Process/Idn               Time           AI/Idn"
		<< "      Event      State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && ident (e->station) != sid) continue;

		if ((r = e->chain) == NULL) {
			// A system event
			if (zz_flg_nosysdisp) continue;
			if (!isStationId (sid)) {
				print ("Sys", 5);
				Ouf << ' ';
				print (e->process->getTName (), 11);
			} else {
				print (e->process->getTName (), 17);
			}

			Ouf << form ("/%03d ",
					zz_trunc (e->process->Id, 3));

			if (def (e->waketime)) {
				ptime (e->waketime, 18);
				Ouf << '*';
			} else {
				print ("undefined", 18);
				Ouf << ' ';
			}

			Ouf << ' ';
			print (e->ai->getTName (), 11);
			if ((l = e->ai->zz_aid ()) != NONE)
				Ouf << form ("/%03d ", zz_trunc (l, 3));
			else
				Ouf << "     ";
			print (e->ai->zz_eid (e->event_id), 10);
			Ouf << ' ';
			print (e->process->zz_sn (e->pstate), 10);
			Ouf << '\n';

		} else {

			if (e->station == System) continue;
			for (i = 0; ; i++) {

				if (i == 0) {   // The first time around

					if (!isStationId (sid)) {
						if ((l = ident (e->station)) <
							0)
							print ("Sys", 5);
						else
							print (zz_trunc (ident
								(e->station),
									5), 5);
						Ouf << ' ';
						print (e->process->getTName (),
							11);
					} else {
						print (e->process->getTName (),
							17);
					}

					Ouf << form ("/%03d ",
						zz_trunc (e->process->Id, 3));
				} else {

					Ouf << "                      ";
				}

				if (def (r->when)) {
					ptime (r->when, 18);
					if (pless (e->waketime, r->when))
						Ouf << ' ';
					else if (e->chain == r)
						Ouf << '*';
					else
						Ouf << '?';
				} else {
					print ("undefined", 18);
					Ouf << ' ';
				}

				Ouf << ' ';
				print (r->ai->getTName (), 11);
				if ((l = r->ai->zz_aid ()) != NONE)
					Ouf << form ("/%03d ", zz_trunc (l, 3));
				else
					Ouf << "     ";
				print (r->ai->zz_eid (r->event_id), 10);
				Ouf << ' ';
				print (e->process->zz_sn (r->pstate), 10);
				Ouf << '\n';

				if ((r = r->other) == e->chain) break;
			}
		}
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    ZZ_KERNEL::exPrint1 (const char *hdr, int sid) {

/* ---------------------------------------------------- */
/* Print the global request queue (abbreviated version) */
/* ---------------------------------------------------- */

	ZZ_EVENT                        *e;
	Long                            l;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << "(Kernel) Abbreviated event queue";
		if (isStationId (sid))
			Ouf << " for " << idToStation (sid)->getOName ();
		Ouf << ":\n\n";
	}

	if (isStationId (sid))
		Ouf << "     ";
	else
		Ouf << " Stat";

	Ouf << "     Process/Idn                Time          AI/Idn"
		<< "      Event      State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && ident (e->station) != sid) continue;
		if ((e->chain == NULL || e->station == System) &&
			zz_flg_nosysdisp) continue;

		if (!isStationId (sid)) {
			if (e->chain == NULL || ident (e->station) < 0)
				print ("Sys", 5);
			else
				print (zz_trunc (ident (e->station), 5), 5);

			Ouf << ' ';
			print (e->process->getTName (), 11);
		} else {
			print (e->process->getTName (), 17);
		}

		Ouf << form ("/%03d ", zz_trunc (e->process->Id, 3));

		ptime (e->waketime, 19);

		Ouf << ' ';
		print (e->ai->getTName (), 11);
		if ((l = e->ai->zz_aid ()) != NONE)
			Ouf << form ("/%03d ", zz_trunc (l, 3));
		else
			Ouf << "     ";

		print (e->ai->zz_eid (e->event_id), 11); Ouf << ' ';

		if (e->chain == NULL)
			print (e->process->zz_sn (e->pstate), 10);
		else
			print (e->process->zz_sn (e->chain->pstate), 10);

		Ouf << '\n';
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    ZZ_KERNEL::exDisplay2 () {

/* ----------------------------- */
/* Display the simulation status */
/* ----------------------------- */

#if	ZZ_NOC
	TIME    dt;
#endif

	display (zz_processId );
	display (cpuTime ()   );
	display (Time         );
	display (zz_npre      );
	display (zz_npee      );
#if	ZZ_NOC
	display (zz_NGMessages);
	display (zz_NRMessages);
	display (zz_NQBits    );
#else
	display (0);
	display (0);
	display (0);
#endif
#if	ZZ_NOC
	dt = Time - zz_GSMTime;
	display (Etu * (double)zz_NRBits/((dt == TIME_0) ? TIME_1 : dt));
#else
	display (0);
#endif
}


void    ZZ_KERNEL::exDisplay0 (int sid) {

/* ----------------------------------------------- */
/* Display the global request queue (full version) */
/* ----------------------------------------------- */

	ZZ_REQUEST                      *r;
	ZZ_EVENT                        *e;
	Long                            l, i;

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && ident (e->station) != sid) continue;

		if ((r = e->chain) == NULL) {
			if (zz_flg_nosysdisp) continue;
			// A system event
			if (!isStationId (sid))
				display ("Sys");

			display (e->process->getTName ());
			display (e->process->Id);

			if (def (e->waketime)) {
				display (e->waketime);
				display ('*');
			} else {
				display ("undefined");
				display (' ');
			}

			display (e->ai->getTName ());
			if ((l = e->ai->zz_aid ()) != NONE)
				display (l);
			else
				display (' ');

			display (e->ai->zz_eid (e->event_id));
			display (e->process->zz_sn (e->pstate));

		} else {

			if (e->station == System) continue;
			for (i = 0; ; i++) {

				if (i == 0) {   // The first time around

					if (!isStationId (sid)) {
						if ((l = ident (e->station)) <
							0)
							display ("Sys");
						else
						   display (ident (e->station));

					}

					display (e->process-> getTName ());
					display (e->process->Id);

				} else {

					if (!isStationId (sid))
						display (" ");
					display (" ");
					display (" ");
				}

				if (def (r->when)) {
					dtime (r->when);
					if (pless (e->waketime, r->when))
						display (' ');
					else if (e->chain == r)
						display ('*');
					else
						display ('?');
				} else {
					display ("undefined");
					display (' ');
				}

				display (r->ai->getTName ());
				if ((l = r->ai->zz_aid ()) != NONE)
					display (l);
				else
					display (' ');

				display (r->ai->zz_eid (r->event_id));
				display (e->process->zz_sn (r->pstate));
				if ((r = r->other) == e->chain) break;
			}
		}
	}
}

void    ZZ_KERNEL::exDisplay1 (int sid) {

/* ------------------------------------------------------ */
/* Display the global request queue (abbreviated version) */
/* ------------------------------------------------------ */

	ZZ_EVENT                        *e;
	Long                            l;

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && ident (e->station) != sid) continue;
		if ((e->chain == NULL || e->station == System) &&
			zz_flg_nosysdisp) continue;

		if (!isStationId (sid)) {
			if (e->chain == NULL || ident (e->station) < 0)
				display ("Sys");
			else
				display (ident (e->station));

		}

		display (e->process->getTName ());
		display (e->process->Id);

		dtime (e->waketime);

		display (e->ai->getTName ());
		if ((l = e->ai->zz_aid ()) != NONE)
			display (l);
		else
			display (' ');

		// display (e->ai->zz_eid (e->event_id));

		if (e->chain == NULL)
			display (e->process->zz_sn (e->pstate));
		else
			display (e->process->zz_sn (e->chain->pstate));
	}
}

void    ZZ_KERNEL::exDisplay3 () {

/* ------------------------------------ */
/* Display information about last event */
/* ------------------------------------ */

	if (TheStation == NULL)
		display ("none");
	else
		display (TheStation->getOName ());

	if (TheProcess == NULL)
		display ("????");
	else
		display (TheProcess->getOName ());

	if (zz_CE == NULL)
		display ("????");
	else
		display (zz_CE->ai->getOName ());

	if (zz_CE == NULL)
		display (zz_event_id);
	else
		display (zz_CE->ai->zz_eid ((LPointer) zz_event_id));

	if (zz_CE == NULL)
		display (TheState);
	else
		display (TheProcess->zz_sn (TheState));
}

void    zz_print_event_info (ostream &os) {

/* -------------------------------------------------- */
/* Prints out information about the last event/status */
/* -------------------------------------------------- */

#if	ZZ_NOC
	TIME    dt;
#endif
	os    << ">>> CPU execution time       = " << cpuTime () << '\n';
	os    << ">>> Events processed so far  = " << zz_npre << '\n';
	os    << ">>> Pending events           = " << zz_npee << '\n';
#if	ZZ_NOC
	os    << ">>> Generated messages       = " << zz_NGMessages << '\n';
	os    << ">>> Received messages        = " << zz_NRMessages << '\n';
	os    << ">>> Queued bits              = " << zz_NQBits << '\n';
#endif
#if	ZZ_NOC
	dt = Time - zz_GSMTime;
	os    << ">>> Global throughput        = " <<
		((double)zz_NRBits/((dt == TIME_0) ? TIME_1 : dt)) << '\n';
#endif
	os    << ">>> Last station run         = ";

	if (TheStation == NULL)
		os << "System";
	else
		os << TheStation->getOName ();
	os    << '\n';

	os    << ">>> Process                  = ";

	if (TheProcess == NULL)
		os << "unknown";
	else
		os << TheProcess->getOName ();

	os    << '\n';

	if (zz_CE != NULL && zz_CE->waketime == Time) {

		os    << ">>> Awakened by              = ";
		os << zz_CE->ai->getOName ();
		os << '\n';

		os    << ">>> Event                    = ";
		os << zz_CE->ai->zz_eid ((LPointer) zz_event_id);
		os    << '\n';

		os    << ">>> Restarted at state       = ";
		os << TheProcess->zz_sn (TheState);
	
		os    << '\n';
	}
}

#if     ZZ_DBG

static  int     header_printed = NO;

void    zz_print_debug_info () {

/* ------------------------------------------------- */
/* Prints out debug information about the last event */
/* ------------------------------------------------- */

	Long    l;
	const char *s;

	if (zz_flg_nosysdisp && (zz_CE == NULL ||
		TheStation == System || zz_CE->chain == NULL)) return;
	if (!header_printed) {
		header_printed = YES;
		Ouf <<
"              Time          AI/Idn      Event Station    Process/Idn      State\n";
	}
	print (Time, 18); Ouf << ' ';
	if (zz_CE == NULL)
		print ("unknown    ", 15);
	else {
		print (zz_CE->ai->getTName (), 11);
		if ((l = zz_CE->ai->zz_aid ()) != NONE)
			print (form ("/%03d", zz_trunc (l, 3)), 4);
		else
			Ouf << "    ";
	}

	Ouf << ' ';
	if (zz_CE == NULL)
		print (zz_event_id, 10);
	else
		print (zz_CE->ai->zz_eid (zz_event_id), 10);
	Ouf << ' ';
	if (TheStation == NULL || (l = ident (TheStation)) == NONE)
		print ("System", 7);
	else
		print (zz_trunc (l, 7), 7);
	Ouf << ' ';
	if (TheProcess == NULL) {
		print ("unknown", 10);
		print ("    ");
	} else {
		if ((s = TheProcess->getNName ()) == NULL) {
			print (TheProcess->getTName (), 10);
			print (form ("/%03d", zz_trunc (ident (TheProcess), 3)),
				4);
		} else
			print (s, 14);
	}
	Ouf << ' ';
	if (zz_CE == NULL)
		print (TheState, 10);
	else
		print (TheProcess->zz_sn (TheState), 10);
	Ouf << '\n';

#if ZZ_NOL || ZZ_NOR
	if (DebugTracingFull) {
	    int     i;
	    char    hdr [256];
#endif

#if ZZ_NOL
	    {
		// Link dumps -- makes no sense if there are no links
		for (i = 0; i < NLinks; i++) {
			strcpy (hdr, form ("Activities in %s:",
				idToLink (i)->getOName ()));
			idToLink (i)->printAct (hdr);
		}
	    }
#endif

#if ZZ_NOR
	    {
		// RFChannel dumps
		for (i = 0; i < NRFChannels; i++) {
			strcpy (hdr, form ("Activities in %s:",
				idToRFChannel (i)->getOName ()));
			idToRFChannel (i)->printRAct (hdr);
		}
	    }
#endif

#if ZZ_NOL || ZZ_NOR
	}
#endif

}
#endif
