/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-08   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

/* -------------- */
/* The Mailbox AI */
/* -------------- */

#include        "system.h"
#include	"cdebug.h"

Mailbox::~Mailbox () {
  if (nextm != (Mailbox*)NONE)
    excptn ("Mailbox: %s, once created, a station mailbox cannot be destroyed",
	getSName ());

#if ZZ_REA || ZZ_RSY
  if (sfd != NONE)
    // This is a bound mailbox
    destroy_bound ();
  else
#endif
  {
    ZZ_QITEM *c, *d;
    for (c = head; c != NULL; c = d) {
      d = c->next;
      delete c;
    }
    trigger_all_events ();
  }

  pool_out (this);
  zz_DREM (this);
  if (zz_nickname != NULL) delete (zz_nickname);
}

void Mailbox::trigger_all_events () {
/*
 * When we destroy a mailbox, we wake up all processes waiting on it - to let
 * them know about this rather important occurrence.
 */
	ZZ_REQUEST 	*rq;
	ZZ_EVENT	*ev;

#if 	ZZ_TAG
	int		q;
#endif

	while (WList != NULL) {
		rq = WList;
		// TheMailbox = NULL; this will let them know that the mailbox
		// is no more
		rq->Info01 = NULL;
		rq->Info02 = NULL;
#if     ZZ_TAG
		rq->when . set (Time);
		if ((q = (ev = rq->event)->waketime.cmp (rq->when)) > 0
		    || (q == 0 && FLIP))
#else
		rq->when = Time;
		if ((ev = rq->event)->waketime > Time || FLIP)
#endif
			ev->new_top_request (rq);

		pool_out (rq);
		// Move these requests to the zombie list. They will be
		// deallocated when the respective processes get awakened.
		pool_in (rq, zz_orphans);
	}
}

Mailbox::Mailbox (Long lm, const char *nn) {

/* ---------------------------------------------------------- */
/* Mailbox  constructor,  especially  useful for (semi)static */
/* declarations within stations                               */
/* ---------------------------------------------------------- */

Mailbox *p;

	Class = AIC_mailbox;
#if  ZZ_REA || ZZ_RSY
        csd = sfd = NONE;
#endif
	count = 0;
	head = tail = NULL;
	WList = NULL;

	limit = lm;

	if (zz_flg_started) {
          // Ignore this constructor if we are not in the first state
          // of root. Note: we cannot have a static declaration of the
          // mailbox in these circumstances.
          nextm = (Mailbox*)NONE;
          // This is different from NULL. We will use it to tell the
          // difference between a station mailbox (defined in the
          // network initialization phase) and a dynamically created
          // one.
        } else {
	  nextm = NULL;
	  if (nn != NULL) {
		// Nickname specified
		zz_nickname = new char [strlen (nn) + 1];
		strcpy (zz_nickname, nn);
	  }
	  // Add to the temporary list
	  if (zz_ncmailbox == NULL)
		zz_ncmailbox = this;
	  else {
		// Find the end of the list
		for (p = zz_ncmailbox; p->nextm != NULL; p = p->nextm);
		p->nextm = this;
	  }
        }
	// Add to the ownership tree
	pool_in (this, TheProcess->ChList);
}

void    Mailbox::zz_start () {

/* -------------------------------------------- */
/* Nonstandard constructor (called from create) */
/* -------------------------------------------- */

	int     srn;
	Mailbox *m;

	if (zz_flg_started) {
          // This is a dynamic mailbox -- we don't link it to the station
          Id = sernum++;
          // The rest will be done by the constructor
          return;
        }

	// Add the mailbox at the end of list at the current station

	Assert (TheStation != NULL, "Mailbox create: TheStation undefined");

	Assert (zz_ncmailbox == this,
		"Mailbox create: internal error -- initialization corrupted");

	// Remove from the temporary list
	zz_ncmailbox = NULL;
	nextm = NULL;

	if ((m = TheStation->Mailboxes) == NULL) {
		TheStation->Mailboxes = this;
		srn = 0;
	} else {
		// Find the end of the station's mailbox list
		for (srn = 1, m = TheStation->Mailboxes; m -> nextm != NULL;
			m = m -> nextm, srn++);
		m -> nextm = this;
	}
	// Put into Id the station number combined with the mailbox number
	// to be used for display and printing
	Id = (TheStation->Id & 0177777) << 16 | srn;
}

void    Mailbox::setup (Long limit) {

/* --------------------------- */
/* The standard setup function */
/* --------------------------- */

	count = 0;
	setLimit (limit);
}

inline int Mailbox::checkImmediate (Long ev) {

/* ------------------------------------------ */
/* Internal: checks if the event is immediate */
/* ------------------------------------------ */

  int cnt;

  if (limit < 0)
	// No immediate events on such a mailbox
	return NO;

  switch (ev) {

    case NONEMPTY:

      if (count) {
#if ZZ_REA || ZZ_RSY
        if (sfd != NONE)
          zz_c_other->Info01 = (void*) ibuf [iout];
        else
#endif
        if (head != NULL)
          zz_c_other->Info01 = head->item;
        else
          zz_c_other->Info01 = NULL;
        return YES;
      }
      return NO;

    case RECEIVE:

      // For RECEIVE, Info01 is set by main
      return (count > 0);

    case SENTINEL:

#if ZZ_REA || ZZ_RSY
      if (sfd != NONE) {
        if (cnt = sentinelFound ()) {
          zz_c_other->Info01 = (void*) cnt;
          return YES;
        }
        // Return YES also when the buffer is full. The sentinel will never
        // arrive until the mailbox is emptied.
        if (count == limit) {
          zz_c_other->Info01 = (void*) 0;
          return YES;
        }
      }
#endif
      return NO;

    default:

#if ZZ_REA || ZZ_RSY
      if (sfd == NONE) {
        if (count == ev) {
          zz_c_other->Info01 = (void*) count;
          return YES;
        }
      } else {
        if (ev >= 0 && count >= ev) {
          zz_c_other->Info01 = (void*) count;
          return YES;
        }
      }
#else
      if (count == ev) {
        zz_c_other->Info01 = (void*) count;
        return YES;
      }
#endif
      return NO;
  }
}

#if  ZZ_TAG
void    Mailbox::wait (Long ev, int pstate, LONG tag) {
	int q;
#else
void    Mailbox::wait (Long ev, int pstate) {
#endif
/* -------------------- */
/* Mailbox wait request */
/* -------------------- */
    if_from_observer ("Mailbox->wait: called from an observer");
#if ZZ_ASR
    if (limit == 0) {
      assert (ev == NEWITEM,
	"Mailbox->wait: %s, event %1d illegal for capacity 0 mailbox",
	  getSName (), ev);
    } else if (limit > 0) {
      assert (ev >= RECEIVE, "Mailbox->wait: %s, illegal event %1d",
	getSName (), ev);
      assert (ev <= limit, "Mailbox->wait: %s, count (%1d) > limit (%1d)",
	getSName (), ev, limit);
    }
#endif
	if (zz_c_first_wait) {

		zz_c_other = NULL;    // No requests so far

		// Create new simulation event
		zz_c_wait_event = new ZZ_EVENT;
		zz_c_wait_event -> station = TheStation;
		zz_c_wait_event -> process = TheProcess;

		new ZZ_REQUEST (&WList, this, (LPointer) ev, pstate, NONE, NULL,
			(void*) this);

		assert (zz_c_other != NULL,
			"Mailbox->wait: internal error -- null request");

		zz_c_wait_event -> pstate = pstate;
		zz_c_wait_event -> ai = this;
		zz_c_wait_event -> event_id = (LPointer) ev;
		zz_c_wait_event -> chain = zz_c_other;

		zz_c_whead = zz_c_other;

		zz_c_first_wait = NO;
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;
		zz_c_wait_event -> Info02 = (void*) this;

		// Determine if the event occurs immediately

		if (checkImmediate (ev)) {
                        zz_c_wait_event->Info01 = zz_c_other->Info01;
#if  ZZ_TAG
		        zz_c_wait_tmin . set (Time, tag);
			zz_c_other -> when =
			    zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
			zz_c_wait_event -> waketime =
			    zz_c_wait_tmin = zz_c_other -> when = Time;
#endif
			zz_c_wait_event->enqueue ();
		} else {
			// Item arrival time unknown
			zz_c_wait_event -> Info01 = NULL;
#if  ZZ_TAG
			zz_c_wait_tmin . set (TIME_inf, tag);
			zz_c_other -> when =
			    zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
			zz_c_wait_event -> waketime =
			    zz_c_wait_tmin = zz_c_other -> when = TIME_inf;
#endif
			zz_c_wait_event->store ();
		}

	} else {
		new ZZ_REQUEST (&WList, this, (LPointer) ev, pstate, NONE, NULL,
			(void*) this);
		assert (zz_c_other != NULL,
			"Mailbox->wait: internal error -- null request");

		if (checkImmediate (ev)) {
#if     ZZ_TAG
			zz_c_other -> when . set (Time, tag);
			if ((q = zz_c_wait_tmin . cmp (Time, tag)) > 0) {
#else
			zz_c_other -> when = Time;
			if (zz_c_wait_tmin > Time) {
#endif
			    zz_c_wait_event -> pstate = pstate;
			    zz_c_wait_event -> ai = this;
			    zz_c_wait_event -> event_id = (LPointer) ev;
#if     ZZ_TAG
			    zz_c_wait_event -> waketime = zz_c_wait_tmin = 
				zz_c_other -> when;
#else
			    zz_c_wait_event -> waketime = zz_c_wait_tmin = Time;
#endif
			    zz_c_wait_event -> chain = zz_c_other;
			    zz_c_wait_event -> Info01 = zz_c_other -> Info01;
			    zz_c_wait_event -> Info02 = (void*) this;
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
			    zz_c_wait_event -> event_id = (LPointer) ev;
			    zz_c_wait_event -> chain = zz_c_other;
			    zz_c_wait_event -> Info01 = zz_c_other -> Info01;
			    zz_c_wait_event -> Info02 = (void*) this;
			}
#endif

		} else
#if     ZZ_TAG
			zz_c_other -> when . set (TIME_inf, tag);
#else
			zz_c_other -> when = TIME_inf;
#endif

		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;
	}
}

Long    Mailbox::setLimit (Long lim) {

/* ------------------------- */
/* Sets the mailbox capacity */
/* ------------------------- */

	Long ol;


#if  ZZ_REA || ZZ_RSY
	Assert (sfd == NONE,
                "Mailbox->setLimit: %s, operation illegal on connected mailbox",
			getSName ());
#endif
	Assert (lim >= count || (lim < 0 && count == 0),
		"Mailbox->setLimit: %s, current count (%1d) > new limit (%1d)",
			getSName (), count, lim);

	if (limit < 0)
		Assert (lim < 0 || WList == NULL,
			"Mailbox->setLimit: %s, changing barrier to fifo while "
			"wait requests are pending", getSName ());
	else
		Assert (lim >= 0 || WList == NULL,
			"Mailbox->setLimit: %s, changing fifo to barrier while "
			"wait requests are pending", getSName ());
	
	ol = limit;
	limit = lim;
	return ol;
}

int     Mailbox::get () {

/* ----------------------- */
/* Default get (YES or NO) */
/* ----------------------- */

	ZZ_REQUEST *rq;
	ZZ_EVENT   *e;
#if ZZ_TAG
	int q;
#endif
	if (count) {
		count--;        // Decrement element count
#if  ZZ_REA || ZZ_RSY
		if (csd != NONE) {
                  // Update out for the record
                  if (++iout > limit) iout = 0;
                }
#endif
		for (rq = WList; rq != NULL; rq = rq -> next) {
		    // Restart processes waiting for this count
		    if (rq->event_id == GET || rq->event_id == count) {
#if     ZZ_TAG
			rq -> when . set (Time);
			if ((q = (e = rq->event) -> waketime . cmp (rq->when))
			  > 0 || (q == 0 && FLIP))
			    e -> new_top_request (rq);
#else
			rq -> when = Time;
			if ((e = rq->event) -> waketime > Time || FLIP)
			    e -> new_top_request (rq);
#endif
		    }
		}
		return (YES);
	}
	return (NO);
}

int     Mailbox::put () {

/* ----------- */
/* Default put */
/* ----------- */

	ZZ_REQUEST *rq;
	ZZ_EVENT   *e;
	int        na;
#if ZZ_TAG
	int q;
#endif

#if	ZZ_REA || ZZ_RSY
  if (sfd != NONE) {
    // We are dealing with a connected mailbox
    if (csd == NONE || (oin + 1) % (limit + 1) == oout) return REJECTED;
    // Send a zero item
    obuf [oin] = '\0';
    if (++oin > limit) oin = 0;
    return ACCEPTED;
  }
#endif
	if (limit > 0) {
		if (count < limit) {
			count++;
			for (na = 0, rq = WList; rq != NULL; rq = rq -> next) {
			    if (rq->event_id < GET || rq->event_id == count) {
				na++;
#if     ZZ_TAG
				rq -> when . set (Time);
				if ((q = (e = rq->event) -> waketime .
				  cmp (rq->when)) > 0 || (q == 0 && FLIP))
#else
				rq -> when = Time;
				if ((e = rq->event) -> waketime > Time ||
				  FLIP)
#endif
				    e -> new_top_request (rq);
			    }
			}
			return (na ? ACCEPTED : QUEUED);
		}
		return (REJECTED);
	}

	// We get here when limit <= 0

	for (na = 0, rq = WList; rq != NULL; rq = rq -> next) {
	    // On a barrier mailbox (limit < 0), default (argument-less)
	    // put restarts processes waiting for anything at all
	    if (limit < 0 || rq->event_id == NEWITEM) {
		na++;
#if     ZZ_TAG
		rq -> when . set (Time);
		if ((q = (e = rq->event) -> waketime .  cmp (rq->when)) > 0 ||
		  (q == 0 && FLIP))
#else
		rq -> when = Time;
		if ((e = rq->event) -> waketime > Time || FLIP)
#endif
		    e -> new_top_request (rq);
	    }
	}
	return (na ? ACCEPTED : REJECTED);
}

int     Mailbox::putP () {

/* -------------------- */
/* Default priority put */
/* -------------------- */

	ZZ_REQUEST *rq, *rs;

#if  ZZ_REA || ZZ_RSY
  Assert (sfd == NONE,
                "Mailbox->putP: %s, operation illegal on a connected mailbox",
			getSName ());
#endif
	rs = NULL;
	if (limit > 0) {
		Assert (count == 0,
			"Mailbox->putP: %s, mailbox is not empty (%1d)",
				getSName (), count);
		count++;

		// Somebody must be waiting for the item

		for (rq = WList; rq != NULL; rq = rq -> next) {
		    if (rq -> event_id < GET || rq->event_id == count) {
			Assert (rs == NULL,
			    "Mailbox->putP: %s, more than one recipient",
				getSName ());
			rs = rq;
		    }
		}
	} else {
		for (rq = WList; rq != NULL; rq = rq -> next) {
		    if (limit < 0 || rq -> event_id == NEWITEM) {
			Assert (rs == NULL,
			    "Mailbox->putP: %s, more than one recipient",
				getSName ());
			rs = rq;
		    }
		}
	}
		
	if (rs == NULL) return (REJECTED);
#if  ZZ_TAG
	rs -> when . set (Time);
#else
	rs -> when = Time;
#endif
	assert (zz_pe == NULL,
		"Mailbox->putP: %s, more than one pending priority event",
			getSName ());

	zz_pe = (zz_pr = rs) -> event;
	return (ACCEPTED);
}

int     Mailbox::erase () {

/* ----------------------------------------------------- */
/* Empties the mailbox (standard version, counters only) */
/* ----------------------------------------------------- */

	Long       ni;
	ZZ_REQUEST *rq;
	ZZ_EVENT   *e;
#if ZZ_TAG
	int q;
#endif

	ni = count;
	count = 0;

	if (limit >= 0) {
		// Nothing to do for a barrier mailbox
		for (rq = WList; rq != NULL; rq = rq -> next) {
		    if (rq -> event_id == 0 || rq -> event_id == GET) {
#if     ZZ_TAG
			rq -> when . set (Time);
			if ((q = (e = rq->event) -> waketime . cmp (rq->when)) >
			    0 || (q == 0 && FLIP))
#else
			rq -> when = Time;
			if ((e = rq->event) -> waketime > Time || FLIP)
#endif
				e -> new_top_request (rq);
		    }
		}
	}
#if  ZZ_REA || ZZ_RSY
	if (sfd != NONE) iin = iout = 0;
#endif
	return ni;
}

void    *Mailbox::zz_get () {

/* ---------------------- */
/* Subtype version of get */
/* ---------------------- */

	ZZ_REQUEST *rq;
	ZZ_EVENT   *e;
	ZZ_QITEM   *q;
#if ZZ_TAG
	int qq;
#endif
	if (count) {
		// This will never happen if limit < 0 (barrier mailbox)
		count--;        // Decrement element count
		for (rq = WList; rq != NULL; rq = rq -> next) {
		    // Restart processes waiting for this count
		    if (rq->event_id == GET || rq->event_id == count) {
			rq -> Info01 = NULL;
#if     ZZ_TAG
			rq -> when . set (Time);
			if ((qq = (e = rq->event) -> waketime . cmp (rq->when))
			  > 0 || (qq == 0 && FLIP))
#else
			rq -> when = Time;
			if ((e = rq->event) -> waketime > Time || FLIP)
#endif
			    e -> new_top_request (rq);
		    }
		}
#if  ZZ_REA || ZZ_RSY
                if (sfd != NONE) {
                  // We are dealing with a socket mailbox
                  zz_mbi = (void*) ((Long)(ibuf [iout]));
                  if (++iout > limit) iout = 0;
                  zz_outitem ();
                  return (zz_mbi);
                }
#endif
		assert (head != NULL,
			"Mailbox->get: %s, inconsistent item count",
				getSName ());
		zz_mbi = (q = head)->item;
		head = q->next;
		delete (q);
		zz_outitem ();
		return (zz_mbi);
	}
	return (NULL);
}

void    *Mailbox::zz_first () {

/* -------------------------------------------------- */
/* Subtype version of first (peeks at the first item) */
/* -------------------------------------------------- */

	if (count) {
#if  ZZ_REA || ZZ_RSY
                if (sfd != NONE) {
                  // This is a socket mailbox
                  zz_mbi = (void*) ((Long)(ibuf [iout]));
                  return (zz_mbi);
                }
#endif
		assert (head!=NULL,
			"Mailbox->first: %s, inconsistent item count",
				getSName ());
		return (head->item);
	}
	return (NULL);
}

int     Mailbox::zz_put (void *it) {

/* --------- */
/* Typed put */
/* --------- */

	ZZ_REQUEST *rq;
	ZZ_EVENT   *e;
	ZZ_QITEM   *q;
	int        na;
#if ZZ_TAG
	int qq;
#endif

#if     ZZ_REA || ZZ_RSY
  if (sfd != NONE) {
    // We are dealing with a connected mailbox
    if (csd == NONE || (oin + 1) % (limit + 1) == oout) return REJECTED;
    obuf [oin] = (char)(IPointer)it;
    if (++oin > limit) oin = 0;
    return ACCEPTED;
  }
#endif
	if (limit > 0) {
		if (count < limit) {
			zz_mbi = it;
			zz_initem ();
			count++;
			for (na = 0, rq = WList; rq != NULL; rq = rq -> next) {
			    if (rq->event_id < GET || rq->event_id == count) {
				rq -> Info01 = it;
				na++;
#if     ZZ_TAG
				rq -> when . set (Time);
				if ((qq = (e = rq->event) -> waketime .
				 cmp (rq->when)) > 0 || (qq == 0 && FLIP))
#else
				rq -> when = Time;
				if ((e = rq->event) -> waketime > Time ||
				  FLIP)
#endif
				    e -> new_top_request (rq);
			    }
			}
			q = new ZZ_QITEM (it);
			if (head == NULL)
				head = tail = q;
			else {
				tail->next = q;
				tail = q;
			}
			return (na ? ACCEPTED : QUEUED);
		}
		return (REJECTED);
	}

	// We get here when limit <= 0

	zz_mbi = it;	// We always succeed if limit == 0
	zz_initem ();

	if (limit < 0) {
		// Trigger mailbox
		for (na = 0, rq = WList; rq != NULL; rq = rq -> next) {
	    		if ((int)it == rq->event_id) {
				rq -> Info01 = it;
				na++;
#if     ZZ_TAG
				rq -> when . set (Time);
				if ((qq = (e = rq->event) ->
				  waketime . cmp (rq->when)) > 0 || (qq == 0
				    && FLIP))
#else
				rq -> when = Time;
				if ((e = rq->event) -> waketime > Time || FLIP)
#endif
		    			e -> new_top_request (rq);
	    		}
		}
	} else {
		for (na = 0, rq = WList; rq != NULL; rq = rq -> next) {
	    		if (rq -> event_id == NEWITEM) {
				rq -> Info01 = it;
				na++;
#if     ZZ_TAG
				rq -> when . set (Time);
				if ((qq = (e = rq->event) ->
				  waketime . cmp (rq->when)) > 0 || (qq == 0
				    && FLIP))
#else
				rq -> when = Time;
				if ((e = rq->event) -> waketime > Time || FLIP)
#endif
		    			e -> new_top_request (rq);
	    		}
		}
	}
	return (na ? ACCEPTED : REJECTED);
}

int     Mailbox::zz_putP (void *it) {

/* ------------------------------- */
/* Subtype version of priority put */
/* ------------------------------- */

	ZZ_REQUEST *rq, *rs;

#if  ZZ_REA || ZZ_RSY
  	Assert (sfd == NONE,
                "Mailbox->putP: %s, operation illegal on a connected mailbox",
			getSName ());
#endif
	zz_mbi = it;
	zz_initem ();

	rs = NULL;
	if (limit > 0) {
		Assert (count == 0,
			"Mailbox->putP: %s, mailbox is not empty (%1d)",
				getSName (), count);
		assert (head == NULL,
			"Mailbox->putP: %s, inconsistent item count",
				getSName ());
		count++;
		head = tail = new ZZ_QITEM (it);

		// Somebody must be waiting for the item

		for (rq = WList; rq != NULL; rq = rq -> next) {
		    if (rq -> event_id < GET || rq->event_id == count) {
			Assert (rs == NULL,
			    "Mailbox->putP: %s, more than one recipient",
				getSName ());
			rs = rq;
		    }
		}
	} else {
		for (rq = WList; rq != NULL; rq = rq -> next) {
		    if ((limit < 0 && rq->event_id == (int)it) || (limit == 0 &&
		      rq->event_id == NEWITEM)) {
			Assert (rs == NULL,
			    "Mailbox->putP: %s, more than one recipient",
				getSName ());
			rs = rq;
		    }
		}
	}
		
	if (rs == NULL) return (REJECTED);
#if  ZZ_TAG
	rs -> when . set (Time);
#else
	rs -> when = Time;
#endif
	rs -> Info01 = it;

	assert (zz_pe == NULL,
		"Mailbox->putP: %s, more than one pending priority event",
			getSName ());

	zz_pe = (zz_pr = rs) -> event;
	return (ACCEPTED);
}

Boolean Mailbox::zz_queued (void *el) {

	ZZ_QITEM *c;

	for (c = head; c != NULL; c = c->next)
		if (c->item == el)
			return YES;
	return NO;
}

int     Mailbox::zz_erase () {

/* ------------------------------------- */
/* Empties the mailbox (subtype version) */
/* ------------------------------------- */

	Long       ni;
	ZZ_QITEM   *c, *d;
	ZZ_REQUEST *rq;
	ZZ_EVENT   *e;
#if ZZ_TAG
	int q;
#endif

	ni = count;

	if (limit >= 0) {
#if  ZZ_REA || ZZ_RSY
        	if (sfd != NONE) {
			count = iin = iout = 0;
		} else {
#endif
			for (c = head; c != NULL; c = d) {
				d = c->next;
				zz_mbi = c->item;
				zz_outitem ();
				delete (c);
				count--;
			}
			head = NULL;
			assert (count == 0,
				"Mailbox->erase: %s, inconsistent item count",
					getSName ());
#if  ZZ_REA || ZZ_RSY
		}
#endif
		for (rq = WList; rq != NULL; rq = rq -> next) {
	    		if (rq -> event_id == 0 || rq -> event_id == GET) {
#if     ZZ_TAG
				rq -> when . set (Time);
				if ((q = (e = rq->event) ->
				  waketime . cmp (rq->when)) > 0 || (q == 0 &&
				    FLIP))
#else
				rq -> when = Time;
				if ((e = rq->event) -> waketime > Time || FLIP)
#endif
		    			e -> new_top_request (rq);
	    		}
		}
	}

	return ni;
}
		
sexposure (Mailbox)

/* ------------------------------------ */
/* Defines exposures for the Mailbox AI */
/* ------------------------------------ */

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr, (int) SId);

		sexmode (1)

			exPrint1 (Hdr, (int) SId);

		sexmode (2)

			exPrint2 (Hdr, YES);    // Contents

		sexmode (3)

			exPrint2 (Hdr, NO);     // No header line
	}

	sonscreen {

		sfxmode (0)

			exDisplay0 ((int) SId);

		sexmode (1)

			exDisplay1 ((int) SId);

		sexmode (2)

			exDisplay2 ();
	}
}

void    Mailbox::exPrint0 (const char *hdr, int sid) {

/* --------------------------- */
/* Print the full request list */
/* --------------------------- */

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;
	Long               l;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Full AI wait list";
		if (isStationId (sid))
			Ouf << " for " << idToStation (sid)->getOName ();
		Ouf << ":\n\n";
	}

	if (!isStationId (sid))
		Ouf << "       Time   St";
	else
		Ouf << "           Time ";

	Ouf << "    Process/Idn     MState         AI/Idn" <<
		"      Event      State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (e->chain == NULL || e->station == NULL) continue;
		if (isStationId (sid) && ident (e->station) != sid) continue;

		for (r = e->chain;;) {

			if (r->ai->Class == AIC_mailbox) {

				if (!isStationId (sid))
					ptime (r->when, 11);
				else
					ptime (r->when, 15);

				if (pless (e->waketime, r->when))
					Ouf << ' ';     // Obsolete
				else if (e->waketime == r->when)
					Ouf << '?';     // Uncertain
				else
					Ouf << '*';

				if (!isStationId (sid)) {
					// Identify the station
					if (ident (e->station) >= 0)
						Ouf << form ("%4d ",
					zz_trunc (ident (e->station), 3));
					else
						Ouf << " Sys ";
				}
				print (e->process->getTName (), 10);
				Ouf << form ("/%03d ",
					zz_trunc (e->process->Id, 3));
				// State name
				print (e->process->zz_sn (r->pstate), 10);

				Ouf << ' ';

				print (e->ai->getTName (), 10);
				if ((l = e->ai->zz_aid ()) != NONE)
					Ouf << form ("/%03d ",
						zz_trunc (l, 3));
				else
					Ouf << "     ";

				print (e->ai->zz_eid (e->event_id), 10);
				Ouf << ' ';

				print (e->process->zz_sn (e->chain->pstate),10);
				Ouf << '\n';
			}

			if ((r = r->other) == e->chain) break;
		}
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Mailbox::exPrint1 (const char *hdr, int sid) {

/* --------------------------------- */
/* Print the abreviated request list */
/* --------------------------------- */

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;

	if (hdr != NULL)
		Ouf << hdr << "\n\n";
	else {
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
	Ouf << "      Process/Ident      Event        State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (e->chain == NULL || e->station == NULL) continue;
		if (isStationId (sid) && ident (e->station) != sid) continue;

		for (r = e->chain;;) {

			if (r->ai->Class == AIC_mailbox) {

				if (!isStationId (sid))
					ptime (r->when, 20);
				else
					ptime (r->when, 26);

				if (pless (e->waketime, r->when))
					Ouf << ' ';     // Obsolete
				else if (e->chain == r)
					Ouf << '*';     // Current
				else
					Ouf << '?';

				if (!isStationId (sid)) {
					// Identify the station
					if (ident (e->station) >= 0)
						Ouf << form ("%6d ",
					zz_trunc (ident (e->station), 5));
					else
						Ouf << "   Sys ";
				}
				print (e->process->getTName (), 10);
				Ouf << form ("/%05d ",
					zz_trunc (e->process->Id, 5));

				print (zz_eid (r->event_id), 10);
				Ouf << ' ';

				print (e->process->zz_sn (r->pstate), 12);
				Ouf << '\n';
			}

			if ((r = r->other) == e->chain) break;
		}
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Mailbox::exPrint2 (const char *hdr, int longer) {

/* -------------------------- */
/* Print the Mailbox contents */
/* -------------------------- */

	Long       cntr;
	ZZ_REQUEST *rq;

	if (longer) {
		print ("        Mailbox", 16);
		print ("      Count", 11);
		print ("      Limit", 11);
		print ("   Requests", 11);
		Ouf << '\n';
	}

	if (hdr == NULL) hdr = getOName ();
	print (hdr, 16); Ouf << ' ';
	print (count, 10);
	if (limit == MAX_Long)
		print ("Infinity", 11);
	else if (limit < 0)
		print ("Trigger", 11);
	else
		print (limit, 11);
	// Count wait requests
	for (rq = WList, cntr = 0; rq != NULL; rq = rq -> next, cntr++);
	print (cntr, 11);
	Ouf << '\n';
}

void    Mailbox::exDisplay0 (int sid) {

/* ----------------------------- */
/* Display the full request list */
/* ----------------------------- */

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;
	Long               l;

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (e->chain == NULL || e->station == NULL) continue;
		if (isStationId (sid) && ident (e->station) != sid) continue;

		for (r = e->chain;;) {

			if (r->ai->Class == AIC_mailbox) {

				dtime (r->when);

				if (pless (e->waketime, r->when))
					display (' ');  // Obsolete
				else if (e->chain == r)
					display ('*');  // Current
				else
					display ('?');

				if (!isStationId (sid)) {
					// Identify the station
					if (ident (e->station) >= 0)
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
				display (e->process->zz_sn (e->chain->pstate));
			}
			if ((r = r->other) == e->chain) break;
		}
	}
}

void    Mailbox::exDisplay1 (int sid) {

/* ----------------------------------- */
/* Display the abreviated request list */
/* ----------------------------------- */

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (e->chain == NULL || e->station == NULL) continue;
		if (isStationId (sid) && ident (e->station) != sid) continue;

		for (r = e->chain;;) {

			if (r->ai->Class == AIC_mailbox) {

				dtime (r->when);
				if (pless (e->waketime, r->when))
					display (' ');
				else if (e->chain == r)
					display ('*');
				else
					display ('?');

				if (!isStationId (sid)) {
					// Identify the station
					if (ident (e->station) >= 0)
						   display (ident (e->station));
					else
						   display ("Sys");
				}
				display (e->process->getTName ());
				display (e->process->Id);
				display (zz_eid (r->event_id));
				display (e->process->zz_sn (r->pstate));
			}
			if ((r = r->other) == e->chain) break;
		}
	}
}

void    Mailbox::exDisplay2 () {

/* ---------------------------- */
/* Display the Mailbox contents */
/* ---------------------------- */

	Long       cntr;
	ZZ_REQUEST *rq;

	display (count);
	if (limit == MAX_Long)
		display ("Inf");
	else if (limit < 0)
		display ("Tri");
	else
		display (limit);
	// Count wait requests
	for (rq = WList, cntr = 0; rq != NULL; rq = rq -> next, cntr++);
	display (cntr);
}

#if  ZZ_REA || ZZ_RSY

#include "connect.h"

#endif
