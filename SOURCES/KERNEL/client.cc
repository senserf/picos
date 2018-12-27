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

#if	ZZ_NOC

// #define CDEBUG 1

/* ---------------------- */
/* The standard Client AI */
/* ---------------------- */

#include        "system.h"
#include	"cdebug.h"

#define	VA_TYPE	va_list

/* --------------------------------------------- */
/* Create with nickname for Traffic's RVariables */
/* --------------------------------------------- */

#define rvcreate(vn,nn,txp)\
  { char *zz_cc; zz_cc = (nn); tp->vn = new RVariable; \
    tp->vn->zz_start (); tp->vn->zz_nickname = \
    new char [strlen(zz_cc)+1]; strcpy (tp->vn->zz_nickname, zz_cc);\
    tp->vn->setup (txp, 2); }

/* ---------------------------------------------------------- */
/* Create  for temporary RVariables (bypasses the nonstandard */
/* constructor                                                */
/* ---------------------------------------------------------- */

#define tcreate(t) { trv = new ZZ_TRVariable; trv->setup (t, 2); }

#define	mq_sen(qp)	zz_hptr (qp)

zz_client::zz_client () {

/* --------------- */
/* The constructor */
/* --------------- */

	Class = AIC_client;
	Id = NONE;
}

static  Long cp2m1 (Long n) {

/* ---------------------------------------------------------- */
/* Calculates  the  smallest  power  of two minus one greater */
/* than or equal to the nonnegative argument                  */
/* ---------------------------------------------------------- */

	Long    res;

	for (res = 16; res <= n; res += res);

	return (res - 1);
}

/* --------------------------------------- */
/* Client service process type declaration */
/* --------------------------------------- */

extern  int     zz_ClientService_prcs;

class   ClientService : public ZZ_SProcess {

	public:

	Station *S;

	ClientService () {
		S = TheStation;
		zz_typeid = (void*) (&zz_ClientService_prcs);
	};

	virtual char *getTName () { return ("ClientService"); };

	enum {Start, GenMessage, GenBurst};

	private:

		Traffic         *mst;
		int             st, sr, i, tp;
		Long            ml;
		TIME            t;

	public:

	void setup () { 

		// Initialize state name list

		zz_ns = 3;
		zz_sl = new const char* [GenBurst + 1];
		zz_sl [0] = "Start";
		zz_sl [1] = "GenMessage";
		zz_sl [2] = "GenBurst";
	};

	void zz_code ();
};

static  ClientService *CSPHandle = NULL;

void    ClientService::zz_code () {

     switch (TheState) {

	  case Start:

		// Initialize the process execution
		for (i = 0; i < NTraffics; i++) {
			// Schedule initial events for all traffic patterns
			mst = idToTraffic (i);
			if (mst->FlgACT == OFF) continue;
			if (mst->DstBIT != UNDEFINED) {
				// Schedule the burst generation event
				new ZZ_EVENT (zz_sentinel_event,
					Time + mst->genBIT (), System, NULL,
						(void*) i, TheProcess, Client,
							ARR_BST, GenBurst,
								NULL);
			} else if (mst->DstMIT != UNDEFINED) {
				new ZZ_EVENT (Time + mst->genMIT (), System,
					NULL, (void*) i, TheProcess, Client,
						ARR_MSG, GenMessage, NULL);
			}
		}
			
	  break; case GenMessage:

		// Generate new message
		mst = idToTraffic (tp = TheTraffic);
		assert (mst->DstMIT != UNDEFINED,
		  "Client: MIT distribution for Traffic %s undefined",
		    mst->getSName ());
		st = mst->genSND ();            // Generate sender
		assert (isStationId (st),
		  "Client: cannot generate sender for Traffic %s", 
			mst->getSName ());
		ml = mst->genMLE ();            // Generate message length

		mst->genMSG (st, mst->genRCV (), ml);

		// Schedule next message generation event
		if (mst->DstBIT == UNDEFINED || --(mst->RemBSI) > 0) {
			new ZZ_EVENT (Time + mst->genMIT (), System, NULL,
				(void*) tp, TheProcess, Client,
					ARR_MSG, GenMessage, NULL);
		}

	  break; case GenBurst:

		// Generate new burst
		mst = idToTraffic (tp = TheTraffic);
		assert (mst->DstBIT != UNDEFINED,
		  "Client: Traffic %s is not bursty", mst->getSName ());
		ml = mst->genBSI ();            // Generate burst size
		if (! mst->RemBSI && ml)
			// Schedule message generation event
			new ZZ_EVENT (Time + mst->genMIT (), System, NULL,
				(void*) tp, TheProcess, Client,
					ARR_MSG, GenMessage, NULL);
		mst -> RemBSI += ml;
		// Schedule next burst generation event
		new ZZ_EVENT (zz_sentinel_event, Time + mst->genBIT (), System,
			NULL, (void*) tp, TheProcess, Client, ARR_BST,
				GenBurst, NULL);
     }
};

char *Packet::zz_getptr () {

	if (zz_pvoffset & 0x80000000) {
		// Determine the location of virtual function pointer
		(char*)(&(this->QTime)) - (char*)this;
		if ( (char*)(&(this->QTime)) - (char*)this ) {
			zz_pvoffset = 0;
		} else {
			zz_pvoffset = (char*)(&(this->Receiver)) -
				(char*)this + sizeof (IPointer);
		}
	}

	return *((char**)((char*)this + zz_pvoffset));
};

void    zz_start_client () {            // Starts the standard Client

	Traffic         *tp;
	int             i, j;
	CGroup          *cg;
	SGroup          *sg;


	// Postprocess traffic patterns

	for (i = 0; i < NTraffics; i++) {
		tp = idToTraffic (i);
		for (j = 0; j < tp->NCGroups; j++) {
			cg = tp->CGCluster [j];
			Assert (cg != NULL,
      "Client: Traffic %s -- CGroup %1d undefined, but a higher group defined",
				tp->getSName (), j);
			sg = cg->Senders;
			Assert (sg != NULL, 
      "Client: Traffic %s, CGroup %1d -- Senders not defined",
				tp->getSName (), j);
			zz_sort_group (sg, cg->SWeights);
			Assert (sg->NStations,
      "Client: Traffic %s, CGroup %1d -- The sender group has zero weight",
				tp->getSName (), j);
			if ((sg = cg->Receivers) != NULL) {
			    if (cg->RWeights != (float*) (-1)) {
				zz_sort_group (sg, cg->RWeights);
				Assert (sg->NStations,
      "Client: Traffic %s, CGroup %1d -- The receiver group has zero weight",
				tp->getSName (), j);
			    } else 
				zz_sort_group (sg);
			}
		}
		tp->preprocess_weights ();
	}

	// Initialize counters (global)

	zz_NGMessages = zz_NQMessages = zz_NTMessages = zz_NRMessages =
	zz_NTPackets = zz_NRPackets = 0;
	zz_NQBits = zz_NTBits = zz_NRBits = BITCOUNT_0;
	zz_GSMTime = Time;

	for (i = 0; i < NTraffics; i++) {

		tp = idToTraffic (i);

		// Initialize counters (local)
		tp->NQMessages = tp->NTMessages = tp->NRMessages =
		tp->NTPackets = tp->NRPackets = 0;
		tp->NQBits = tp->NTBits = tp->NRBits = BITCOUNT_0;
		tp->RemBSI = 0;        // Remaining burst size
		tp->SMTime = Time;
#if     ZZ_CLI
	    if (zz_flg_stdClient) {

		// Determine if standard performance measures are to be
		// calculated

		if (tp->FlgSPF == ON) {

			// Generate standard RVariables and make them belong
			// to the Traffic

			rvcreate (RVAMD, "AMDelay", TYPE_long);
			// Move it to Traffic
			pool_out ((ZZ_Object*)(tp->RVAMD));
			pool_in ((ZZ_Object*)(tp->RVAMD), tp->ChList);
	
			rvcreate (RVAPD, "APDelay", TYPE_long);
			pool_out ((ZZ_Object*)(tp->RVAPD));
			pool_in ((ZZ_Object*)(tp->RVAPD), tp->ChList);
	
			rvcreate (RVWMD, "WMDelay", TYPE_BITCOUNT);
			pool_out ((ZZ_Object*)(tp->RVWMD));
			pool_in ((ZZ_Object*)(tp->RVWMD), tp->ChList);
	
			rvcreate (RVMAT, "MAcTime", TYPE_long);
			pool_out ((ZZ_Object*)(tp->RVMAT));
			pool_in ((ZZ_Object*)(tp->RVMAT), tp->ChList);
	
			rvcreate (RVPAT, "PAcTime", TYPE_long);
			pool_out ((ZZ_Object*)(tp->RVPAT));
			pool_in ((ZZ_Object*)(tp->RVPAT), tp->ChList);
	
			rvcreate (RVMLS, "MsgStat", TYPE_long);
			pool_out ((ZZ_Object*)(tp->RVMLS));
			pool_in ((ZZ_Object*)(tp->RVMLS), tp->ChList);
	
		}
	    }
#endif

	}

#if     ZZ_CLI
	if (zz_flg_stdClient) {
		// Create the Client service process
		// phandle = create ClientService;
		CSPHandle = new ClientService;
		((ClientService*)CSPHandle)->zz_start ();
		((ClientService*)CSPHandle)->setup ();
	}
#endif

}

static	ZZ_REQUEST *SRWList = NULL;	// Suspend/resume waiting list

#if  ZZ_TAG
void    zz_client::wait (int ev, int pstate, LONG tag) {
	int q;
#else
void    zz_client::wait (int ev, int pstate) {
#endif

/* ----------------------- */
/* The Client wait request */
/* ----------------------- */

	if_from_observer ("Client->wait: called from an observer");
	assert (ev >= -5 && ev < NTraffics, "Client->wait: illegal event %1d",
		ev);

	if (zz_c_first_wait) {

		zz_c_other = NULL;    // No requests so far

		// Create new simulation event
		zz_c_wait_event = new ZZ_EVENT;
		zz_c_wait_event -> station = TheStation;
		zz_c_wait_event -> process = TheProcess;

		if (ev < -3)       // ARRIVAL or INTERCEPT
			new ZZ_REQUEST (&(TheStation->CWList), this, ev,pstate);
		else if (ev < 0)   // SUSPEND/RESUME/RESET
			new ZZ_REQUEST (&SRWList, this, ev,pstate);
		else               // Traffic wait request
			new ZZ_REQUEST (&(TheStation->CWList), idToTraffic (ev),
				ARRIVAL, pstate);

		assert (zz_c_other != NULL,
			"Client->wait: internal error -- null request");

		zz_c_wait_event -> pstate = pstate;
		zz_c_wait_event -> ai = zz_c_other->ai;
		zz_c_wait_event -> event_id = zz_c_other->event_id;
		zz_c_wait_event -> chain = zz_c_other;
		zz_c_wait_event -> Info01 = NULL;
		zz_c_wait_event -> Info02 = NULL;

		zz_c_whead = zz_c_other;
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;
		zz_c_first_wait = NO;

		if (ev==SUSPEND && isSuspended () ||
                 ev==RESUME && isResumed ()) {
		  // The event occurs immediately
#if	ZZ_TAG
		  zz_c_wait_tmin . set (Time, tag);
		  zz_c_other -> when =
			zz_c_wait_event ->  waketime = zz_c_wait_tmin;
#else
                  zz_c_wait_event -> waketime =
                           zz_c_wait_tmin = zz_c_other -> when = Time;
#endif
		  zz_c_wait_event -> enqueue ();
		} else {
		  // Event time unknown
#if     ZZ_TAG
		  zz_c_wait_tmin . set (TIME_inf, tag);
		  zz_c_wait_event -> waketime = zz_c_other -> when =
			zz_c_wait_tmin;
#else
		  zz_c_wait_event -> waketime =
			zz_c_wait_tmin = zz_c_other -> when = TIME_inf;
#endif
  		  zz_c_wait_event->store ();
		}

	} else {

                if (ev < -3)       // ARRIVAL or INTERCEPT
                        new ZZ_REQUEST (&(TheStation->CWList), this, ev,pstate);
                else if (ev < 0)   // SUSPEND/RESUME/RESET
                        new ZZ_REQUEST (&SRWList, this, ev,pstate);
                else               // Traffic wait request
                        new ZZ_REQUEST (&(TheStation->CWList), idToTraffic (ev),
                                ARRIVAL, pstate);

		assert (zz_c_other != NULL,
			"Client->wait: internal error -- null request");

		if (ev==SUSPEND && isSuspended () ||
                 ev==RESUME && isResumed ()) {
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
                            zz_c_wait_event -> Info02 = NULL;
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
                            zz_c_wait_event -> Info02 = NULL;
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

void    Traffic::suspend () {

/* ---------------------------- */
/* Disables the traffic pattern */
/* ---------------------------- */

  ZZ_EVENT *ev, *eh;
  ZZ_REQUEST *cw;
#if ZZ_TAG
  int q;
#endif

  if (FlgSUS == ON) return;    // Already suspended
  FlgSUS = ON;
  if (FlgSCL == ON) FlgACT = OFF;
  // Remove Client events for this traffic pattern
  if (zz_flg_stdClient) {
    for (ev = zz_eq; ev != zz_sentinel_event; ) {
      // System events only
      if (ev -> ai != Client || ev -> chain != NULL ||
       ptrToLong (ev->Info02) != Id){
        ev = ev -> next;
        continue;
      }
      // Release the event
      eh = ev -> next;
      ev -> cancel ();
      delete ev;
      ev = eh;
    }
  }
  // Generate the SUSPEND event
  for (cw = SRWList; cw != NULL; cw = cw -> next) {
    if (cw->event_id == SUSPEND && cw->ai == this) {
        cw -> Info02 = (void*) Id;
#if     ZZ_TAG
	cw -> when . set (Time);
	if ((q = (ev = cw-> event) -> waketime .  cmp (cw -> when)) > 0 ||
		(q == 0 && FLIP))
#else
	cw -> when = Time;
	if ((ev = cw-> event) -> waketime > Time || FLIP)
#endif
						ev->new_top_request (cw);
    }
  }
}

void    Traffic::resume () {

/* ------------------------- */
/* Enables a traffic pattern */
/* ------------------------- */

  ZZ_EVENT *ev;
  ZZ_REQUEST *cw;
#if ZZ_TAG
  int q;
#endif

  Assert (zz_flg_stdClient,
    "Traffic->resume illegal: Client is permanently disabled");
  assert (CSPHandle != NULL, "Traffic->resume: CSP handle is NULL");
  if (FlgSUS == OFF) return;     // Already enabled
  FlgSUS = OFF;
  if (FlgSCL == ON) {
    FlgACT = ON;
    if (zz_flg_stdClient) {
      assert (CSPHandle != NULL, "Traffic->resume: CSP handle is NULL");
      if (DstBIT != UNDEFINED) {
        // Schedule the burst generation event
        new ZZ_EVENT (zz_sentinel_event, Time + genBIT (), System, NULL,
         (void*) Id, CSPHandle, Client, ARR_BST, ClientService::GenBurst, NULL);
      } else if (DstMIT != UNDEFINED) {
        new ZZ_EVENT (Time + genMIT (), System, NULL, (void*) Id, CSPHandle,
         Client, ARR_MSG, ClientService::GenMessage, NULL);
      }
    }
  }
  // Generate the RESUME event
  for (cw = SRWList; cw != NULL; cw = cw -> next) {
    if (cw->event_id == RESUME && cw->ai == this) {
        cw -> Info02 = (void*) Id;
#if     ZZ_TAG
        cw -> when . set (Time);
        if ((q = (ev = cw-> event) -> waketime .  cmp (cw -> when)) > 0 ||
                (q == 0 && FLIP))
#else
        cw -> when = Time;
        if ((ev = cw-> event) -> waketime > Time || FLIP)
#endif
                                                ev->new_top_request (cw);
    }
  }
}

int     Traffic::isSuspended () {

/* -------------------------------------------- */
/* Tells whether a traffic pattern is suspended */
/* -------------------------------------------- */

	return (FlgSUS == ON);
}

int	zz_client::isSuspended () {

// Tells whether the Client is suspended

	int i;

	for (i = 0; i < NTraffics; i++)
		if (idToTraffic (i) -> FlgSUS != ON)
			return (NO);
	return (YES);
}

void    zz_client::suspend () {

/* ----------------------------- */
/* Disables all traffic patterns */
/* ----------------------------- */

  int   i;
  ZZ_EVENT *ev;
  ZZ_REQUEST *cw;

  if (isSuspended ()) return;
  for (i = 0; i < NTraffics; i++) idToTraffic (i) -> suspend ();
  // Generate the Client SUSPEND event
  for (cw = SRWList; cw != NULL; cw = cw -> next) {
    if (cw->event_id == SUSPEND && cw->ai == this) {
#if     ZZ_TAG
        cw -> when . set (Time);
        if ((i = (ev = cw-> event) -> waketime .  cmp (cw -> when)) > 0 ||
                (i == 0 && FLIP))
#else
        cw -> when = Time;
        if ((ev = cw-> event) -> waketime > Time || FLIP)
#endif
                                                ev->new_top_request (cw);
    }
  }
}

void    zz_client::resume () {

/* ---------------------------- */
/* Enables all traffic patterns */
/* ---------------------------- */

  int   i;
  ZZ_EVENT *ev;
  ZZ_REQUEST *cw;

  if (isResumed ()) return;
  for (i = 0; i < NTraffics; i++) idToTraffic (i) -> resume ();
  // Generate the Client RESUME event
  for (cw = SRWList; cw != NULL; cw = cw -> next) {
    if (cw->event_id == RESUME && cw->ai == this) {
#if     ZZ_TAG
        cw -> when . set (Time);
        if ((i = (ev = cw-> event) -> waketime .  cmp (cw -> when)) > 0 ||
                (i == 0 && FLIP))
#else
        cw -> when = Time;
        if ((ev = cw-> event) -> waketime > Time || FLIP)
#endif
                                                ev->new_top_request (cw);
    }
  }
}

void    Traffic::resetSPF () {

/* ---------------------------------------------------------- */
/* Resets  standard  performance  measures  for  the  traffic */
/* pattern                                                    */
/* ---------------------------------------------------------- */

  ZZ_REQUEST *cw;
  ZZ_EVENT *ev;
#if	ZZ_TAG
  int q;
#endif

  if (FlgSPF != OFF) {
	  RVAMD -> erase ();
	  RVAPD -> erase ();
	  RVWMD -> erase ();
	  RVMAT -> erase ();
	  RVPAT -> erase ();
	  RVMLS -> erase ();

	  NTMessages -= NRMessages;
	  NRMessages = 0;
	  NTPackets -= NRPackets;
	  NRPackets = 0;
	  NTBits -= NRBits;
	  NRBits = BITCOUNT_0;
	  SMTime = Time;
  }
  // Generate the RESET event
  for (cw = SRWList; cw != NULL; cw = cw -> next) {
    if (cw->event_id == RESET && cw->ai == this) {
        cw -> Info02 = (void*) Id;
#if     ZZ_TAG
        cw -> when . set (Time);
        if ((q = (ev = cw-> event) -> waketime .  cmp (cw -> when)) > 0 ||
                (q == 0 && FLIP))
#else
        cw -> when = Time;
        if ((ev = cw-> event) -> waketime > Time || FLIP)
#endif
                                                ev->new_top_request (cw);
    }
  }
}

void    zz_client::resetSPF () {

/* ---------------------------------------------------------- */
/* Resets  standard  performance  measures  for  all  traffic */
/* patterns and zeroes out the global counters                */
/* ---------------------------------------------------------- */

  int i;
  ZZ_REQUEST *cw;
  ZZ_EVENT *ev;

  for (i = 0; i < NTraffics; i++) idToTraffic (i) -> resetSPF ();
  zz_NTMessages -= zz_NRMessages;
  zz_NGMessages = zz_NQMessages + zz_NTMessages;
  zz_NRMessages = 0;
  zz_NTPackets -= zz_NRPackets;
  zz_NRPackets = 0;
  zz_NTBits -= zz_NRBits;
  zz_NRBits = BITCOUNT_0;
  zz_GSMTime = Time;
  // Generate the Client RESET event
  for (cw = SRWList; cw != NULL; cw = cw -> next) {
    if (cw->event_id == RESET && cw->ai == this) {
#if     ZZ_TAG
        cw -> when . set (Time);
        if ((i = (ev = cw-> event) -> waketime .  cmp (cw -> when)) > 0 ||
                (i == 0 && FLIP))
#else
        cw -> when = Time;
        if ((ev = cw-> event) -> waketime > Time || FLIP)
#endif
                                                ev->new_top_request (cw);
    }
  }
} 

void    Traffic::setQSLimit (Long lim) {

/* ---------------------------------------------------------- */
/* Sets the limit on the number of messages belonging to this */
/* traffic  pattern  that  can  be  queued  at (all) stations */
/* awaiting transmission. When this  limit  is  reached,  new */
/* generated messages will be ignored.                        */
/* ---------------------------------------------------------- */

#if     ZZ_QSL
	Assert (FlgSPF == ON,
			"Traffic->setQSLimit illegal: %s, SPF_off was selected",
				getSName ());
	QSLimit = lim;
#else
	lim++;
	excptn ("Traffic->setQSLimit illegal: smurph not created with '-q'");
#endif
}

void    setQSLimit (Long lim) {

/* ---------------------------------------------------------- */
/* Sets  the  global limit on the number of messages that can */
/* be queued at (all) stations  awaiting  transmission.  When */
/* this  limit  is  reached,  new  generated messages will be */
/* ignored.                                                   */
/* ---------------------------------------------------------- */

#if     ZZ_QSL
	zz_QSLimit = lim;
#else
	lim++;
	excptn ("setQSLimit illegal: smurph not created with '-q'");
#endif
}

#if  ZZ_TAG
void    Traffic::wait (int ev, int pstate, LONG tag) {
	int q;
#else
void    Traffic::wait (int ev, int pstate) {
#endif

/* ------------------------------------------------ */
/* The Client wait request (addressed to a Traffic) */
/* ------------------------------------------------ */

	if_from_observer ("Traffic->wait: called from an observer");
	assert (ev < 0 && ev > -6, "Traffic->wait: %s, illegal event %1d",
		getSName (), ev);

	if (zz_c_first_wait) {

		zz_c_other = NULL;    // No requests so far

		// Create new simulation event
		zz_c_wait_event = new ZZ_EVENT;
		zz_c_wait_event -> station = TheStation;
		zz_c_wait_event -> process = TheProcess;

		if (ev < -3) // ARRIVAL or INTERCEPT
		  new ZZ_REQUEST (&(TheStation->CWList), this, ev, pstate);
		else
		  new ZZ_REQUEST (&SRWList, this, ev, pstate);

		assert (zz_c_other != NULL,
			"Traffic->wait: internal error -- null request");

		zz_c_wait_event -> pstate = pstate;
		zz_c_wait_event -> ai = this;
		zz_c_wait_event -> event_id = ev;
		zz_c_wait_event -> chain = zz_c_other;
		zz_c_wait_event -> Info01 = NULL;
		zz_c_wait_event -> Info02 = NULL;

		zz_c_whead = zz_c_other;
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;
		zz_c_first_wait = NO;
                if (ev==SUSPEND && FlgSUS==ON || ev==RESUME && FlgSUS==OFF) {
		  // The event occurs immediately
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
#if     ZZ_TAG
		  zz_c_wait_tmin . set (TIME_inf, tag);
		  zz_c_wait_event -> waketime =
			zz_c_other -> when = zz_c_wait_tmin;
#else
		  zz_c_wait_event -> waketime =
			zz_c_wait_tmin = zz_c_other -> when = TIME_inf;
#endif
		  zz_c_wait_event->store ();
		}

	} else {

                if (ev < -3) // ARRIVAL or INTERCEPT
                  new ZZ_REQUEST (&(TheStation->CWList), this, ev, pstate);
                else
                  new ZZ_REQUEST (&SRWList, this, ev, pstate);

		assert (zz_c_other != NULL,
			"Traffic->wait: internal error -- null request");

                if (ev==SUSPEND && FlgSUS==ON || ev==RESUME && FlgSUS==OFF) {
		  // The event occurs immediately
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
                            zz_c_wait_event -> Info02 = (void*) Id;
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
                            zz_c_wait_event -> Info02 = (void*) Id;
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

void    SGroup::setup (int n, int *sl) {

/* --------------------------- */
/* Initializes a station group */
/* --------------------------- */

	int     i, j, k;

	Assert (n > -::NStations && ::NStations >= n,
		"SGroup: illegal number of stations %1d", n);

	if (n == 0) {                   // Null group
		NStations = (int) ::NStations;
		SIdents = NULL;
		sorted = YES;
		return;
	}

	if (n < 0) {

		// Check if exceptions are legal station ids
		for (i = 0; i < -n; i++)
			if (!isStationId (sl [i]))
				excptn ("SGroup: illegal exception %1d", sl[i]);

		// Check exception list against duplicates
		for (k = n, i = 0; i < -n; i++) {
			if (sl [i] == NONE) continue;
			for (j = i+1; j < -n; j++)
				if (sl [i] == sl [j]) k++;
		}

		NStations = (int) (::NStations + k);
	} else
		NStations = n;

	SIdents = new short [cp2m1 (NStations)];

	if (n < 0) {
		// The array contains list of exceptions
		n = -n;
		for (i = 0, k = 0; i < ::NStations; i++) {

			for (j = 0; j < n; j++)
				 if (sl [j] == i) break;

			if (j >= n) SIdents [k++] = i;
		}
	} else {
		// The array contains list of stations
		for (i = 0; i < n; i++) {
			Assert (isStationId (sl [i]),
				"SGroup: illegal station id %1d", sl [i]);
			SIdents [i] = sl [i];
		}
	}
}

void    SGroup::setup (int n, int f, ...) {

/* ------------------------------------------- */
/* As above, but with an explicit station list */
/* ------------------------------------------- */

	int     i;
        VA_TYPE      ap;    // For processing variable-length argument list 
	narray  (slist, int, n);

	Assert (n > 0, "SGroup: illegal station count %1d", n);

	va_start (ap, n);   // Start variable-length argument processing

	slist [0] = f;

	for (i = 1; i < n; i++)
          slist [i] = va_arg (ap, int);

	va_end (ap);
        setup (n, slist);
	darray (slist);
}

void    SGroup::setup (int n, Station *f, ...) {

/* -------------------------------------------------- */
/* As above, but with station pointers instead of ids */
/* -------------------------------------------------- */

	VA_TYPE      ap;    // For processing variable-length argument list
	int     i;
        Station *st;
	narray  (slist, int, n);

	Assert (n > 0, "SGroup: illegal station count %1d", n);

	va_start (ap, n);   // Start variable-length argument processing

	slist [0] = (int)(f->Id);

	for (i = 1; i < n; i++) {
		st = va_arg (ap, Station*);
		slist [i] = (int)(st->Id);
	}
	va_end (ap);
	setup (n, slist);
	darray (slist);
}

static  short   *s_idents;      // Global variables to reduce the number of
static  float   *s_weights;     // parameters.

static void group_sorter (int lo, int up) {

/* -----------------------------------*/
/* The proper sorter for 'sort_group' */
/* -----------------------------------*/

int     i, j;
short   t;
float   s;

	// This is a simple variation of quicksort

	while (up > lo) {
		t = s_idents [i = (lo + up) / 2];
		s_idents [i] = s_idents [lo];
		s_idents [lo] = t;
		if (s_weights != NULL) {
			s = s_weights [i];
			s_weights [i] = s_weights [lo];
			s_weights [lo] = s;
		}
		i = lo;
		j = up;

		while (i < j) {
			while (s_idents [j] > t) j--;
			s_idents [i] = s_idents [j];
			if (s_weights != NULL)
				s_weights [i] = s_weights [j];
			while ((i < j) && (s_idents [i] <= t)) i ++;
			s_idents [j] = s_idents [i];
			if (s_weights != NULL)
				s_weights [j] = s_weights [i];
		}
		s_idents [i] = t;
		if (s_weights != NULL) s_weights [i] = s;
		group_sorter (lo, i-1);
		lo = i + 1;
	}
}

void zz_sort_group (SGroup *sg, float *w) {

/* ---------------------------------------------------------- */
/* Sort  station  id's  (and  weights)  in the non-decreasing */
/* order                                                      */
/* ---------------------------------------------------------- */

	int     n, i, j;
	short   l;

	if (sg->sorted) return;
	sg->sorted = YES;
	if ((s_idents = sg->SIdents) == NULL) return;   // Nothing to sort
	s_weights = w;
	n = sg->NStations;
	group_sorter (0, n-1);

	// Remove duplicates

	for (i = 1, j = 0, l = s_idents [0]; i < n; i++) {
		if (s_idents [i] != l) {
			s_idents [++j] = l = s_idents [i];
			if (s_weights != NULL) s_weights [j] = s_weights [i];
		} else {
			if (s_weights != NULL) s_weights [j] += s_weights [i];
		}
	}

	n = j + 1;

	if (s_weights != NULL) {
		for (i = 0, j = 0; i < n; i++) {
			// Remove stations with zero weight
			if (s_weights [i] > 0.0) {
				s_idents [j] = s_idents [i];
				s_weights [j++] = s_weights [i];
			}
		}
		n = j;
	}
	sg->NStations = n;
}

void    CGroup::setup (SGroup *snd, float *snw, SGroup *rcv, float *rcw) {

/* --------------------------------- */
/* Initializes a communication group */
/* --------------------------------- */

	Senders = snd;
	Receivers = rcv;
	CGWeight = 0.0;
	
	if (snw != NULL && Senders->NStations) {
		SWeights = new float [cp2m1 (Senders->NStations)];
		for (int i = 0; i < Senders->NStations; i++) {
			Assert (snw [i] >= 0.0,
				"CGroup: negative sender weight %f", snw [i]);
			CGWeight += (SWeights [i] = snw [i]);
		}
	} else {
			SWeights = NULL;
			CGWeight = 1.0;
	}

	// Assume all stations
	if (Senders->NStations == 0) Senders->NStations = (int) NStations;

	if (rcw != NULL && Receivers->NStations) {
		RWeights = new float [cp2m1 (Receivers->NStations)];
		for (int i = 0; i < Receivers->NStations; i++) {
			Assert (rcw [i] >= 0.0,
				"CGroup: negative receiver weight %f", rcw [i]);
			RWeights [i] = rcw [i];
		}
	} else
			RWeights = NULL;
};

void    CGroup::setup (SGroup *snd, float *snw, SGroup *rcv, int bdcst) {

/* ------------------------------------------------ */
/* Initializes a broadcast-type communication group */
/* ------------------------------------------------ */

	Senders = snd;
	Receivers = rcv;
	CGWeight = 0.0;
	
	if (snw != NULL && Senders->NStations) {
		SWeights = new float [cp2m1 (Senders->NStations)];
		for (int i = 0; i < Senders->NStations; i++) {
			Assert (snw [i] >= 0.0,
				"CGroup: negative sender weight %f", snw [i]);
			CGWeight += (SWeights [i] = snw [i]);
		}
	} else {
			SWeights = NULL;
			CGWeight = 1.0;
	}
	// Assume all stations
	if (Senders->NStations == 0) Senders->NStations = (int) NStations;

	Assert (bdcst == GT_broadcast, "CGroup: illegal argument (%1d), "
		"should be GT_broadcast", bdcst);

	RWeights = (float*) (-1);       // Flag as broadcast type
};

void    Traffic::zz_addinit (int gr) {

/* -------------------------------------------- */
/* A preprocessor for addSender and addReceiver */
/* -------------------------------------------- */

	int     mgr, i;
	CGroup  **tcg, *cg;

	if ((mgr = gr+1) > NCGroups) {
		// A brand new group to be created
		if (CGCluster == NULL) {
			// Creating from scratch
			Assert (NCGroups == 0,
	  "Traffic->add...: internal error -- inconsistent initialization");
			CGCluster = new CGroup* [cp2m1 (mgr)];
		} else if (mgr > cp2m1 (NCGroups)) {
			// The group array must be extended
			tcg = new CGroup* [NCGroups];
			for (i = 0; i < NCGroups; i++) tcg [i] = CGCluster [i];
			delete [] CGCluster;
			CGCluster = new CGroup* [cp2m1 (mgr)];
			for (i = 0; i < NCGroups; i++) CGCluster [i] = tcg [i];
			delete [] tcg;
		}
		for (i = NCGroups; i < mgr; i++) CGCluster [i] = NULL;
		NCGroups = mgr;
	}

	if ((cg = CGCluster [gr]) == NULL) {
		// Must create the communication group
		cg = new CGroup;
		cg->Senders = cg->Receivers = NULL;
		cg->SWeights = cg->RWeights = NULL;
		cg->CGWeight = 0.0;
		CGCluster [gr] = cg;
	}
}

void    Traffic::addSender (Station *s, double w, int gr) {

/* ---------------------------------------------------------- */
/* Adds a sender to dynamically created/updated communication */
/* group                                                      */
/* ---------------------------------------------------------- */

	CGroup  *cg;
	SGroup  *sg;
	short   *sl;
	float   *fl;
	int     i;

	Assert (w != BROADCAST,
	  "Traffic->addSender: %s, BROADCAST illegal for senders", getSName ());
	Assert (w >= 0.0, "Traffic->addSender: %s, negative weight %f",
		getSName (), w);
	Assert (gr >=0, "Traffic->addSender: %s, negative group %1d",
		getSName (), gr);

	zz_addinit (gr);
	cg = CGCluster [gr];
	if ((sg = cg->Senders) == NULL) {
		sg = new SGroup;
		sg->NStations = 0;
		sg->SIdents = NULL;
		sg->sorted = NO;
		cg->Senders = sg;
	}

	if (s == ALL) {
		Assert (w == 1.0,
			"Traffic->addSender: %s, weight (%f) must be 1.0 if "
				"ALL stations are included",
					getSName (), w);
		// All stations
		Assert (sg->NStations == 0,
      			"Traffic->addSender: %s, senders of group %1d, "
				"duplicate definition", getSName (), gr);
		sg->NStations = (int) ::NStations;
		cg->CGWeight = 1.0;
		sg->sorted = YES;
		cg->SWeights = NULL;
		return;
	}

	Assert (sg->NStations != ::NStations || sg->SIdents != NULL,
      		"Traffic->addSender: %s, senders of group %1d, cannot "
			"extend ALL", getSName (), gr);

	// Add a new entry

	if (sg->SIdents == NULL) {
		// Creating from scratch
		Assert (sg->NStations == 0 && cg->CGWeight == 0.0 &&
			cg->SWeights == NULL,
	  "Traffic->addSender: internal error -- inconsistent initialization");
		sg->SIdents = new short [cp2m1 (1)];
		cg->SWeights = new float [cp2m1 (1)];
	} else if (sg->NStations + 1 > cp2m1 (sg->NStations)) {
		// The group array must be extended
		sl = new short [sg->NStations];
		for (i = 0; i < sg->NStations; i++) sl [i] = sg->SIdents [i];
		delete [] sg->SIdents;
		sg->SIdents = new short [cp2m1 (sg->NStations + 1)];
		for (i = 0; i < sg->NStations; i++) sg->SIdents [i] = sl [i];
		delete [] sl;

		fl = new float [sg->NStations];
		for (i = 0; i < sg->NStations; i++) fl [i] = cg->SWeights [i];
		delete [] cg->SWeights;
		cg->SWeights = new float [cp2m1 (sg->NStations + 1)];
		for (i = 0; i < sg->NStations; i++) cg->SWeights [i] = fl [i];
		delete [] fl;
	}

	sg->SIdents [sg->NStations] = (short) ident (s);
	cg->SWeights [sg->NStations] = w;
	sg->NStations ++;
	cg->CGWeight += w;
	sg->sorted = NO;
}

void    Traffic::addSender (Long s, double w, int gr) {

/* ---------------------------------------------------------- */
/* Another version of the above -- with station id instead of */
/* pointer                                                    */
/* ---------------------------------------------------------- */

	Assert (isStationId (s),
     		"Traffic->addSender: %s, group %1d, %1d is not a valid "
			"station id", getSName (), gr, s);

	addSender (idToStation (s), w, gr);
}

void    Traffic::addReceiver (Station *s, double w, int gr) {

/* ---------------------------------------------------------- */
/* Adds    a    receiver   to   dynamically   created/updated */
/* communication group                                        */
/* ---------------------------------------------------------- */

	CGroup  *cg;
	SGroup  *sg;
	short   *sl;
	float   *fl;
	int     i;

	Assert (w >= 0.0 || w == BROADCAST,
		"Traffic->addReceiver: %s, negative weight %f",
			getSName (), w);
	Assert (gr >=0, "Traffic->addReceiver: %s, negative group %1d",
			getSName (), gr);

	zz_addinit (gr);
	cg = CGCluster [gr];
	if ((sg = cg->Receivers) == NULL) {
		sg = new SGroup;
		sg->NStations = 0;
		sg->SIdents = NULL;
		sg->sorted = NO;
		cg->Receivers = sg;
	}

	if (s == ALL) {

		Assert (w == 1.0 || w == BROADCAST,
	      		"Traffic->addSender: %s, with ALL, weight (%f) must be "
				"1.0 or BROADCAST", getSName (), w);
		// All stations
		Assert (sg->NStations == 0,
    			"Traffic->addSender: %s, receivers of group %1d, "
				"duplicate definition", getSName (), gr);
		sg->NStations = (int) ::NStations;
		sg->sorted = YES;
		cg->RWeights = (w == BROADCAST) ? (float*) (-1) : NULL;
		return;
	}

	Assert (sg->NStations != ::NStations || sg->SIdents != NULL,
     		"Traffic->addReceiver: %s, receivers of group %1d, cannot "
			"extend ALL", getSName (), gr);

	// Add a new entry

	if (sg->SIdents == NULL) {
		// Creating from scratch
		Assert (sg->NStations == 0 && cg->RWeights == NULL,
			"Traffic->addReceiver: internal error -- inconsistent "
				"initialization");
		sg->SIdents = new short [cp2m1 (1)];
		cg->RWeights = (w == BROADCAST) ? (float*) (-1) :
			new float [cp2m1 (1)];
	} else if (sg->NStations + 1 > cp2m1 (sg->NStations)) {
		// The group array must be extended
		sl = new short [sg->NStations];
		for (i = 0; i < sg->NStations; i++) sl [i] = sg->SIdents [i];
		delete [] sg->SIdents;
		sg->SIdents = new short [cp2m1 (sg->NStations + 1)];
		for (i = 0; i < sg->NStations; i++) sg->SIdents [i] = sl [i];
		delete [] sl;

		if (cg->RWeights != (float*) (-1)) {
			fl = new float [sg->NStations];
			for (i = 0; i < sg->NStations; i++)
				fl [i] = cg->RWeights [i];
			delete [] cg->RWeights;
			cg->RWeights = new float [cp2m1 (sg->NStations + 1)];
			for (i = 0; i < sg->NStations; i++)
				cg->RWeights [i] = fl [i];
			delete [] fl;
		}
	}

	sg->SIdents [sg->NStations] = (short) ident (s);
	if (w == BROADCAST) {
		Assert (cg->RWeights == (float*) (-1),
			"Traffic->addReceiver: %s, group %1d nonempty, cannot "
				"change to BROADCAST", getSName (), gr);
	} else {
		Assert (cg->RWeights != (float*) (-1),
			"Traffic->addReceiver: %s, group %1d nonempty, cannot "
				"change to non-BROADCAST", getSName (), gr);
		cg->RWeights [sg->NStations] = w;
	}
	sg->NStations ++;
	sg->sorted = NO;
}

void    Traffic::addReceiver (Long s, double w, int gr) {

/* ---------------------------------------------------------- */
/* Another version of the above -- with station id instead of */
/* pointer                                                    */
/* ---------------------------------------------------------- */

	Assert (isStationId (s),
   		"Traffic->addReceiver: %s, group %1d, %1d is not a valid "
			"station id", getSName (), gr, s);

	addReceiver (idToStation (s), w, gr);
}

void    Traffic::zz_start () {

/* --------------------------- */
/* The nonstandard constructor */
/* --------------------------- */

	static  int     asize = 15;     // A small power of two - 1 (initial
					// size of zz_tp)
	Traffic         **scratch;
	Packet          *p;
        int i;

	Assert (!zz_flg_started,
	    "Traffic: cannot create Traffics after the protocol has started");
	ChList = NULL;
	LastSender = NONE;
	LastCGroup = NULL;
#if     ZZ_QSL
	QSLimit = MAX_Long;
#endif

	// Obtain the pointer to the virtual function table of the packet
	p = zz_mkp ();
	ptviptr = p->zz_getptr ();
	delete (p);

	if ((Id = sernum++) == 0) {
		// The first time around -- create the array of Traffics
		zz_tp = new Traffic* [asize];
	} else if (Id >= asize) {
		// The array must be enlarged
		scratch = new Traffic* [asize];
		for (i = 0; i < asize; i++)
			// Backup copy
			scratch [i] = zz_tp [i];

		delete [] zz_tp;         // Deallocate previous array
		zz_tp = new Traffic* [asize = (asize+1) * 2 - 1];
		while (i--) zz_tp [i] = scratch [i];
		delete [] scratch;
	}

	zz_tp [Id] = this;
	NTraffics = sernum;

	// Setup for class
	Class = AIC_traffic;

	// Add the object to the owner (at this moment it is Kernel)
	pool_in (this, TheProcess->ChList);
}

void    Traffic::zz_setup_flags (int flags) {

/* ------------------- */
/* Setup traffic flags */
/* ------------------- */

	DstMIT = DstMLE = DstBIT = DstBSI = UNDEFINED;
	FlgACT = FlgSCL = FlgSPF = ON;
	FlgSUS = OFF;

	if (flags & MIT_exp) {
		Assert (! (flags & (MIT_unf | MIT_fix)),
			"Traffic: %s, illegal options %x", getSName (), flags);
		DstMIT = EXPONENTIAL;
	}
	if (flags & MIT_unf) {
		Assert (! (flags & (MIT_exp | MIT_fix)),
			"Traffic: %s, illegal options %x", getSName (), flags);
		DstMIT = UNIFORM;
	}
	if (flags & MIT_fix) {
		Assert (! (flags & (MIT_exp | MIT_unf)),
			"Traffic: %s, illegal options %x", getSName (), flags);
		DstMIT = FIXED;
	}
	
	
	if (flags & MLE_exp) {
		Assert (! (flags & (MLE_unf | MLE_fix)),
			"Traffic: %s, illegal options %x", getSName (), flags);
		DstMLE = EXPONENTIAL;
	}
	if (flags & MLE_unf) {
		Assert (! (flags & (MLE_exp | MLE_fix)),
			"Traffic: %s, illegal options %x", getSName (), flags);
		DstMLE = UNIFORM;
	}
	if (flags & MLE_fix) {
		Assert (! (flags & (MLE_exp | MLE_unf)),
			"Traffic: %s, illegal options %x", getSName (), flags);
		DstMLE = FIXED;
	}
	
	if (flags & BIT_exp) {
		Assert (! (flags & (BIT_unf | BIT_fix)),
			"Traffic: %s, illegal options %x", getSName (), flags);
		DstBIT = EXPONENTIAL;
	}
	if (flags & BIT_unf) {
		Assert (! (flags & (BIT_exp | BIT_fix)),
			"Traffic: %s, illegal options %x", getSName (), flags);
		DstBIT = UNIFORM;
	}
	if (flags & BIT_fix) {
		Assert (! (flags & (BIT_exp | BIT_unf)),
			"Traffic: %s, illegal options %x", getSName (), flags);
		DstBIT = FIXED;
	}
	
	if (flags & BSI_exp) {
		Assert (! (flags & (BSI_unf | BSI_fix)),
			"Traffic: %s, illegal options %x", getSName (), flags);
		DstBSI = EXPONENTIAL;
	}
	if (flags & BSI_unf) {
		Assert (! (flags & (BSI_exp | BSI_fix)),
			"Traffic: %s, illegal options %x", getSName (), flags);
		DstBSI = UNIFORM;
	}
	if (flags & BSI_fix) {
		Assert (! (flags & (BSI_exp | BSI_unf)),
			"Traffic: %s, illegal options %x", getSName (), flags);
		DstBSI = FIXED;
	}

	if (flags & SCL_on) {
		Assert (! (flags & SCL_off),
			"Traffic: %s, illegal options %x", getSName (), flags);
		FlgACT = FlgSCL = ON;
	}
	if (flags & SCL_off) {
		Assert (! (flags & SCL_on),
			"Traffic: %s, illegal options %x", getSName (), flags);
		FlgACT = FlgSCL = OFF;
	}

	if (flags & SPF_on) {
		Assert (! (flags & SPF_off),
			"Traffic: %s, illegal options %x", getSName (), flags);
		FlgSPF = ON;
	}
	if (flags & SPF_off) {
		Assert (! (flags & SPF_on),
			"Traffic: %s, illegal options %x", getSName (), flags);
		FlgSPF = OFF;
	}
};

void    Traffic::setup (CGroup **cgl, int ncgl, int flags, ...) {

/* ---------------------------- */
/* Initialize a traffic pattern */
/* ---------------------------- */

        VA_TYPE      ap;    // For processing variable-length argument list 

	Assert (ncgl >= 0, "Traffic: %s, illegal group count %1d",
		getSName (), ncgl);

	if (ncgl) {
		// The cluster is nonempty
		CGCluster = new CGroup* [ncgl];
		for (int i = 0; i < ncgl; i++) CGCluster [i] = cgl [i];
	} else
		CGCluster = NULL;

	NCGroups = ncgl;

	zz_setup_flags (flags);

	va_start (ap, flags);   // Start variable-length argument processing

	if (DstMIT != UNDEFINED) {
		ParMnMIT = va_arg (ap, double) * Etu;
		if (DstMIT == UNIFORM) {
			ParMxMIT = va_arg (ap, double) * Etu;
		} else {
			ParMxMIT = ParMnMIT;
		}
	}

	if (DstMLE != UNDEFINED) {
		ParMnMLE = va_arg (ap, double);
		if (DstMLE == UNIFORM) {
			ParMxMLE = va_arg (ap, double);
		} else {
			ParMxMLE = ParMnMLE;
		}
	}

	if (DstBIT != UNDEFINED) {
		ParMnBIT = va_arg (ap, double) * Etu;
		if (DstBIT == UNIFORM) {
			ParMxBIT = va_arg (ap, double) * Etu;
		} else {
			ParMxBIT = ParMnBIT;
		}
	}

	if (DstBSI != UNDEFINED) {
		ParMnBSI = va_arg (ap, double);
		if (DstBSI == UNIFORM) {
			ParMxBSI = va_arg (ap, double);
		} else {
			ParMxBSI = ParMnBSI;
		}
	}

	va_end (ap);
}

void    Traffic::setup (int flags, ...) {

/* ---------------------------------------------------------- */
/* Initialize  a  traffic pattern without groups. They can be */
/* later defined dynamically.                                 */
/* ---------------------------------------------------------- */

        VA_TYPE      ap;    // For processing variable-length argument list 

	// Initialize group list
	CGCluster = NULL;
	NCGroups = 0;

	zz_setup_flags (flags);

	va_start (ap, flags);   // Start variable-length argument processing

	if (DstMIT != UNDEFINED) {
		ParMnMIT = va_arg (ap, double) * Etu;
		if (DstMIT == UNIFORM) {
			ParMxMIT = va_arg (ap, double) * Etu;
		} else {
			ParMxMIT = ParMnMIT;
		}
	}

	if (DstMLE != UNDEFINED) {
		ParMnMLE = va_arg (ap, double);
		if (DstMLE == UNIFORM) {
			ParMxMLE = va_arg (ap, double);
		} else {
			ParMxMLE = ParMnMLE;
		}
	}

	if (DstBIT != UNDEFINED) {
		ParMnBIT = va_arg (ap, double) * Etu;
		if (DstBIT == UNIFORM) {
			ParMxBIT = va_arg (ap, double) * Etu;
		} else {
			ParMxBIT = ParMnBIT;
		}
	}

	if (DstBSI != UNDEFINED) {
		ParMnBSI = va_arg (ap, double);
		if (DstBSI == UNIFORM) {
			ParMxBSI = va_arg (ap, double);
		} else {
			ParMxBSI = ParMnBSI;
		}
	}

	va_end (ap);
}

void    Traffic::setup (CGroup *cg, int flags, ...) {

/* ---------------------------------------------------- */
/* Initialize a traffic pattern (single CGroup version) */
/* ---------------------------------------------------- */

        VA_TYPE      ap;    // For processing variable-length argument list 

	CGCluster = new CGroup* [1];
	CGCluster [0] = cg;
	NCGroups = 1;

	zz_setup_flags (flags);

	va_start (ap, flags);   // Start variable-length argument processing
	if (DstMIT != UNDEFINED) {
		ParMnMIT = va_arg (ap, double) * Etu;
		if (DstMIT == UNIFORM) {
			ParMxMIT = va_arg (ap, double) * Etu;
		} else {
			ParMxMIT = ParMnMIT;
		}
	}

	if (DstMLE != UNDEFINED) {
		ParMnMLE = va_arg (ap, double);
		if (DstMLE == UNIFORM) {
			ParMxMLE = va_arg (ap, double);
		} else {
			ParMxMLE = ParMnMLE;
		}
	}

	if (DstBIT != UNDEFINED) {
		ParMnBIT = va_arg (ap, double) * Etu;
		if (DstBIT == UNIFORM) {
			ParMxBIT = va_arg (ap, double) * Etu;
		} else {
			ParMxBIT = ParMnBIT;
		}
	}

	if (DstBSI != UNDEFINED) {
		ParMnBSI = va_arg (ap, double);
		if (DstBSI == UNIFORM) {
			ParMxBSI = va_arg (ap, double);
		} else {
			ParMxBSI = ParMnBSI;
		}
	}

	va_end (ap);
}

void    Traffic::preprocess_weights () {

/* ---------------------------------------------------------- */
/* Goes  through  the communication groups and calculates new */
/* weights, so that interpolation can  be  used  to  generate */
/* senders and receivers                                      */
/* ---------------------------------------------------------- */

	CGroup  *cg;
	SGroup  *sg;
	int     k, j;
	double  w;

	TCGWeight = 0.0;
	for (j = 0; j < NCGroups; j++) {
		cg = CGCluster [j];
		if (cg->SWeights == NULL) {
			// Equal weights of 1/n
			TCGWeight += 1.0;
			cg->CGWeight = TCGWeight;
		} else {
			sg = cg->Senders;
			for (cg->CGWeight = 0.0, k = 0; k < sg->NStations; k++)
				// Convert weights to density
				cg->SWeights [k] = (cg->CGWeight +=
					cg->SWeights [k]);
			cg->CGWeight = (TCGWeight += cg->CGWeight);
		}

		// Take care of receivers
		if (ptrToLong (cg->RWeights) == -1 || cg->RWeights == NULL)
			continue;
		sg = cg->Receivers;
		for (w = 0.0, k = 0; k < sg->NStations; k++)
			// Convert weights to density
			cg->RWeights [k] = (w += cg->RWeights [k]);
	}
}

TIME    Traffic::genMIT () {

/* ----------------------------------- */
/* Generates message interarrival time */
/* ----------------------------------- */

	assert (DstMIT != UNDEFINED, "Traffic->genMIT: %s, MIT distribution "
		"is undefined", getSName ());
	if (DstMIT == EXPONENTIAL)
		return (tRndPoisson (ParMnMIT));
	else if (DstMIT == FIXED)
		return ((TIME) ParMnMIT);
	else
		return (tRndUniform (ParMnMIT, ParMxMIT));
}

TIME    Traffic::genBIT () {

/* --------------------------------- */
/* Generates burst interarrival time */
/* --------------------------------- */

	assert (DstBIT != UNDEFINED, "Traffic->genBIT: %s, BIT distribution "
		"is undefined", getSName ());
	if (DstBIT == EXPONENTIAL)
		return (tRndPoisson (ParMnBIT));
	else if (DstBIT == FIXED)
		return ((TIME) ParMnBIT);
	else
		return (tRndUniform (ParMnBIT, ParMxBIT));
}

Long    Traffic::genMLE () {

/* ------------------------ */
/* Generates message length */
/* ------------------------ */

	Long    ml;

	assert (DstMLE != UNDEFINED, "Traffic->genMLE: %s, MLE distribution "
		"is undefined", getSName ());
	if (DstMLE == EXPONENTIAL)
		ml = lRndPoisson (ParMnMLE);
	else if (DstMLE == FIXED)
		ml = (Long) ParMnMLE;
	else
		ml = lRndUniform (ParMnMLE, ParMxMLE);
	if (ml < 8)
		ml = 8;                 // At least one byte
	else
		ml = (ml + 4) & ~07;    // Round up to bytes
	return (ml);
}

Long    Traffic::genBSI () {

/* -------------------- */
/* Generates burst size */
/* -------------------- */

	assert (DstBSI != UNDEFINED, "Traffic->genBSI: %s, BSI distribution "
		"is undefined", getSName ());
	if (DstBSI == EXPONENTIAL)
		return (lRndPoisson (ParMnBSI));
	else if (DstBSI == FIXED)
		return ((Long) ParMnBSI);
	else
		return (lRndUniform (ParMnBSI, ParMxBSI));
}

static  int select_station (int ns, short *idents, float *weights, double w) {

/* ---------------------------------------------------------- */
/* Selects  one  station  from  the  group  according  to the */
/* weights                                                    */
/* ---------------------------------------------------------- */

	int             j, es, ef;
	double          l, u;

	if (weights == NULL) {
		// All stations have equal weights of 1/n
		if (ns == 0) ns = (int) NStations;
		assert ((w < 1.01) && (w > -0.01),
			"Traffic->genXXX: bad weight %f", w);
		if ((j = (int) (ns * w)) >= ns) j = ns - 1; else
			if (j < 0) j = 0;
	} else if (ns < 5) {
		// Do it "manually", if the number of stations is not too big
		for (j = 0, ns--; j < ns; j++) 
			if (weights [j] >= w) break;
	} else {
		// Interpolate
		l = weights [es = 0];
		u = weights [ef = ns - 1];

		while (YES) {

			// Interpolation loop
			if ((j = (int)(((w - l) / (u - l)) * (ef - es) +
				es + 1)) < es)
					j = es;
			else if (j > ef) j = ef;

			if (weights [j] < w) {
				// Search on the right
				if ((es = j + 1) >= ef) {
					j = ef;
					break;
				}
				l = weights [es];
			} else if ((j <= es) || (weights [j - 1] < w)) {
				break;
			} else {
				// Search on the left
				if ((ef = j - 1) <= es) {
					j = es;
					break;
				}
				u = weights [ef];
			}
		}
	}

      return ((idents == NULL) ? j : (int)(idents [j]));
}

Long Traffic::genSND () {

/* --------------- */
/* Generate sender */
/* --------------- */

	CGroup  *cg;
	int     j, es, ef;
	double  w, l, u;

	if (NCGroups == 0) return (LastSender = NONE); // Nothing to do

	w = rnd (SEED_traffic) * TCGWeight;

	// Determine the group; do it "manually", if the number of groups
	// is not too big

	if (NCGroups < 5) {
		for (j = 0, cg = CGCluster [0]; j < NCGroups; j++)
			if ((cg = CGCluster [j]) -> CGWeight >= w) goto FI;
			// Please, don't tell me that goto's are not nice.
			// I've been told this nonsense long enough.
		cg = CGCluster [--j];
FI: ;
	} else {
		// Too many groups -- interpolate
		l = CGCluster[0]->CGWeight;
		u = TCGWeight;                  // Bounds
		es = 0;
		ef = NCGroups - 1;
		while (YES) {
			// Interpolation loop
			j = (int)(((w - l) / (u - l)) * (ef - es) + es + 1);
			if (j > ef) j = ef; else if (j < es) j = es;
			if ((cg = CGCluster [j]) -> CGWeight < w) {
				// Search on the right
				if ((es = j+1) >= ef) {
					cg = CGCluster [j = ef];
					break;
				}
				l = CGCluster [es] -> CGWeight;
			} else if ((j <= es) || (CGCluster [j-1] -> CGWeight <
				w)) {
					break;
			} else {
				// Search on the left
				if ((ef = j-1) <= es) {
					cg = CGCluster [j = es];
					break;
				}
				u = CGCluster [ef] -> CGWeight;
			}
		}
	}

	if (j) w -= CGCluster [j-1] -> CGWeight;

	// Set LastSender and LastCGroup to be used by genRCV. genRCV
	// must select the receiver from the same group and the receiver must
	// be different from the sender

	LastSender = select_station ((cg->Senders)->NStations,
		(cg->Senders)->SIdents, cg->SWeights, w);
	LastCGroup = cg;
	return (LastSender);
}

static  int     sender_index;       // Set by 'within_group'

CGroup *Traffic::genCGR (Long sender) {

/* ---------------------------------------------------------- */
/* Generate  a  selection  group  containing the given sender */
/* (according  to  the  probability  of  the  sender's  being */
/* selected from that group)                                  */
/* ---------------------------------------------------------- */

	double  sp [EETSIZE];
	CGroup  *cg;
	int     j, ng;
	double  tsw, w, l;

	if (NCGroups == 0) {
		LastSender = NONE;
		LastCGroup = NULL;
		return (NULL);
	}

	// Determine the probability of the sender in each selection group
	for (j = 0, tsw = 0.0, ng = NCGroups; j < ng; j++) {
		cg = CGCluster [j];

		if ((cg->Senders)->occurs (sender)) {
			if (sender_index == NONE)       // All stations
				l = 1.0 / (double) NStations;
			else if (cg->SWeights == NULL)
				l = ((cg->Senders)->NStations) ?
					1.0/((cg->Senders)->NStations) :
						0;
			
			else if (sender_index)
				l = cg->SWeights [sender_index] -
					cg->SWeights [sender_index-1];
			else
				l = cg->SWeights [0];
		} else
			l = 0.0;

		sp [j] = (tsw += l);
	}

	w = rnd (SEED_traffic) * tsw;

	// Determine the group

	for (j = 0; j < ng; j++)
		if (sp [j] >= w) {
			LastSender = sender;
			LastCGroup = CGCluster [j];
			break;
		}
	return (LastCGroup);
}

IPointer    Traffic::genRCV () {

/* ------------------- */
/* Generate a receiver */
/* ------------------- */

	SGroup  *receivers;
	int     ns, i, nr, f, l;
	double  w, wbs, was, wg;

	if (LastSender == NONE) {
		// The sender was not generated, so generate it first
		genSND ();
		if (LastSender == NONE) return (NONE);
	}

	if (((receivers = LastCGroup -> Receivers) == NULL) ||
		((nr = receivers -> NStations) == 0)) {
		// No receiver
		return (NONE);

	}

	if ((IPointer) (LastCGroup->RWeights) == -1) {
		// A broadcast group -- return the whole group
		return ((IPointer) receivers);
	}

	// Select the receiver at random

	wg = (LastCGroup->RWeights == NULL) ? 1.0 :
		LastCGroup->RWeights [ nr - 1 ];
RCV_RETRY:
	for (i = 0; i < 10; i++) {
		// Give it 10 tries
		w = rnd (SEED_traffic) * wg;
		if ((ns = select_station (receivers->NStations,
			receivers->SIdents, LastCGroup->RWeights, w)) !=
				LastSender)
					return (ns);
	}

	// We have tried it 10 times and the receiver was always
	// the same as the sender. Now let us take a closer look
	// at what is going on.

	if (nr <= 1)
	    excptn ("genRCV: cannot generate receiver for Traffic %s",
		getSName ());

	// All stations - keep trying
	if (LastCGroup->RWeights == NULL) goto RCV_RETRY;

	// Eliminate the sender from the receiver list

	f = 0; l = nr-1;

	while (1) {

		// Find the sender. Note that it occurs exactly once
		// as all duplicates were removed when the group was
		// read from the input.

		assert (f <= l, "Traffic->genRCV: %s, sender not found",
			getSName ());

		i = (f + l) / 2;

		// We use bisection here. Note that the receivers are
		// sorted according to their id's.

		if (receivers -> SIdents [i] < LastSender) {
			f = i + 1;
		} else if (receivers -> SIdents [i] > LastSender) {
			l = i - 1;
		} else
			break;
	}

	// Total weight of receivers preceding the sender
	wbs = i ? LastCGroup -> RWeights [i - 1] : 0.0;

	// Total weight of receivers following the sender
	was = wg - LastCGroup -> RWeights [i];

	w = rnd (SEED_traffic) * (wbs + was);

	if ((w <= wbs) && i) {
		// Search on the left
		ns = select_station (i, receivers->SIdents,
			LastCGroup->RWeights, w);
	} else {
		// Search on the right
		ns = select_station (nr - i - 1,
			&(receivers->SIdents [i+1]),
				&(LastCGroup->RWeights [i+1]),
					w + LastCGroup->RWeights [i]);
	}
	return (ns);
}

Message  *Traffic::genMSG (Long sender, IPointer receiver, Long length) {

/* ---------------------------------------------------------- */
/* Generates  a  message  with  the  specified parameters and */
/* queues it at the end of the corresponding sender's  queue. */
/* A pointer to the message is returned as the function value */
/* ---------------------------------------------------------- */

	Message            *m;
	Station            *s;
	ZZ_REQUEST         *cw;
	ZZ_EVENT           *e;
	int                i;
#if ZZ_TAG
	int q;
#endif
	assert (length >= 0, "Traffic->genMSG: %s, negative length %1d",
		getSName (), length);

	s = idToStation (sender);
#if     ZZ_QSL
	// Check if any of the limits has been reached
	if (FlgSPF == ON) {
	    if (zz_NQMessages >= zz_QSLimit || NQMessages >= QSLimit ||
	      s->QSize >= s->QSLimit) {
		// Ignore this generation
		return ((Message*)NULL);
	    }
	    // These rightfully belong to stdpfmMQU (see stdpfmMTR), but
	    // stdpfmMQU knows nothing about the sender 's'
	    s->QSize++;
	}
#endif
	m = zz_mkm ();                          // Make the message

	m -> Receiver = receiver;
	m -> QTime = Time;
	m -> TP = (int) Id;
	m -> Length = length;

	// Put the message into queue

	if (s -> MQHead == NULL) {
		// The message queue does not exist yet -- create it
		s->MQHead = new Message* [NTraffics];
		s->MQTail = new Message* [NTraffics];
		for (i = 0; i < NTraffics; i++)
			s->MQHead [i] = s->MQTail [i] = NULL;
	}

	if (s->MQHead [Id] == NULL) {
		assert ((s->MQTail [Id] == NULL),
			"Traffic->genMSG: %s, message queue corrupted",
				getSName ());
		s->MQHead [Id] = s->MQTail [Id] = m;
		m->prev = mq_sen (s->MQHead [Id]);
	} else {
		assert (s->MQTail [Id] != NULL,
			"Traffic->genMSG: %s, message queue corrupted",
				getSName ());
		(s->MQTail [Id])->next = m;
		m->prev = s->MQTail [Id];
		s->MQTail [Id] = m;
	}

	m -> next = NULL;       // Make sure the queue is terminated properly

	// Restart processes waiting for message arrival

	for (cw = s -> CWList; cw != NULL; cw = cw -> next) {
		if (cw -> ai == Client) {
			if (cw->event_id == ARRIVAL ||
			    cw -> event_id == Id) {
				cw -> Info01 = (void*) m;
				cw -> Info02 = (void*) Id;
#if     ZZ_TAG
				cw -> when . set (Time);
				if ((q = (e = cw-> event) -> waketime .
				  cmp (cw -> when)) > 0 || (q == 0 && FLIP))
#else
				cw -> when = Time;
				if ((e = cw-> event) -> waketime > Time ||
					FLIP)
#endif
						e->new_top_request (cw);

			} else if (cw -> event_id == INTERCEPT) {

#if     ZZ_TAG
				cw -> when . set (Time);
#else
				cw -> when = Time;
#endif
				cw -> Info01 = (void*) m;
				cw -> Info02 = (void*) Id;

				assert (zz_pe == NULL,
					"Traffic->genMSG: %s, more than one "
						"wait for INTERCEPT",
							getSName ());

				zz_pe = (zz_pr = cw) -> event;
			}
		} else {
			if (((Traffic*)(cw->ai))->Id != Id) continue;
			if (cw -> event_id == INTERCEPT) {
#if     ZZ_TAG
				cw -> when . set (Time);
#else
				cw -> when = Time;
#endif
				cw -> Info01 = (void*) m;
				cw -> Info02 = (void*) Id;

				assert (zz_pe == NULL,
					"Traffic->genMSG: %s, more than one "
						"wait for INTERCEPT",
							getSName ());

				zz_pe = (zz_pr = cw) -> event;
			} else {
				cw -> Info01 = (void*) m;
				cw -> Info02 = (void*) Id;
#if     ZZ_TAG
				cw -> when . set (Time);
				if ((q = (e = cw-> event) -> waketime .
				  cmp (cw -> when)) > 0 || (q == 0 && FLIP))
#else
				cw -> when = Time;
				if ((e = cw-> event) -> waketime > Time ||
					FLIP)
#endif
						e->new_top_request (cw);
			}
		}
	}

	m->setup ();    // User-defined part
	spfmMQU (m);    // Performance
	return (m);
}

int     SGroup::occurs (Long sid) {

/* ----------------------------------------------- */
/* Determine if station id sid occurs in the group */
/* ----------------------------------------------- */

	int     nf, ns;
	short   *idents;

	if (! sorted) zz_sort_group (this);


	if (SIdents == NULL) {
		// All stations belong to this group
		assert (NStations == ::NStations,
			"SGroup: 'occurs (...)' corrupted station group");
		sender_index = NONE;
		return (YES);
	}

	if ((ns = NStations) < 5) {
		// Search sequentially
		for (idents = SIdents; ns--; )
			if (*idents++ == sid) return (YES);
		return (NO);
	}

	// Use bisection
	nf = 0; ns--;

	while (1) {

		// Find the current station id. Note that it occurs
		// at most once in the idents table.

		if (nf > ns) return (NO);

		sender_index = (nf + ns) / 2;

		// sender_index is defined as static above generate_group

		if (SIdents [sender_index] < sid) {
			nf = sender_index + 1;
		} else if (SIdents [sender_index] > sid) {
			ns = sender_index - 1;
		} else
			return (YES);
	}
}

void    Traffic::stdpfmMQU (Message *m) {

/* ---------------------------------------------------------- */
/* Updates  standard  performance measures after a message is */
/* queued at the sender                                       */
/* ---------------------------------------------------------- */

	assert (RVMLS != NULL,
		"Traffic->stdpfmMQU: %s, Client not started", getSName ());
	RVMLS->update (m->Length, 1);
	// Update counters
	NQMessages++;
	zz_NQMessages++;
	zz_NGMessages++;
	NQBits += m->Length;
	zz_NQBits += m->Length;
}

void    Traffic::stdpfmMTR (Packet *p) {

/* ---------------------------------------------------------- */
/* Updates  standard  performance  measures  after  the  last */
/* packet of a message is transmitted                         */
/* ---------------------------------------------------------- */

	assert (RVMAT != NULL,
		"Traffic->stdpfmMTR: %s, Client not started", getSName ());
	RVMAT->update (Itu * ((double) (Time - p->QTime)), 1);
	NQMessages--;
	zz_NQMessages--;
	NTMessages++;
	zz_NTMessages++;
#if     ZZ_QSL
	(zz_st [p->Sender]->QSize)--;
#endif
}

void    Traffic::stdpfmPTR (Packet *p) {

/* ---------------------------------------------------------- */
/* Updates  standard  performance  measures after a packet is */
/* transmitted                                                */
/* ---------------------------------------------------------- */

	assert (RVPAT != NULL,
		"Traffic->stdpfmPTR: %s, Client not started", getSName ());
	RVPAT->update (Itu * ((double) (Time - p->TTime)), 1);
	NQBits -= p->ILength;
	zz_NQBits -= p->ILength;
	NTBits += p->ILength;
	zz_NTBits += p->ILength;
	NTPackets++;
	zz_NTPackets++;
}

void    Traffic::stdpfmPRC (Packet *p) {

/* ---------------------------------------------------------- */
/* Updates  standard  performance  measures after a packet is */
/* received                                                   */
/* ---------------------------------------------------------- */

	assert (RVWMD != NULL,
		"Traffic->stdpfmPRC: %s, Client not started", getSName ());
	if (p->ILength > 0)
		// Weighted message delay
		RVWMD->update (Itu * ((double) (Time - p->QTime)), p->ILength);

	// Absolute packet delay
	assert (RVAPD != NULL,
		"Traffic->stdpfmPRC: %s, Client not started", getSName ());
	RVAPD->update (Itu * ((double) (Time - p->TTime)), 1);
	NRBits += p->ILength;
	zz_NRBits += p->ILength;
	NRPackets++;
	zz_NRPackets++;
}

void    Traffic::stdpfmMRC (Packet *p) {

/* ---------------------------------------------------------- */
/* Updates  standard  performance measures after a message is */
/* received                                                   */
/* ---------------------------------------------------------- */

	assert (RVAMD != NULL,
		"Traffic->stdpfmMRC: %s, Client not started", getSName ());
	RVAMD->update (Itu * ((double) (Time - p->QTime)), 1);
	NRMessages++;
	zz_NRMessages++;
}

Packet  *Traffic::zz_mkp () {

/* --------------------------------------- */
/* Creates a packet (used for tricks only) */
/* --------------------------------------- */

	Packet *p;

	p = new Packet;
	zz_mkpclean ();
	return (p);
}

void    zz_mkpclean () {

/* ---------------------------------------------------------- */
/* Cleans  up  after  zz_mkp.  The Packet constructor assumes */
/* that the packet is  a  statically  declared  buffer.  This */
/* function removes the buffer description from the temporary */
/* list.                                                      */
/* ---------------------------------------------------------- */

	assert (zz_ncpframe != NULL && zz_ncpframe->next == NULL,
		"Traffic::mkpclean: packet buffer declared outside station");

	delete (zz_ncpframe);
	zz_ncpframe = NULL;
}

Packet::Packet () {

/* --------------------------------------------------- */
/* Constructor for creating packet buffers at stations */
/* --------------------------------------------------- */

	new ZZ_PFItem (this);
}

ZZ_PFItem::ZZ_PFItem (Packet *p) {

/* ---------------------------------------------------------- */
/* Constructor  for  a  packet  buffer  description: puts the */
/* description into a temporary list                          */
/* ---------------------------------------------------------- */
ZZ_PFItem *it;

	(buf = p)->Flags = 0L;
	p->QTime = p->TTime = TIME_0;
	p->Sender = p->Receiver = NONE;
	p->ILength = p->TLength = 0;
	p->TP = NONE;
#if  ZZ_DBG
	p->Signature = NONE;
#endif
	next = NULL;
	if (zz_ncpframe == NULL)
		zz_ncpframe = this;
	else {

		for (it = zz_ncpframe; it->next != NULL; it = it->next);
		it->next = this;
	}
}

#if     ZZ_ASR
void    Packet::setup (Message *m) {
#else
void    Packet::setup (Message*) {
#endif

/* ---------------------------------------------------------- */
/* A  virtual  function  to  be  redefined  in a user-defined */
/* Packet subtype. Called when a packet  is  created  from  a */
/* message   and  allows  the  user  to  set  up  nonstandard */
/* attributes of the Packet subtype.                          */
/* ---------------------------------------------------------- */

	// Make sure that the user defines 'setup' for all extended
	// subtypes
	assert (sizeof (Message) == m->frameSize (),
	"Packet: 'setup (Message*)' undefined for an extended Packet subtype");
}

void    Packet::fill (Station *s, Station *r, Long tl, Long il, Long tp) {

/* ---------------------------------------------- */
/* Fills a raw packet with the specified contents */
/* ---------------------------------------------- */

	Sender = (s != NULL) ? s->Id : NONE;
	Receiver = (IPointer) ((r != NULL) ? r->Id : NONE);
	TLength = tl;
	assert (TLength >= 0, "Packet: 'fill' negative TLength %1d", TLength);
	if (il != NONE) {
		assert (il <= tl,
			"Packet: 'fill' ILength (%1d) > TLength (%1d)", il, tl);
		ILength = il;
	} else
		ILength = TLength;

	assert (!isTrafficId (tp),
		"Packet: 'fill' illegal TP (%1d) for a non-standard packet",
			tp);
	setFlag (Flags, PF_full);
	TP = (int) tp;
#if  ZZ_DBG
	Signature = NONE;
#endif
};

void    Packet::fill (Long s, IPointer r, Long tl, Long il, Long tp) {

/* ---------------------------------------------------------- */
/* Fills  a  raw  packet with the specified contents (another */
/* version)                                                   */
/* ---------------------------------------------------------- */

	assert (isStationId (s) || s == NONE,
		"Packet: 'fill' illegal sender id (%1d)", s);
	assert (isStationId (r) || r == NONE,
		"Packet: 'fill' illegal receiver id (%1d)", (Long) r);
	Sender = s;
	Receiver = r;
	TLength = tl;
	assert (TLength >= 0, "Packet: 'fill' negative TLength %1d", TLength);
	if (il != NONE) {
		assert (il <= tl,
			"Packet: 'fill' ILength (%1d) > TLength (%1d)", il, tl);
		ILength = il;
	} else
		ILength = TLength;

	assert (!isTrafficId (tp),
		"Packet: 'fill' illegal TP (%1d) for a non-standard packet",
			tp);
	setFlag (Flags, PF_full);
	TP = (int) tp;
#if  ZZ_DBG
	Signature = NONE;
#endif
};

#if  ZZ_DBG
static Long LastSignature = 0;
#endif

int Traffic::getPacket (Packet *p, Long min, Long max, Long frm) {

/* ---------------------------------------------------------- */
/* Acquires  a  packet (of "this" type) and puts it into `p'. */
/* Returns NO, if there is no packet of "this" type           */
/* ---------------------------------------------------------- */

	Message *m;
	Long    l;

	if_from_observer ("Traffic->getPacket: called from an observer");

	assert (p->isEmpty (), "Traffic->getPacket: %s, the buffer is full",
		getSName ());

	if (TheStation->MQHead == NULL || (m = TheStation->MQHead [Id]) ==
		NULL) {
		// No message queued
		return (NO);
	}

	assert (m->Length >= 0,
		"Traffic->getPacket: %s, negative message length %1d",
			getSName (), m->Length);

	p->QTime = m->QTime;
	if (p->QTime > p->TTime) p->TTime = p->QTime;
	p->Sender      = TheStation->Id;
	p->Receiver    = m->Receiver;
	p->TP          = (int)Id;

#if  ZZ_DBG
	p->Signature   = LastSignature++;
#endif

	if (max && (m -> Length > max))
		l = p->ILength = max;
	else
		l = p->ILength = m -> Length;

	p->Flags = 1 << PF_full;        // The only flag
	if (!isStationId (p->Receiver)) {
		// NOTE !!!!  This trick only works under the assumption that
		// an address cannot be confused with a (not-too-big) integer
		// number representing a station Id.
		if (p->Receiver != NONE) setFlag (p->Flags, PF_broadcast);
	}

	p->TLength = frm + ((l < min) ? min : l);
	Info02 = (void*) Id;
	p->zz_setptr (ptviptr);         // The virtual functions
	p->setup (m);                   // Private attributes

	if ((m->Length -= l) == 0) {
		// The last packet of the message
		setFlag (p->Flags, PF_last);
		Info01 = NULL;

		// Deallocate the message
		pool_out (m);
		if (TheStation->MQTail [Id] == m)
			// The last message being removed - adjust the tail
			TheStation->MQTail [Id] = NULL;
	
		delete (m);
	} else
		Info01 = (void*)m;
	return (YES);
}

int Traffic::getPacket (Packet *p, ZZ_MTTYPE tf, Long min, Long max, Long frm) {

/* ---------------------------------------------------------- */
/* Acquires a packet (of "this" type) whose message satisfies */
/* condition  'tf' and puts it into `p'. Returns NO, if there */
/* is no such packet                                          */
/* ---------------------------------------------------------- */

	Message *m;
	Long    l;

	if_from_observer ("Traffic->getPacket: called from an observer");

	assert (p->isEmpty (), "Traffic->getPacket: %s, the buffer is full",
		getSName ());

	if (TheStation->MQHead == NULL) {
		// No message queued
		return (NO);
	}

	for (m = TheStation->MQHead [Id]; m != NULL && !tf (m); m = m->next);

	if (m == NULL) {
		// No message satisfying 'tf'
		return (NO);
	}

	assert (m->Length >= 0,
		"Traffic->getPacket: %s, negative message length %1d",
			getSName (), m->Length);

	p->QTime = m->QTime;
	if (p->QTime > p->TTime) p->TTime = p->QTime;
	p->Sender      = TheStation->Id;
	p->Receiver    = m->Receiver;
	p->TP          = (int) Id;

#if  ZZ_DBG
	p->Signature   = LastSignature++;
#endif

	if (max && (m -> Length > max))
		l = p->ILength = max;
	else
		l = p->ILength = m -> Length;

	p->Flags = 1 << PF_full;        // The only flag
	if (!isStationId (p->Receiver)) {
		// NOTE !!!!  This trick only works under the assumption that
		// an address cannot be confused with a (not-too-big) integer
		// number representing a station Id.
		if (p->Receiver != NONE) setFlag (p->Flags, PF_broadcast);
	}

	p->TLength = frm + ((l < min) ? min : l);
	Info02 = (void*) Id;
	p->zz_setptr (ptviptr);         // The virtual functions
	p->setup (m);                   // Private attributes

	if ((m->Length -= l) == 0) {
		// The last packet of the message
		setFlag (p->Flags, PF_last);
		Info01 = NULL;

		pool_out (m);
		// Deallocate the message
		if (TheStation->MQTail [Id] == m)
			// The last message being removed - adjust the tail
			TheStation->MQTail [Id] =
				(TheStation->MQHead [Id] == NULL) ? NULL :
					m->prev;
	
		delete (m);
	} else
		Info01 = (void*)m;
	return (YES);
}

int     Traffic::isPacket () {

/* ---------------------------------------------------------- */
/* Returns  YES,  if  there  is a packet of "this" type to be */
/* acquired from the Client                                   */
/* ---------------------------------------------------------- */

	if (TheStation->MQHead == NULL || TheStation->MQHead [Id] == NULL) {
		Info01 = NULL;
		Info02 = (void*) NONE;
		return (NO);
	}

	Info01 = (void*) (TheStation->MQHead [Id]);
	Info02 = (void*) Id;
	return (YES);
};

int     Traffic::isPacket (ZZ_MTTYPE tf) {

/* ---------------------------------------------------------- */
/* Returns   YES,  if  there  is  a  packet  of  "this"  type */
/* satisfying 'tf' to be acquired from the Client             */
/* ---------------------------------------------------------- */

	Message *m;

	if (TheStation->MQHead == NULL) {
		Info01 = NULL;
		Info02 = (void*) NONE;
		return (NO);
	}

	for (m = TheStation->MQHead [Id]; m != NULL && !tf (m); m = m->next);

	if (m == NULL) {
		Info01 = NULL;
		Info02 = (void*) NONE;
		return (NO);
	}

	Info01 = (void*) m;
	Info02 = (void*) Id;
	return (YES);
};

int     zz_client::isPacket () {

/* ---------------------------------------------------------- */
/* Returns  YES,  if  there  is  a  packet  of any type to be */
/* acquired from the Client                                   */
/* ---------------------------------------------------------- */

	int     i;

	if (TheStation->MQHead == NULL) {
		Info01 = NULL;
		Info02 = (void*) NONE;
		return (NO);
	}

	for (i = 0; i < NTraffics; i++) {
		if (TheStation->MQHead [i] != NULL) {
			Info01 = (void*) (TheStation->MQHead [i]);
			Info02 = (void*) i;
			return (YES);
		}
	}

	Info01 = NULL;
	Info02 = (void*) NONE;
	return (NO);
};

int     zz_client::isPacket (ZZ_MTTYPE tf) {

/* ---------------------------------------------------------- */
/* Returns  YES,  if there is a packet of any type satisfying */
/* 'tf' to be acquired from the Client                        */
/* ---------------------------------------------------------- */

	int     i;
	Message *m;

	if (TheStation->MQHead == NULL) {
		Info01 = NULL;
		Info02 = (void*) NONE;
		return (NO);
	}

	for (i = 0; i < NTraffics; i++) {
		for (m = TheStation->MQHead [i]; m != NULL && !tf (m);
			m = m->next);
		if (m != NULL) {
			Info01 = (void*) (TheStation->MQHead [i]);
			Info02 = (void*) i;
			return (YES);
		}
	}

	Info01 = NULL;
	Info02 = (void*) NONE;
	return (NO);
};

int zz_client::getPacket (Packet *p, Long min, Long max, Long frm) {

/* ---------------------------------------------------------- */
/* Acquires  a  packet  (of  any  type) and puts it into `p'. */
/* Returns NO, if no message is queued at the station         */
/* ---------------------------------------------------------- */


	Message *m, *mm;
	Long    l;
	int     i, mtp, mtype;
	TIME    qt;

	if_from_observer ("Client->getPacket: called from an observer");
	assert (p->isEmpty (), "Client->getPacket: (at %s) the buffer is full",
		TheStation->getSName ());

	if (TheStation->MQHead == NULL) {
		return (NO);
	}

	if (NTraffics == 1) {
		// A special (and frequent) case worth speeding up
		if ((m = *(TheStation->MQHead)) == NULL) {
			return (NO);
		}
		mtype = 0;
	} else {
		// More message types - determine the earliest one
		for (mtp = i = (int) TOSS (NTraffics), qt = TIME_inf, m = NULL;
			i < NTraffics; i++) {
			// Circular search started at random
			if ((mm = TheStation->MQHead [i]) == NULL) continue;
			if (qt <= mm->QTime) continue;
			qt = (m = mm) -> QTime;
			mtype = i;
		}

		for (i = 0; i < mtp; i++) {
			// The second part of the search
			if ((mm = TheStation->MQHead [i]) == NULL) continue;
			if (qt <= mm->QTime) continue;
			qt = (m = mm) -> QTime;
			mtype = i;
		}

		if (m == NULL) {
			return (NO);
		}
	}

	assert (m->Length >= 0,
		"Client->getPacket: negative message length %1d", m->Length);

	p->QTime = m->QTime;
	if (p->QTime > p->TTime) p->TTime = p->QTime;
	p->Sender      = TheStation->Id;
	p->Receiver    = m->Receiver;
	p->TP          = mtype;

#if  ZZ_DBG
	p->Signature   = LastSignature++;
#endif

	if (max && (m -> Length > max))
		l = p->ILength = max;
	else
		l = p->ILength = m -> Length;

	p->Flags = 1 << PF_full;        // The only flag
	if (!isStationId (p->Receiver)) {
		// NOTE !!!!  This trick only works under the assumption that
		// an address cannot be confused with a (not-too-big) integer
		// number representing a station Id.
		if (p->Receiver != NONE) setFlag (p->Flags, PF_broadcast);
	}

	p->TLength = frm + ((l < min) ? min : l);
	Info02 = (void*) mtype;
	p->zz_setptr (idToTraffic (mtype)->ptviptr);
	p->setup (m);                   // Private attributes

	if ((m->Length -= l) == 0) {
		// The last packet of the message
		setFlag (p->Flags, PF_last);
		Info01 = NULL;

		// Deallocate the message
		pool_out (m);
		if (TheStation->MQTail [mtype] == m)
			// The last message being removed - adjust the tail
			TheStation->MQTail [mtype] = NULL;
	
		delete (m);
	} else
		Info01 = (void*)m;
	return (YES);
}

int zz_client::getPacket (Packet *p, ZZ_MTTYPE tf, Long min, Long max,
	Long frm) {

/* ---------------------------------------------------------- */
/* Acquires  a  packet  (of any type) whose message satisfies */
/* condition 'tf' and puts it into `p'.  Returns  NO,  if  no */
/* message satisfying 'tf' is queued at the station           */
/* ---------------------------------------------------------- */

	Message *m, *mm;
	Long    l;
	int     i, mtp, mtype;
	TIME    qt;

	if_from_observer ("Client->getPacket: called from an observer");
	assert (p->isEmpty (), "Client->getPacket: (at %s) the buffer is full",
		TheStation->getSName ());

	if (TheStation->MQHead == NULL) {
		return (NO);
	}

	if (NTraffics == 1) {
		// A special (and frequent) case worth speeding up
		for (m = *(TheStation->MQHead); m != NULL && !tf (m);
			m = m->next);
		if (m == NULL) {
			return (NO);
		}
		mtype = 0;
	} else {
		// More message types - determine the earliest one
		for (mtp = i = (int) TOSS (NTraffics), qt = TIME_inf, m = NULL;
			i < NTraffics; i++) {
			// Circular search started at random
			for (mm = TheStation->MQHead [i]; ; mm = mm->next) {
				if (mm == NULL || qt < mm->QTime) goto ES;
				if (tf (mm)) break;
			}
			qt = (m = mm) -> QTime;
			mtype = i;
		  ES:   continue;
		}

		for (i = 0; i < mtp; i++) {
			for (mm = TheStation->MQHead [i]; ; mm = mm->next) {
				if (mm == NULL || qt < mm->QTime) goto ET;
				if (tf (mm)) break;
			}
			qt = (m = mm) -> QTime;
			mtype = i;
		  ET:   continue;
		}

		if (m == NULL) {
			return (NO);
		}
	}

	assert (m->Length >= 0,
		"Client->getPacket: negative message length %1d", m->Length);

	p->QTime = m->QTime;
	if (p->QTime > p->TTime) p->TTime = p->QTime;
	p->Sender      = TheStation->Id;
	p->Receiver    = m->Receiver;
	p->TP          = mtype;

#if  ZZ_DBG
	p->Signature   = LastSignature++;
#endif

	if (max && (m -> Length > max))
		l = p->ILength = max;
	else
		l = p->ILength = m -> Length;

	p->Flags = 1 << PF_full;        // The only flag
	if (!isStationId (p->Receiver)) {
		// NOTE !!!!  This trick only works under the assumption that
		// an address cannot be confused with a (not-too-big) integer
		// number representing a station Id.
		if (p->Receiver != NONE) setFlag (p->Flags, PF_broadcast);
	}

	p->TLength = frm + ((l < min) ? min : l);
	Info02 = (void*) mtype;
	p->zz_setptr (idToTraffic (mtype)->ptviptr);
	p->setup (m);                   // Private attributes

	if ((m->Length -= l) == 0) {
		// The last packet of the message
		setFlag (p->Flags, PF_last);
		Info01 = NULL;

		// Deallocate the message
		pool_out (m);
		if (TheStation->MQTail [mtype] == m)
			// The last message being removed - adjust the tail
			TheStation->MQTail [mtype] =
				(TheStation->MQHead [mtype] == NULL) ? NULL :
					m->prev;
		delete (m);
	} else
		Info01 = (void*)m;
	return (YES);
}

int     Message::getPacket (Packet *p, Long min, Long max, Long frm) {

/* ---------------------------------------------------------- */
/* Acquires  a  packet from the specific  message and puts it */
/* into `p'. Always returns YES (for compatibility  with  the */
/* other versions of the function).                           */
/* ---------------------------------------------------------- */


	Long    l;

	if_from_observer ("Client->getPacket: called from an observer");
	assert (p->isEmpty (), "Client->getPacket: (at %s) the buffer is full",
		TheStation->getSName ());
	assert (Length < 0, "Client->getPacket: (at %s) negative message "
		"length %1d", TheStation->getSName (), Length);

	p->QTime = QTime;
	if (p->QTime > p->TTime) p->TTime = p->QTime;
	p->Sender      = TheStation->Id;
	p->Receiver    = Receiver;
	p->TP          = TP;

#if  ZZ_DBG
	p->Signature   = LastSignature++;
#endif

	if (max && (Length > max))
		l = p->ILength = max;
	else
		l = p->ILength = Length;

	p->Flags = 1 << PF_full;        // The only flag
	if (!isStationId (p->Receiver)) {
		// NOTE !!!!  This trick only works under the assumption that
		// an address cannot be confused with a (not-too-big) integer
		// number representing a station Id.
		if (p->Receiver != NONE) setFlag (p->Flags, PF_broadcast);
	}

	p->TLength = frm + ((l < min) ? min : l);
	Info02 = (void*) (p->TP);
	p->zz_setptr (idToTraffic (p->TP)->ptviptr);
	p->setup (this);               // Private attributes

	if ((Length -= l) == 0) {
		// The last packet of the message
		setFlag (p->Flags, PF_last);
		Info01 = NULL;

		// Deallocate the message
		if (prev != NULL) {
			// The message was taken from a queue
			assert ((TheStation->MQHead [p->TP] != NULL)
				&& (TheStation->MQTail [p->TP]
					!= NULL),
				"Message->getPacket: message queue corrupted");
			pool_out (this);
			if (TheStation->MQTail [p->TP] == this)
				// Adjust the tail pointer
				TheStation->MQTail [p->TP] =
					(TheStation->MQHead [p->TP] ==
						NULL) ? NULL : prev;
		}
		delete (this);
	} else
		Info01 = (void*)this;
	return (YES);
}

void    zz_client::receive (Packet *p
#if	ZZ_NOL
                                        , Link *l
#endif
                                                   ) {
	Traffic *tp;

#if ZZ_FLK
	assert (p->isValid (), "Client->receive: (at %s) the packet is damaged",
		TheStation->getSName ());
#endif
	assert (p->isMy (), "Client->receive: packet addressed not to this "
		"station (%s) but to %1d",
			TheStation->getSName (), p->Receiver);

#if	ZZ_NOL
	if (l != NULL) l->spfmPRC (p);
#endif
	if (p->isStandard ()) {

		tp = idToTraffic (p->TP);

		// After receiving a packet

		tp->spfmPRC (p);

		// Check if it is the last packet of a message

		if (p->isLast ()) {

			// After receiving a message

			tp->spfmMRC (p);
#if	ZZ_NOL
			if (l != NULL) l->spfmMRC (p);
#endif
		}
	}
}

#if	ZZ_NOR
void 	zz_client::receive (Packet *p, RFChannel *l) {

	Traffic *tp;

	assert (p->isMy (), "Client->receive: packet addressed not to this "
		"station (%s), but to %1d",
			TheStation->getSName (), p->Receiver);

	l->spfmPRC (p);

	if (p->isStandard ()) {

		tp = idToTraffic (p->TP);

		// After receiving a packet

		tp->spfmPRC (p);

		// Check if it is the last packet of a message
		if (p->isLast ()) {
			// After receiving a message
			tp->spfmMRC (p);
			l->spfmMRC (p);
		}
	}
}

#endif	/* ZZ_NOR */

void    zz_client::release (Packet *p) {

/* ---------------------------------------------------------- */
/* Executed by the sender after the packet `p'is successfully */
/* sent                                                       */
/* ---------------------------------------------------------- */

	Traffic         *mtype;

	if_from_observer ("release: called from an observer");

	assert (p->isFull (), "Client->release: (at %s) packet buffer is empty",
		TheStation->getSName ());
	assert (isTrafficId (p->TP),
		"Client->release: (at %s) illegal packet's Traffic Id %1d",
			TheStation->getSName (), p->TP);

	mtype = idToTraffic (p->TP);
	mtype -> spfmPTR (p);
	if (p->isLast ()) mtype -> spfmMTR (p);
	clearFlag (p->Flags, PF_full);
	p->TTime = Time;
}

int     Packet::isMy (Station *s) {

/* ---------------------------------------------------------- */
/* Determines  if  the  packet  is  addressed to this station */
/* (TheStation)                                               */
/* ---------------------------------------------------------- */

#if ZZ_FLK
	if (isHDamaged ()) return NO;
#endif
	if (s == NULL)
		s = TheStation;

	if (isStationId (Receiver)) {
		return (Receiver == s->Id);
	}
	if (Receiver == NONE) return (NO);

	// A broadcast packet
	return (((SGroup*)Receiver)->occurs (s->Id));
}

static  char    fg [4];

char    *Packet::zz_pflags () {

/* ---------------------------------------------------- */
/* Converts standard packet flags to a printable string */
/* ---------------------------------------------------- */

	int     i;

	i = 0;
	if (flagSet (Flags, PF_broadcast))
		fg [i++] = 'B';
	if (flagSet (Flags, PF_last))
		fg [i++] = 'L';
	fg [i] = '\0';
	return (fg);
}

double zz_client::throughput () {

	TIME dt;

	dt = Time - zz_GSMTime;
	if (dt == TIME_0)
		return 0.0;

	return Etu * ((double)zz_NRBits / dt);
}

double Traffic::throughput () {

	TIME dt;

	dt = Time - zz_GSMTime;
	if (dt == TIME_0)
		return 0.0;

	return Etu * ((double)NRBits / dt);
}

sexposure (zz_client)

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr, (int) SId);    // Requests

		sexmode (1)

			exPrint1 (Hdr);               // Performance

		sexmode (2)

			exPrint2 (Hdr, (int) SId);    // Queues

		sexmode (3)

			exPrint3 (Hdr);               // Definitions (traffic)
	}

	sonscreen {

		sfxmode (0)

			exDisplay0 ((int) SId);       // Requests

		sexmode (1)

			exDisplay1 ();                // Performance

		sexmode (2)

			exDisplay2 ((int) SId);       // Queues

		sexmode (3)

			exDisplay3 ();          // Sent-received statistics

	}
}

void    zz_client::exPrint0 (const char *hdr, int sid) {

/* --------------------- */
/* Print Client requests */
/* --------------------- */

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

	Ouf << "    Process/Idn     CState      Event         AI/Idn" <<
		"      State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && (e->station == NULL ||
			e->chain == NULL || ident (e->station) != sid))
				continue;

		if ((r = e->chain) == NULL) {
			// System event
			if (zz_flg_nosysdisp || e->ai->Class != AIC_client)
				continue;
			if (!isStationId (sid))
				ptime (e->waketime, 11);
			else
				ptime (e->waketime, 15);

			Ouf << "* --- ";
			print (e->process->getTName (), 10);
			Ouf << form ("/%3d ", zz_trunc (e->process->Id, 3));
			// State name
			print (e->process->zz_sn (e->pstate), 10);

			Ouf << ' ';
			print (e->ai->zz_eid (e->event_id), 10);

			Ouf << ' ';
			print (e->ai->getTName (), 10);
			if ((l = e->ai->zz_aid ()) != NONE)
				Ouf << form ("/%03d ", zz_trunc (l, 3));
			else
				Ouf << "     ";

			print (e->process->zz_sn (e->pstate), 10);

			Ouf << '\n';
		} else {
			
			// A station event

			while (1) {

				if (r->ai->Class == AIC_client ||
					r->ai->Class == AIC_traffic) {

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
					Ouf << form ("/%3d ",
						zz_trunc (e->process->Id, 3));
					print (e->process->zz_sn (r->pstate),
						10);

					Ouf << ' ';
					print (r->ai->zz_eid (r->event_id),
						10);

					Ouf << ' ';

					print (e->ai->getTName (), 10);
					if ((l = e->ai->zz_aid ()) != NONE)
						Ouf << form ("/%03d ",
							zz_trunc (l, 3));
					else
						Ouf << "     ";

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

void    zz_client::exPrint1 (const char *hdr) {

/* -------------------------- */
/* Print performance measures */
/* -------------------------- */

	int             l, i;
	ZZ_TRVariable   *trv;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Global performance measures";
		Ouf << ":\n\n";
	}

	// AMD

	for (i = 0; i < NTraffics; i++) {
		Assert (idToTraffic (i)->FlgSPF == OFF ||
		    idToTraffic (i) -> RVAMD != NULL,
			"Client->exposure: Client not started");
	}

	tcreate (TYPE_long);
	for (i = l = 0; i < NTraffics; i++) {
		if (idToTraffic(i)->FlgSPF == OFF) continue;
		if (l == 0) {
			l++;
			*trv = *((ZZ_TRVariable*) ((idToTraffic (i)->RVAMD)));
			continue;
		}
		combineRV (idToTraffic (i)->RVAMD, trv);
	}
	if (l) trv->printCnt ("AMD - Absolute message delay:");
	delete (trv);

	// APD

	tcreate (TYPE_long);
	for (i = l = 0; i < NTraffics; i++) {
		if (idToTraffic(i)->FlgSPF == OFF) continue;
		if (l == 0) {
			l++;
			*trv = *((ZZ_TRVariable*) ((idToTraffic (i)->RVAPD)));
			continue;
		}
		combineRV (idToTraffic (i)->RVAPD, trv);
	}
	if (l) trv->printCnt ("APD - Absolute packet delay:");
	delete (trv);

	// WMD

	tcreate (TYPE_BITCOUNT);
	for (i = l = 0; i < NTraffics; i++) {
		if (idToTraffic(i)->FlgSPF == OFF) continue;
		if (l == 0) {
			l++;
			*trv = *((ZZ_TRVariable*) ((idToTraffic (i)->RVWMD)));
			continue;
		}
		combineRV (idToTraffic (i)->RVWMD, trv);
	}
	if (l) trv->printCnt ("WMD - Weighted message delay:");
	delete (trv);

	// MAT

	tcreate (TYPE_long);
	for (i = l = 0; i < NTraffics; i++) {
		if (idToTraffic(i)->FlgSPF == OFF) continue;
		if (l == 0) {
			l++;
			*trv = *((ZZ_TRVariable*) ((idToTraffic (i)->RVMAT)));
			continue;
		}
		combineRV (idToTraffic (i)->RVMAT, trv);
	}
	if (l) trv->printCnt ("MAT - Message access time:");
	delete (trv);

	// PAT

	tcreate (TYPE_long);
	for (i = l = 0; i < NTraffics; i++) {
		if (idToTraffic(i)->FlgSPF == OFF) continue;
		if (l == 0) {
			l++;
			*trv = *((ZZ_TRVariable*) ((idToTraffic (i)->RVPAT)));
			continue;
		}
		combineRV (idToTraffic (i)->RVPAT, trv);
	}
	if (l) trv->printCnt ("PAT - Packet access time:");
	delete (trv);

	// MLS

	tcreate (TYPE_long);
	for (i = l = 0; i < NTraffics; i++) {
		if (idToTraffic(i)->FlgSPF == OFF) continue;
		if (l == 0) {
			l++;
			*trv = *((ZZ_TRVariable*) ((idToTraffic (i)->RVMLS)));
			continue;
		}
		combineRV (idToTraffic (i)->RVMLS, trv);
	}
	if (l) trv->printCnt ("MLS - Message length distribution:");
	delete (trv);

	print (zz_NGMessages, "    Number of generated messages:   ");
	print (zz_NQMessages, "    Number of queued messages:      ");
	print (zz_NTMessages, "    Number of transmitted messages: ");
	print (zz_NRMessages, "    Number of received messages:    ");
	print (zz_NTPackets , "    Number of transmitted packets:  ");
	print (zz_NRPackets , "    Number of received packets:     ");
	print (zz_NQBits    , "    Number of queued bits:          ");
	print (zz_NTBits    , "    Number of transmitted bits:     ");
	print (zz_NRBits    , "    Number of received bits:        ");
	print (zz_GSMTime   , "    Measurement start time:         ");
	print (throughput (),
		"    Throughput:                     ");

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    zz_client::exPrint2 (const char *hdr, int sid) {

/* -------------------- */
/* Print message queues */
/* -------------------- */

	int             l, i;
	LONG            mc, bc;
	Message         *m, **mm;
	TIME            t;

	if (hdr != NULL) {
		Ouf << hdr;
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Message queues at";
		if (isStationId (sid))
			Ouf << ' ' << idToStation (sid)->getOName ();
		else
			Ouf << " stations";
		Ouf << ':';
	}

	Ouf << "\n\n";

	if (isStationId (sid)) {

		// Station-specific printout

	Ouf << "                Time   Traffic     Length   Receiver\n";

		if (idToStation (sid)->MQHead != NULL) {

			mm = new Message* [NTraffics];
			for (i = 0; i < NTraffics; i++)
				mm [i] = idToStation (sid)->MQHead [i];


			while (1) {

				for (m = NULL, i = l = 0, t = TIME_inf;
					i < NTraffics; i++) {

					if (mm [i] == NULL) continue;
					if (mm [i] -> QTime < t)
						t = (m = mm [l = i])->QTime;
				}

				if (m == NULL) break;

				mm [l] = m->next;

				print (m->QTime, 20);
				print (l, 10);
				print (m->Length, 11);
				if (isStationId (m->Receiver))
					print (m->Receiver, 11);
				else if (m->Receiver == NONE)
					print ("none", 11);
				else
					print ("bcast", 11);

				Ouf << '\n';
			}

			delete [] mm;

		}

	} else {

		Ouf << "Station   Messages       Bits\n";

		for (i = 0; i < NStations; i++) {
			mc = (LONG)(idToStation (i) -> getMQSize (NONE));
			bc = (LONG)(idToStation (i) -> getMQBits (NONE));
			print (i, 7);
			print (mc, 11);
			print (bc, 11);
			Ouf << '\n';
		}
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    zz_client::exPrint3 (const char *hdr) {

/* --------------------------------- */
/* Print traffic pattern definitions */
/* --------------------------------- */

	int             i;

	if (hdr != NULL) {
		Ouf << hdr;
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Traffic definitions:";
	}

	Ouf << "\n\n";

	for (i = 0; i < NTraffics; i++) {

		char    tt [64];

		strcpy (tt, form ("%s:", idToTraffic (i) -> getOName ()));

		idToTraffic (i) -> printDef (tt);
	}

	Ouf << "(" << getOName () << ") End of list\n\n";
}

void    zz_client::exDisplay0 (int sid) {

/* ----------------------- */
/* Display Client requests */
/* ----------------------- */

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;
	Long               l;

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && (e->station == NULL ||
			e->chain == NULL || ident (e->station) != sid))
				continue;

		if ((r = e->chain) == NULL) {
			// System event
			if (zz_flg_nosysdisp || e->ai->Class != AIC_client)
				continue;
			dtime (e->waketime);
			display ('*');
			display ("---");
			display (e->process->getTName ());
			display (e->process->Id);
			display (e->process->zz_sn (e->pstate));
			display (e->ai->zz_eid (e->event_id));
			display (e->ai->getTName ());
			if ((l = e->ai->zz_aid ()) != NONE)
				display (l);
			else
				display (' ');

			display (e->process->zz_sn (e->pstate));

		} else {
			
			// A station event

			while (1) {
				if (r->ai->Class == AIC_client ||
					r->ai->Class == AIC_traffic) {

					dtime (r->when);
					if (pless (e->waketime, r->when))
						display (' ');
					else if (e->chain == r)
						display ('*');
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
					display (r->ai->zz_eid (r->event_id));
					display (e->ai->getTName ());
					if ((l = e->ai->zz_aid ()) != NONE)
						display (l);
					else
						display (' ');

					display (e->process->zz_sn
							    (e->chain->pstate));
				}
				if ((r = r->other) == e->chain) break;
			}
		}
	}
}

void    zz_client::exDisplay1 () {

/* ---------------------------- */
/* Display performance measures */
/* ---------------------------- */

	int             l, i;
	ZZ_TRVariable   *trv;

	// AMD

	for (i = 0; i < NTraffics; i++) {
		if (idToTraffic (i)->FlgSPF != OFF &&
			idToTraffic (i) -> RVAMD == NULL) {
				display (' ');
				return;         // Client not started yet
		}
	}

	tcreate (TYPE_long);
	for (i = l = 0; i < NTraffics; i++) {
		if (idToTraffic(i)->FlgSPF == OFF) continue;
		if (l == 0) {
			l++;
			*trv = *((ZZ_TRVariable*) ((idToTraffic (i)->RVAMD)));
			continue;
		}
		combineRV (idToTraffic (i)->RVAMD, trv);
	}
	if (l) {
		display ("AMD");
		trv->displayOut (2);    // Abbreviated mode - one line
	}
	delete (trv);

	// APD

	tcreate (TYPE_long);
	for (i = l = 0; i < NTraffics; i++) {
		if (idToTraffic(i)->FlgSPF == OFF) continue;
		if (l == 0) {
			l++;
			*trv = *((ZZ_TRVariable*) ((idToTraffic (i)->RVAPD)));
			continue;
		}
		combineRV (idToTraffic (i)->RVAPD, trv);
	}
	if (l) {
		display ("APD");
		trv->displayOut (2);
	}
	delete (trv);

	// WMD

	tcreate (TYPE_BITCOUNT);
	for (i = l = 0; i < NTraffics; i++) {
		if (idToTraffic(i)->FlgSPF == OFF) continue;
		if (l == 0) {
			l++;
			*trv = *((ZZ_TRVariable*) ((idToTraffic (i)->RVWMD)));
			continue;
		}
		combineRV (idToTraffic (i)->RVWMD, trv);
	}
	if (l) {
		display ("WMD");
		trv->displayOut (2);
	}
	delete (trv);

	// MAT

	tcreate (TYPE_long);
	for (i = l = 0; i < NTraffics; i++) {
		if (idToTraffic(i)->FlgSPF == OFF) continue;
		if (l == 0) {
			l++;
			*trv = *((ZZ_TRVariable*) ((idToTraffic (i)->RVMAT)));
			continue;
		}
		combineRV (idToTraffic (i)->RVMAT, trv);
	}
	if (l) {
		display ("MAT");
		trv->displayOut (2);
	}
	delete (trv);

	// PAT

	tcreate (TYPE_long);
	for (i = l = 0; i < NTraffics; i++) {
		if (idToTraffic(i)->FlgSPF == OFF) continue;
		if (l == 0) {
			l++;
			*trv = *((ZZ_TRVariable*) ((idToTraffic (i)->RVPAT)));
			continue;
		}
		combineRV (idToTraffic (i)->RVPAT, trv);
	}
	if (l) {
		display ("PAT");
		trv->displayOut (2);
	}
	delete (trv);
	
	// MLS

	tcreate (TYPE_long);
	for (i = l = 0; i < NTraffics; i++) {
		if (idToTraffic(i)->FlgSPF == OFF) continue;
		if (l == 0) {
			l++;
			*trv = *((ZZ_TRVariable*) ((idToTraffic (i)->RVMLS)));
			continue;
		}
		combineRV (idToTraffic (i)->RVMLS, trv);
	}
	if (l) {
		display ("MLS");
		trv->displayOut (2);
	}
	delete (trv);
}

void    zz_client::exDisplay3 () {

/* -------------------------------- */
/* Display sent-received statistics */
/* -------------------------------- */

	display (throughput ());
	display (zz_NGMessages);
	display (zz_NQMessages);
	display (zz_NTMessages);
	display (zz_NRMessages);
	display (zz_NTPackets );
	display (zz_NRPackets );
	display (zz_NQBits    );
	display (zz_NTBits    );
	display (zz_NRBits    );
}

void    zz_client::exDisplay2 (int sid) {

/* ---------------------- */
/* Display message queues */
/* ---------------------- */

	int             l, i;
	BITCOUNT        bc, maxbc;
	Message         *m, **mm;
	TIME            t;


	if (isStationId (sid)) {

		// Station-specific display

		if (idToStation (sid)->MQHead != NULL) {

			mm = new Message* [NTraffics];
			for (i = 0; i < NTraffics; i++)
				mm [i] = idToStation (sid)->MQHead [i];

			while (1) {

				for (m = NULL, i = l = 0, t = TIME_inf;
					i < NTraffics; i++) {

					if (mm [i] == NULL) continue;
					if (mm [i] -> QTime < t)
						t = (m = mm [l = i])->QTime;
				}

				if (m == NULL) break;

				mm [l] = m->next;

				display (m->QTime);
				display (l);
				display (m->Length);
				if (isStationId (m->Receiver))
					display (m->Receiver);
				else if (m->Receiver == NONE)
					display ("none");
				else
					display ("bcast");
			}
			delete [] mm;
		}

		return;

	}

	// Graphical display for all stations

	maxbc = BITCOUNT_0;
	// Calculate the maximum
	for (i = 0; i < NStations; i++) {
		bc = idToStation (i)->getMQBits (NONE);
		if (maxbc < bc) maxbc = bc;
	}

	if (maxbc == BITCOUNT_0)
		maxbc = BITCOUNT_1;

	startRegion (0.0, (double)(NStations-1), 0.0, (double) maxbc);
	startSegment (00+4*3);       // Stripes (histograms)

	for (i = 0; i < NStations; i++) {
		bc = idToStation (i)->getMQBits (NONE);
		displayPoint ((double)i, (double)bc);
	}

	endSegment ();
	endRegion ();
}

sexposure (Traffic)

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr, (int) SId);    // Request queue

		sexmode (1)

			exPrint1 (Hdr);               // Performance

		sexmode (2)

			exPrint2 (Hdr, (int) SId);    // Queues

		sexmode (3)

			exPrint3 (Hdr);
	}

	sonscreen {

		sfxmode (0)

			exDisplay0 ((int) SId);       // Request queue

		sexmode (1)

			exDisplay1 ();                // Performance

		sexmode (2)

			exDisplay2 ((int) SId);       // Sent-received

		sexmode (3)

			exDisplay3 ();                // Queues
	}
}

void    Traffic::exPrint0 (const char *hdr, int sid) {

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
		Ouf << '(' << getOName () << ") Full AI wait list";
		if (isStationId (sid))
			Ouf << " for " << idToStation (sid)->getOName ();
		Ouf << ":\n\n";
	}

	if (!isStationId (sid))
		Ouf << "       Time   St";
	else
		Ouf << "           Time ";

	Ouf << "    Process/Idn     TState      Event         AI/Idn" <<
		"      State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && (e->station == NULL ||
			e->chain == NULL || ident (e->station) != sid))
				continue;

		if ((r = e->chain) == NULL) {
			// Generation event (system)
			if (zz_flg_nosysdisp || e->ai->Class != AIC_client ||
					ptrToLong (e->Info02) != Id) continue;

			if (!isStationId (sid))
				ptime (e->waketime, 11);
			else
				ptime (e->waketime, 15);

			Ouf << "* --- ";
			print (e->process->getTName (), 10);
			Ouf << form ("/%03d ",
					zz_trunc (e->process->Id, 3));
			// State name
			print (e->process->zz_sn (e->pstate), 10);

			Ouf << ' ';
			print (e->ai->zz_eid (e->event_id), 10);

			Ouf << ' ';

			print (e->ai->getTName (), 10);
			if ((l = e->ai->zz_aid ()) != NONE)
				Ouf << form ("/%03d ", zz_trunc (l, 3));
			else
				Ouf << "     ";

			print (e->process->zz_sn (e->pstate), 10);

			Ouf << '\n';
		} else {
			
			// A station event

			while (1) {

				if ((r->ai->Class == AIC_client &&
					r->event_id == Id) ||
							r->ai == this) {

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
					print (r->ai->zz_eid (r->event_id),
						10);

					Ouf << ' ';

					print (e->ai->getTName (), 10);
					if ((l = e->ai->zz_aid ()) != NONE)
						Ouf << form ("/%03d ",
							zz_trunc (l, 3));
					else
						Ouf << "     ";

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

void    Traffic::exPrint1 (const char *hdr) {

/* -------------------------- */
/* Print performance measures */
/* -------------------------- */

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Performance measures:\n\n";
	}

	if (FlgSPF == OFF) {
		Ouf << ">>> Switched off <<<\n";
	} else {

		Assert (RVAMD != NULL,
			"Traffic->exposure: %s, Client not started",
				getSName ());
		// AMD
		RVAMD->printCnt ("AMD - Absolute message delay:");
		// APD
		RVAPD->printCnt ("APD - Absolute packet delay:");
		// WMD
		RVWMD->printCnt ("WMD - Weighted message delay:");
		// MAT
		RVMAT->printCnt ("MAT - Message access time:");
		// PAT
		RVPAT->printCnt ("PAT - Packet access time:");
		// MLS
		RVMLS->printCnt ("MLS - Message length statistics:");
	}

	print (NTMessages + NQMessages, "    Number of generated messages:   ");
	print (NQMessages             , "    Number of queued messages:      ");
	print (NTMessages             , "    Number of transmitted messages: ");
	print (NRMessages             , "    Number of received messages:    ");
	print (NTPackets              , "    Number of transmitted packets:  ");
	print (NRPackets              , "    Number of received packets:     ");
	print (NQBits                 , "    Number of queued bits:          ");
	print (NTBits                 , "    Number of transmitted bits:     ");
	print (NRBits                 , "    Number of received bits:        ");
	Ouf << "\n";
	print (SMTime                 , "    Measurement start time:         ");


	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Traffic::exPrint2 (const char *hdr, int sid) {

/* -------------------- */
/* Print message queues */
/* -------------------- */

	int             i;
	LONG            mc, bc;
	Message         *m;

	if (hdr != NULL) {
		Ouf << hdr;
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Message queue";
		if (isStationId (sid))
			Ouf << " at " << idToStation (sid)->getOName ();
		else
			Ouf << "s at stations";
			
		Ouf << ':';
	}

	Ouf << "\n\n";

	if (isStationId (sid)) {

		// Station-specific printout

		Ouf << "                Time     Length   Receiver\n";

		if (idToStation (sid)->MQHead != NULL) {

			for (m = idToStation (sid)->MQHead [Id]; m != NULL;
				m = m->next) {

				print (m->QTime, 20);
				print (m->Length, 11);
				if (isStationId (m->Receiver))
					print (m->Receiver, 11);
				else if (m->Receiver == NONE)
					print ("none", 11);
				else
					print ("bcast", 11);

				Ouf << '\n';
			}
		}

	} else {

		Ouf << "Station   Messages       Bits\n";

		for (i = 0; i < NStations; i++) {
			mc = (LONG)(idToStation (i) -> getMQSize (Id));
			bc = (LONG)(idToStation (i) -> getMQBits (Id));
			print (i, 7);
			print (mc, 11);
			print (bc, 11);
			Ouf << '\n';
		}
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Traffic::exPrint3 (const char *hdr) {

/* -------------------------------- */
/* Print traffic pattern definition */
/* -------------------------------- */

	Long            l;
	int             i, j;
	double          gw, sw;
	CGroup          *cg;
	SGroup          *sg;

	if (hdr != NULL) {
		Ouf << hdr;
	} else  {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Definition:";
	}

	Ouf << "\n\n";

	print ("MIT distribution:      ");

	if (DstMIT == EXPONENTIAL) {
		print ("EXPONENTIAL (Mean = ");
		print (ParMnMIT/Etu, 8);
		Ouf << ')';
	} else if (DstMIT == UNIFORM) {
		print ("UNIFORM     (Min  = ");
		print (ParMnMIT/Etu, 8);
		print (", Max  = ");
		print (ParMxMIT/Etu, 8);
		Ouf << ')';
	} else if (DstMIT == FIXED) {
		print ("FIXED       (Val  = ");
		print (ParMnMIT/Etu, 8);
		Ouf << ')';
	} else
		print ("UNDEFINED");
	Ouf << '\n';

	print ("MLE distribution:      ");

	if (DstMLE == EXPONENTIAL) {
		print ("EXPONENTIAL (Mean = ");
		print (ParMnMLE, 8);
		Ouf << ')';
	} else if (DstMLE == UNIFORM) {
		print ("UNIFORM     (Min  = ");
		print (ParMnMLE, 8);
		print (", Max  = ");
		print (ParMxMLE, 8);
		Ouf << ')';
	} else if (DstMLE == FIXED) {
		print ("FIXED       (Val  = ");
		print (ParMnMLE, 8);
		Ouf << ')';
	} else
		print ("UNDEFINED");
	Ouf << '\n';

	if (DstBIT != UNDEFINED) {      // Bursty traffic

		print ("BIT distribution:      ");

		if (DstBIT == EXPONENTIAL) {
			print ("EXPONENTIAL (Mean = ");
			print (ParMnBIT/Etu, 8);
			Ouf << ')';
		} else if (DstBIT == UNIFORM) {
			print ("UNIFORM     (Min  = ");
			print (ParMnBIT/Etu, 8);
			print (", Max  = ");
			print (ParMxBIT/Etu, 8);
			Ouf << ')';
		} else if (DstBIT == FIXED) {
			print ("FIXED       (Val  = ");
			print (ParMnBIT/Etu, 8);
			Ouf << ')';
		} else
			print ("UNDEFINED");
		Ouf << '\n';

		print ("BSI distribution:      ");

		if (DstBSI == EXPONENTIAL) {
			print ("EXPONENTIAL (Mean = ");
			print (ParMnBSI, 8);
			Ouf << ')';
		} else if (DstBSI == UNIFORM) {
			print ("UNIFORM     (Min  = ");
			print (ParMnBSI, 8);
			print (", Max  = ");
			print (ParMxBSI, 8);
			Ouf << ')';
		} else if (DstBSI == FIXED) {
			print ("FIXED       (Val  = ");
			print (ParMnBSI, 8);
			Ouf << ')';
		} else
			print ("UNDEFINED");
		Ouf << '\n';
	}

	print ("Standard Client is:    ");
	print ((FlgSCL == ON) ? "ON" : "OFF");
	Ouf << '\n';

	print ("Standard measures are: ");
	print ((FlgSPF == ON) ? "ON" : "OFF");
	Ouf << "\n\n";

	for (i = 0, gw = 0.0; i < NCGroups; i++) {

		cg = CGCluster [i];
		Ouf << form ("Communication group #%03d (density %8.2g)\n\n",
			i, cg->CGWeight - gw);

		if (zz_flg_started) gw = cg->CGWeight;

		print ("Senders:   ");
		if ((sg = cg->Senders) == NULL || sg->NStations == NStations ||
			sg->SIdents == NULL) {

			l = NStations;
			print ("all stations");
		} else {
			l = sg->NStations;
			print (l, 5);
		}

		if (cg->SWeights == NULL) {
			print (", equal weights totaling to 1.0");
			if (l != NStations) {
				Ouf << '\n';
				for (j = 0; j < l; j++) {
					if (j % 10 == 0) Ouf << '\n';
					print (sg->SIdents [j], 6);
				}
			}
		} else {
			Ouf << '\n';
			for (j = 0, sw = 0.0; j < l; j++) {
				if (j % 4 == 0) Ouf << '\n';
				Ouf << '(';
				print ((l<NStations)? sg->SIdents [j] : j, 5);
				Ouf << ' ';
				print (cg->SWeights [j] - sw, 8);
				if (zz_flg_started) sw = cg->SWeights [j];
				Ouf << ")   ";
			}
		}

		Ouf << "\n\n";

		print ("Receivers: ");
		if ((sg = cg->Receivers) == NULL || sg->NStations == NStations
			|| sg->SIdents == NULL) {

			l = NStations;
			print ("all stations");
		} else {
			l = sg->NStations;
			print (l, 5);
		}

		if (cg->RWeights == NULL || cg->RWeights == (float*) NONE) {
			if (cg->RWeights == NULL)
				print (", equal weights totaling to 1.0");
			else
				print (", broadcast group");
			Ouf << '\n';
			if (l != NStations) {
				for (j = 0; j < l; j++) {
					if (j % 10 == 0) Ouf << '\n';
					print (sg->SIdents [j], 6);
				}
			}
		} else {
			Ouf << '\n';
			for (j = 0, sw = 0.0; j < l; j++) {
				if (j % 4 == 0) Ouf << '\n';
				Ouf << '(';
				print ((l<NStations)? sg->SIdents [j] : j, 5);
				Ouf << ' ';
				print (cg->RWeights [j] - sw, 8);
				if (zz_flg_started) sw = cg->RWeights [j];
				Ouf << ")   ";
			}
		}

		Ouf << "\n\n";
	}

	Ouf << "(" << getOName () << ") End of list\n\n";
}

void    Traffic::exDisplay0 (int sid) {

/* ------------------------- */
/* Display the request queue */
/* ------------------------- */

	ZZ_REQUEST         *r;
	ZZ_EVENT           *e;
	Long               l;

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && (e->station == NULL ||
			e->chain == NULL || ident (e->station) != sid))
				continue;

		if ((r = e->chain) == NULL) {
			// Generation event (system)
			if (zz_flg_nosysdisp || e->ai->Class != AIC_client ||
					ptrToLong (e->Info02) != Id) continue;

			dtime (e->waketime);
			display ('*');
			display ("---");
			display (e->process->getTName ());
			display (e->process->Id);
			display (e->process->zz_sn (e->pstate));
			display (e->ai->zz_eid (e->event_id));
			display (e->ai->getTName ());
			if ((l = e->ai->zz_aid ()) != NONE)
				display (l);
			else
				display (' ');

			display (e->process->zz_sn (e->pstate));

		} else {
			
			// A station event

			while (1) {
				if ((r->ai->Class == AIC_client &&
					r->event_id == Id) ||
							r->ai == this) {

					dtime (r->when);

					if (pless (e->waketime, r->when))
						display (' ');
					else if (e->chain == r)
						display ('*');
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
					display (r->ai->zz_eid (r->event_id));
					display (e->ai->getTName ());
					if ((l = e->ai->zz_aid ()) != NONE)
						display (l);
					else
						display (' ');

					display (e->process->zz_sn
							    (e->chain->pstate));
				}
				if ((r = r->other) == e->chain) break;
			}
		}
	}
}

void    Traffic::exDisplay1 () {

/* ---------------------------- */
/* Display performance measures */
/* ---------------------------- */

	if (FlgSPF == OFF) {
		display (' ');
		display ("OFF");
		return;
	}

	if (RVAMD == NULL) {            // Standard Client not started
		display (' ');
		return;
	}

	display ("AMD");
	RVAMD->displayOut (2);
	display ("APD");
	RVAPD->displayOut (2);
	display ("WMD");
	RVWMD->displayOut (2);
	display ("MAT");
	RVMAT->displayOut (2);
	display ("PAT");
	RVPAT->displayOut (2);
	display ("MLS");
	RVMLS->displayOut (2);
}

void    Traffic::exDisplay3 () {

/* -------------------------------- */
/* Display sent-received statistics */
/* -------------------------------- */

	display (NTMessages + NQMessages);
	display (NQMessages             );
	display (NTMessages             );
	display (NRMessages             );
	display (NTPackets              );
	display (NRPackets              );
	display (NQBits                 );
	display (NTBits                 );
	display (NRBits                 );
}

void    Traffic::exDisplay2 (int sid) {

/* ---------------------- */
/* Display message queues */
/* ---------------------- */


	int             i;
	BITCOUNT        bc, maxbc;
	Message         *m, **mm;

	if (isStationId (sid)) {

		// Station-specific display

		if ((mm = idToStation (sid)->MQHead) != NULL) {

			for (m = mm [Id]; m != NULL; m = m->next) {

				display (m->QTime);
				display (m->Length);
				if (isStationId (m->Receiver))
					display (m->Receiver);
				else if (m->Receiver == NONE)
					display ("none");
				else
					display ("bcast");
			}
		}

		return;
	}

	// Graphical display for all stations

	maxbc = BITCOUNT_0;
	// Calculate the maximum
	for (i = 0; i < NStations; i++) {
		bc = idToStation (i) -> getMQBits (Id);
		if (maxbc < bc) maxbc = bc;
	}

	if (maxbc == BITCOUNT_0) maxbc = BITCOUNT_1;

	startRegion (0.0, (double)(NStations-1), 0.0, (double) maxbc);
	startSegment (00+4*3);       // Stripes (histograms)

	for (i = 0; i < NStations; i++) {
		bc = idToStation (i) -> getMQBits (Id);
		displayPoint ((double)i, (double)bc);
	}

	endSegment ();
	endRegion ();
}

#endif /* NOC */
