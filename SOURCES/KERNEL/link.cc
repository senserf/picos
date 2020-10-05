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

#if	ZZ_NOL

/* ----------- */
/* The link AI */
/* ----------- */

#include        "system.h"

#define BIG_size BIG_precision

/* ---------------------------------------------------------- */
/* Types  and  static  data  used  by the activity processing */
/* functions                                                  */
/* ---------------------------------------------------------- */
struct  activity_heard_struct {         // Used by 'sort_acs_e'

	TIME    t_s;
	TIME    t_f;
	void    *up;                    // A general purpose fullword
};

struct  activity_descr_struct {         // Used by 'sort_acs_p'

	TIME               t_s;
	ZZ_LINK_ACTIVITY   *ba;
};

static  Process         *shandle = NULL;// Service process handle
static  Packet          *tpckt;         // Triggering packet (for ACTIVITY, BOT,
					// EOT, etc)
static  IPointer        rinfo;          // Used by wait functions
static  ZZ_LINK_ACTIVITY   *tact;       // Ditto
static  ZZ_LINK_ACTIVITY   *new_activity;

ZZ_LINK_ACTIVITY *zz_gen_activity (int lrid, Port *gport, int type, Packet *p) {

/* ---------------------------------------- */
/* A special contructor for link activities */
/* ---------------------------------------- */

	register ZZ_LINK_ACTIVITY  *a;

	if (p != NULL) {
		// Packet field needed
		a = (ZZ_LINK_ACTIVITY*) new
		char [sizeof (ZZ_LINK_ACTIVITY) - sizeof (Packet) +
		p->frameSize()];
		// Insert the packet
		a->Pkt = *p;
	} else {
		// No packet
		a = (ZZ_LINK_ACTIVITY*) new
		char [sizeof (ZZ_LINK_ACTIVITY) - sizeof (Packet)];
	}

	a->LRId = lrid;
	a->GPort = gport;
	a->STime = Time;
	a->FTime = TIME_inf;
	a->Type = type;
	// Will be queued explicitly
	return (a);
};

STATIC INLINE ZZ_LINK_ACTIVITY   *Port::initLAI () {

/* -------------------------------------------------------- */
/* Update last activity interpolator (LT_pointtopoint only) */
/* -------------------------------------------------------- */

	register ZZ_LINK_ACTIVITY  *a;

	if (IJTime <= Time) {

		// The interpolator activity may have been junked

		a = Lnk->Alive;
ZZ:
		for (; a != NULL; a = a->next) {
			// Skip upstream activities
			if (a->LRId > LRId) continue;
			// And those no longer heard
			if (undef (a->FTime)) {
				// Still being heard
				Interpolator = a;
				// Pessimistic estimate of time when a goes
				// to the archive
				IJTime = Time + MaxDistance;
				return (a);
			}

			// The activity has a definite FTime
			if (a->FTime + DV [a->LRId] < Time) continue;
			// And is no longer relevant

			// Still relevant -- use it
			Interpolator = a;
			// Exact time when the interpolator is junked
			IJTime = a->FTime + a->GPort->MaxDistance;
			return (a);
		}

		Interpolator = NULL;    // Unknown interpolator
		IJTime = TIME_0;
		return (NULL);

	} else {

		// The previous interpolator still alive
		if (def ((a = Interpolator)->FTime)) {
			if (a->FTime + DV [a->LRId] < Time) {
				// Irrelevant, skip and look ahead
				a = a->next;
				goto ZZ;
			} else {
				// Improve the junk time estimate
				IJTime = a->FTime + a->GPort->MaxDistance;
				return (a);
			}
		}
		return (Interpolator);
	}
}

STATIC  INLINE  ZZ_EVENT   *Port::findHint (ZZ_LINK_ACTIVITY *act) {

/* ---------------------------------------------------------- */
/* Finds    the   "hint"   event   for   a   given   activity */
/* (unidirectional links only)                                */
/* ---------------------------------------------------------- */

	register ZZ_LINK_ACTIVITY  *b, *f;
	register Long           i;

	// A heuristic limit for the number of tries: 1/4 of the activity
	// pool; can be adjusted experimentally, but who cares!
	i = ((Lnk->NAlive) >> 2) & 017777777776;

	for (f = (b = act)->prev; i;) {
		if (f != (ZZ_LINK_ACTIVITY*)(&(Lnk->Alive))) {
			// Try one step forward
			if (f->ae != NULL)
				return (f->ae);
			i--;
			if (b != NULL) {
				// And one step backward
				if (b->ae != NULL)
					return (b->ae);
				i--;
				b = b->next;
			}
			f = f->prev;
		} else if (b == NULL) {
			// Only backward step possible
			return (NULL);
			// And it fails
		} else {
			if (b->ae != NULL)
				return (b->ae);
			i--;
			b = b->next;
		}
	}
	return (NULL);
}

/* ------------------------------------------- */
/* The link server's state list is made global */
/* ------------------------------------------- */

enum {Start, RemFromLk, RemFromAr};

/* ------------------------------------- */
/* Link service process type declaration */
/* ------------------------------------- */

extern  int  zz_LinkService_prcs;

class   LinkService : public ZZ_SProcess {

    public:

    Station *S;

    LinkService () {
	S = TheStation;
	zz_typeid = (void*) (&zz_LinkService_prcs);
    };

    virtual char *getTName () { return ("LinkService"); };

    private:

      ZZ_LINK_ACTIVITY     *a;
      Link                 *cl;

    public:

    void setup () {

	// Initialize state name list

	zz_ns = 3;
	zz_sl = new const char* [RemFromAr + 1];
	zz_sl [0] = "Start";
	zz_sl [1] = "RemFromLk";
	zz_sl [2] = "RemFromAr";
    };

    void zz_code ();
};

void    LinkService::zz_code () {

  switch (TheState) {

      case Start:

	// Void action (go to sleep)

      break; case RemFromLk:

	// Remove activity from link

	cl = ((a = (ZZ_LINK_ACTIVITY*) Info01) -> GPort) -> Lnk;

	if (cl->Type == LT_pointtopoint) {
		// Take care of the alive list tail. This tail is only
		// used for the POINTTOPOINT link type
		if (a == cl->AliveTail) {
			if (a == cl->Alive) {   // The only activity in link
				cl->Alive = cl->AliveTail = NULL;
			} else {
				cl->AliveTail = a->prev;
				cl->AliveTail->next = NULL;
			}
		} else {
			pool_out (a);
		}
	} else
		pool_out (a);

	cl->NAlive --;

	assert (cl->NAlive >= 0,
		"LinkService: negative number of link activities");

	if (cl->PurgeDelay == TIME_0) {
		// Archive not used
		if (a->Type == JAM) {
			--cl->NAliveJams;
			assert (cl->NAliveJams >= 0,
				"LinkService: negative number of link jams");
			delete (ZZ_LINK_ACTIVITY_JAM*) a;
		} else {
			cl->NAliveTransfers--;
			assert (cl->NAliveTransfers >= 0,
			   "LinkService: negative number of link transfers");
			if (cl->PCleaner)
				(*(cl->PCleaner))(&(a->Pkt));
			delete (a);
		}

		return;
	}

	if (a->Type == JAM) {
		--cl->NAliveJams;
		assert (cl->NAliveJams >= 0,
			"LinkService: negative number of link jams");
		cl->NArchivedJams++;
	} else {
		cl->NAliveTransfers--;
		assert (cl->NAliveTransfers >= 0,
			"LinkService: negative number of link transfers");
		cl->NArchivedTransfers++;
	}

	cl->NArchived ++;

	if (cl->Type >= LT_unidirectional) {

		// Note: for any unidirectional link, archived activities
		// are kept in the order in which they leave the link.
		// Statistically, it should improve removal event processing.

		a->next = 0;    // This is going to be a new tail
		if (cl->ArchivedTail == NULL) {
			// The archive is empty
			cl->Archived = cl->ArchivedTail = a;
			a->prev = (ZZ_LINK_ACTIVITY*)(&(cl->Archived));
			a->ae = new ZZ_EVENT (Time + cl->PurgeDelay, System,
				(void*) a, NULL, TheProcess, cl, ARC_PURGE,
					RemFromAr, NULL);
		} else {
			a->prev = cl->ArchivedTail;
			cl->ArchivedTail->next = a;
			cl->ArchivedTail = a;
			// Use hint
			a->ae = new ZZ_EVENT (a->prev->ae, Time+cl->PurgeDelay,
				System, (void*) a, NULL, TheProcess, cl,
					ARC_PURGE, RemFromAr, NULL);
		}
	} else {

		pool_in (a, cl->Archived);

		new ZZ_EVENT (Time + cl->PurgeDelay, System, (void*) a, NULL,
			TheProcess, cl, ARC_PURGE, RemFromAr, NULL);
	}

      break; case RemFromAr:

	// Remove an activity from archive

	cl = ((a = (ZZ_LINK_ACTIVITY*) Info01) -> GPort) -> Lnk;

	if (cl->Type >= LT_unidirectional) {
		// Take care of the tail
		if (a == cl->ArchivedTail) {
			if (a == cl->Archived) {
				// The only activity
				cl->Archived = cl->ArchivedTail = NULL;
			} else {
				cl->ArchivedTail = a->prev;
				cl->ArchivedTail->next = NULL;
			}
		} else {
			pool_out (a);
		}
	} else
		pool_out (a);

	cl->NArchived--;

	assert (cl->NArchived >= 0,
		"LinkService: negative number of archived activities");

	if (a->Type == JAM) {
		cl->NArchivedJams--;
		assert (cl->NArchivedJams >= 0,
			"LinkService: negative number of archived jams");
		delete (ZZ_LINK_ACTIVITY_JAM*) a;
	} else {
		cl->NArchivedTransfers--;
		assert (cl->NArchivedTransfers >= 0,
			"LinkService: negative number of archived transfers");
		if (cl->PCleaner)
			(*(cl->PCleaner))(&(a->Pkt));
		delete a;
	}
  }
}

void Link::setPacketCleaner (void (*clnr)(Packet*)) {

	PCleaner = clnr;
}

void    Link::zz_start () {

/* --------------------------- */
/* The nonstandard constructor */
/* --------------------------- */

	// Initialize some members
	FlgSPF = ON;
#if ZZ_FLK
	FRate = 0.0;
	FType = NONE;
#endif
	Type = LT_broadcast;
	PCleaner = NULL;
	// Add the link to Kernel
	pool_in (this, TheProcess->ChList);
}

void Link::setFaultRate (double r, int ft) {

/* ---------------------------------- */
/* Sets fault processing for the link */
/* ---------------------------------- */
#if ZZ_FLK
    Assert (r >= 0.0 && r < 1.0,
	"Link->setFaultRate: rate (%f) must be >= 0 && < 1", r);
    Assert (ft >= NONE && ft <= FT_LEVEL2,
	"Link->setFaultRate: illegal fault service type %1d", ft);
    FRate = r;
    if (FRate == 0.0) ft = NONE;
    FType = ft;
#else
    excptn ("Link->setFaultRate illegal -- smurph not created with '-z'");
#endif
}

void	Link::setPurgeDelay (TIME at) {

	PurgeDelay = (at <= TIME_1) ? TIME_0 : at;
}

void    Link::setup (Long np, RATE r, TIME at, int spf) {

/* ------------------- */
/* Link initialization */
/* ------------------- */

	int     i, j;
	static  int     asize = 15;     // A small power of two - 1 (initial
					// size of zz_lk)
	DefXRate = r;
	DefAevMode = YES;

	Link    **scratch;

	Assert (!zz_flg_started,
	"Link->setup: cannot create new links after the protocol has started");

	FlgSPF = spf;   // Whether to calculate standard performance measures
	NPorts = (int)np;

	setPurgeDelay (at);

	NAliveTransfers = NArchivedTransfers = NArchived = NAlive =
		NArchivedJams = 0;
	NTJams = NTAttempts = 0;

	NRBits = NTBits = BITCOUNT_0;
	NRMessages = NTMessages = NRPackets = NTPackets = 0;
#if ZZ_FLK
	NDBits = BITCOUNT_0;
	NHDPackets = NDPackets = 0;
#endif
	// Clear request queues
	for (i = 0; i < N_LINK_EVENTS; i++) RQueue [i] = NULL;

	if ((Id = sernum++) == 0) {
		// The first time around -- create the array of Links
		zz_lk = new Link* [asize];
	} else if (Id >= asize) {
		// The array must be enlarged
		scratch = new Link* [asize];
		for (i = 0; i < asize; i++)
			// Backup copy
			scratch [i] = zz_lk [i];

		delete [] zz_lk;         // Deallocate previous array
		zz_lk = new Link* [asize = (asize+1) * 2 - 1];
		while (i--) zz_lk [i] = scratch [i];
		delete [] scratch;
	}

	Class = AIC_link;

	zz_lk [Id] = this;
	NLinks = sernum;

	Assert (NPorts >= 0,
		"Link->setup: illegal number of ports (%1d) for link %s",
			NPorts, getSName ());

	AliveTail = ArchivedTail = NULL;

	// Allocate the temporary distance matrix ...
	Alive = (ZZ_LINK_ACTIVITY*) new DISTANCE [np * np];

	// ... and the temporary port definition table
	Archived = (ZZ_LINK_ACTIVITY*) new Port* [np];

#define TDM(l)  ((DISTANCE*) (l->Alive))  // Temporary distance matrix
#define DTB(l)  ((Port**)(l->Archived))   // Temporary port definition table
					  // Reference to TDM
#define RTDM(l,i,j)     (*((DISTANCE*)(l->Alive) + (l->NPorts)*(i) + (j)))
					  // Reference to PDT
#define RDTB(l,i)       (((Port**)(l->Archived))[i])
#define LASTI   NAliveTransfers           // Last row position
#define LASTJ   NAliveJams                // Last row position

	LASTI = -1;     // Last "i" undefined
	LASTJ = -1;     // Last "j" indefined

	// Initialize the diagonal of the TDM and the PDT
	for (i = 0; i < np; i++) {
		RTDM (this, i, i) = DISTANCE_0;
		RDTB (this, i)    = NULL;
	}

	// Mark all entries in the distance matrix as uninitialized
	for (i = 0; i < np; i++)
		for (j = 0; j < np; j++)
			if (i != j) RTDM (this, i, j) = DISTANCE_inf;

	// Start the service process (if not started already)
	if (shandle == NULL) {

		Station         *sts;
		Process         *spr;

		// Pretend that the link service process is created by Kernel
		sts = TheStation;
		spr = TheProcess;
		TheStation = System;
		TheProcess = Kernel;
		// shandle = create LinkService;
		shandle = new LinkService;
		((LinkService*) shandle)->zz_start ();
		((LinkService*) shandle)->setup ();
		TheStation = sts;
		TheProcess = spr;
	}
}

void    Link::setD (int i, int j, double d) {

/* ------------------------------------ */
/* Set one entry in the distance matrix */
/* ------------------------------------ */

	int             k;
	DISTANCE	dd;

	LASTI = i;
	LASTJ = j;

	Assert (!zz_flg_started, "Link->setD: %s, cannot (re)set distances "
		"after the protocol has started", getSName ());

	Assert (i >= 0 && i < NPorts && j >= 0 && j < NPorts,
	    "Link->setD: DM indices (%1d,%1d) for link %s are out of range",
		i, j, getSName ());

	Assert (i != j || d == Distance_0,
	    "Link->setD: attempt to set diagonal entry %1d for link %s",
		i, getSName ());

	if (i > j) {
		k = i;
		i = j;
		j = k;
	}

	Assert (d < Distance_inf,
         "Link->setD: attempt to set infinite distance (%1d,%1d) for link %s",
	    i, j, getSName ());

	dd = duToItu (d);
	
	Assert (RTDM (this, i, j) == DISTANCE_inf || RTDM (this, i, j) == dd,
   	  "Link->setD: attempt to reset the already set entry (%1d,%1d) for "
	    "link %s", i, j, getSName ());

	RTDM (this, i, j) = RTDM (this, j, i) = dd;
}

void Link::complete () {

    Boolean more;
    DISTANCE dc, dl, dr;
    int i, j, k;

    if (Type == LT_pointtopoint) {

      more = YES;
      while (more) {
        more = NO;
        for (i = 0; i < NPorts-2; i++) {
          for (j = i+1; j < NPorts-1; j++) {
            if ((dl = RTDM (this, i, j)) == DISTANCE_inf) continue;
            for (k = j+1; k < NPorts; k++) {
              if ((dr = RTDM (this, j, k)) != DISTANCE_inf) {
                if ((dc = RTDM (this, i, k)) == DISTANCE_inf) {
                  more = YES;
                  RTDM (this, i, k) = RTDM (this, k, i) = dl + dr;
                } else if (dc != dl + dr) {
NL:
	         excptn ("Link->setD: PTP link %1d is not strictly linear", Id);
                }
              }
            }
          }
        }

        for (i = 0; i < NPorts-2; i++) {
          for (j = i+2; j < NPorts; j++) {
            if ((dc = RTDM (this, i, j)) == DISTANCE_inf) continue;
            for (k = i+1; k < j; k++) {
              dl = RTDM (this, i, k);
              dr = RTDM (this, k, j);
              if (dl == DISTANCE_inf) {
                if (dr != DISTANCE_inf) {
                  more = YES;
                  if (dr > dc) goto NL;
                  RTDM (this, i, k) = RTDM (this, k, i) = dc - dr;
                }
              } else {
                if (dr == DISTANCE_inf) {
                  more = YES;
                  if (dl > dc) goto NL;
                  dr = RTDM (this, k, j) = RTDM (this, j, k) = dc - dl;
                } else
                  if (dr + dl != dc) goto NL;
              }
            }
          }
        }
      }
    }
};

void    Link::setDTo (int j, double d) {

/* ---------------------------------------------------------- */
/* As  above,  but  with  the first index the same as for the */
/* last set                                                   */
/* ---------------------------------------------------------- */

	Assert (LASTI != -1, "Link->setDTo: %s, 'from' port undefined",
		getSName ());
	Assert (Type < LT_unidirectional || j > LASTI,
		"Link->setDTo: %s, cannot set distance upstream %1d > %1d",
			getSName (), j, LASTI);
	setD (LASTI, j, d);
}

void    Link::setDFrom (int i, double d) {

/* ---------------------------------------------------------- */
/* As  above,  but  with the second index the same as for the */
/* last set                                                   */
/* ---------------------------------------------------------- */

	Assert (LASTJ != -1, "Link->setDFrom: %s, 'to' port undefined",
		getSName ());
	Assert (Type < LT_unidirectional || LASTJ > i,
		"Link->setDFrom: %s, cannot set distance upstream %1d > %1d",
			getSName (), LASTJ, i);
	setD (i, LASTJ, d);
}

static  Port   *LASTIP = NULL,
	       *LASTJP = NULL;

void    setD (Port *a, Port *b, double d) {

/* ---------------------------------------------------------- */
/* Link-independent distance setting between two ports (which */
/* should belong to the same link, however).                  */
/* ---------------------------------------------------------- */

	Link    *Lnk;
	int     slasti, slastj;

	Assert (a != b, "setD: attempt to set distance from p (%s) to itself",
		a->getSName ());

	Assert (a->Lnk != NULL,
		"setD: first argument, %s, not connected to a link",
			a->getSName ());

	Assert (b->Lnk != NULL,
		"setD: second argument, %s, not connected to a link",
			b->getSName ());

	Lnk = a->Lnk;

	Assert (Lnk == b->Lnk,
	    "setD: the two ports (%s, %s) belong to different links (%s, %s)",
		a->getSName (), b->getSName (), Lnk->getSName (),
		    b->Lnk->getSName ());

	Assert (d < Distance_inf,
		"setD: attempt to set infinite distance from %s to %s",
			a->getSName (), b->getSName ());

	LASTIP = a;
	LASTJP = b;

	slasti = Lnk->LASTI;    // Preserve these; must be restored afterwards
	slastj = Lnk->LASTJ;

	Lnk->setD (a->LRId, b->LRId, d);

	Lnk->LASTI = slasti;
	Lnk->LASTJ = slastj;
}

void    setD (Port &a, Port &b, double d) {

/* -------- */
/* An alias */
/* -------- */

	setD (&a, &b, d);
}

void    Port::setDTo (Port *a, double d) {

/* --------------------------- */
/* Set distance from this to a */
/* --------------------------- */

	int     slasti, slastj;

	Assert (a != this, "Port->setDTo: %s, attempt to set distance to self",
		getSName ());

	Assert (Lnk != NULL, "Port->setDTo: %s, source not connected to a link",
		getSName ());

	Assert (a->Lnk != NULL,
		"Port->setDTo: %s, target (%s) not connected to a link",
		getSName (), a->getSName ());

	Assert (Lnk == a->Lnk,
		"Port->setDTo: source %s and target %s belong to different "
			"links (%s, %s)",
				getSName (), a->getSName (), Lnk->getSName (),
					a->Lnk->getSName ());

	Assert (Lnk->Type < LT_unidirectional || LRId < a->LRId,
	    "Port->setDTo: %s, cannot set distance upstream to %s (%1d < %1d)",
		getSName (), a->getSName (), LRId, a->LRId);

	LASTIP = this;
	LASTJP = a;

	slasti = Lnk->LASTI;    // Preserve these; must be restored afterwards
	slastj = Lnk->LASTJ;

	Lnk->setD (LRId, a->LRId, d);

	Lnk->LASTI = slasti;
	Lnk->LASTJ = slastj;
}

void    Port::setDTo (Port &a, double d) {

	setDTo (&a, d);
}

void    Port::setDFrom (Port *a, double d) {

/* --------------------------- */
/* Set distance from a to this  */
/* --------------------------- */

	int     slasti, slastj;

	Assert (a != this,
		"Port->setDFrom: %s, attempt to set distance to self",
			getSName ());

	Assert (Lnk != NULL,
		"Port->setDFrom: %s, source (%s) not connected to a link",
			getSName (), a->getSName ());

	Assert (a->Lnk != NULL,
		"Port->setDFrom: %s, target not connected to a link",
			getSName ());

	Assert (Lnk == a->Lnk,
		"Port->setDFrom: source %s and target %s belong to different "
			"links (%s, %s)",
				a->getSName (), getSName (), 
					a->Lnk->getSName (), Lnk->getSName ());

	Assert (Lnk->Type < LT_unidirectional || LRId > a->LRId,
		"Port->setDFrom: %s, cannot set distance upstream from %s "
			"(%1d > %1d)",
				getSName (), a->getSName (), LRId, a->LRId);

	LASTIP = a;
	LASTJP = this;

	slasti = Lnk->LASTI;    // Preserve these; must be restored afterwards
	slastj = Lnk->LASTJ;

	Lnk->setD (a->LRId, LRId, d);

	Lnk->LASTI = slasti;
	Lnk->LASTJ = slastj;
}

void    Port::setDFrom (Port &a, double d) {

	setDFrom (&a, d);
}

void    setDTo (Port *a, double d) {

/* --------------------------------------------------- */
/* The first port assumed to be the same as previously */
/* --------------------------------------------------- */

	Assert (LASTIP != NULL, "setDTo: 'from' port undefined");

	LASTIP->setDTo (a, d);
}

void    setDTo (Port &a, double d) {

	setDTo (&a, d);
}

void    setDFrom (Port *a, double d) {

/* ---------------------------------------------------- */
/* The second port assumed to be the same as previously */
/* ---------------------------------------------------- */

	Assert (LASTJP != NULL, "setDFrom: 'to' port undefined");

	LASTJP->setDFrom (a, d);
}

void    setDFrom (Port &a, double d) {

	setDFrom (&a, d);
}

Port::~Port () {
	excptn ("Port: once created, a port cannot be destroyed");
};

Port::Port () {

/* ---------------- */
/* Port constructor */
/* ---------------- */

	Port *p;

	Assert (!zz_flg_started,
	"Port: cannot create new ports after the protocol has started");

	Class = AIC_port;
	nextp = NULL;
	XRate = RATE_inf;	// Means "none"
	Lnk = NULL;             // Not connected yet

	// Add to the temporary list
	if (zz_ncport == NULL)
		zz_ncport = this;
	else {
		// Find the end of the list
		for (p = zz_ncport; p->nextp != NULL; p = p->nextp);
		p->nextp = this;
	}

	// Add to the ownership tree
	pool_in (this, TheProcess->ChList);
}

void    Port::zz_start () {

/* ------------------------------------------------------ */
/* Nonstandard constructor for a port created by 'create' */
/* ------------------------------------------------------ */

	int             srn;
	Port            *p;

	// Add the port at the end of list at the current station

	Assert (TheStation != NULL, "Port create: TheStation undefined");

	Assert (zz_ncport == this,
		"Port create: internal error -- initialization corrupted");

	// Remove from the temporary list
	zz_ncport = NULL;
	nextp = NULL;

	if ((p = TheStation->Ports) == NULL) {
		TheStation->Ports = this;
		srn = 0;
	} else {
		// Find the end of the station's port list
		for (srn = 1, p = TheStation->Ports; p -> nextp != NULL;
			p = p -> nextp, srn++);
		p -> nextp = this;
	}

	// Put into Id the station number combined with the port number
	// to be used for display and printing
	Id = MKCID (TheStation->Id, srn);
	Lnk = NULL;     // Flag the port as disconnected yet
	DV = NULL;
}

void    Port::setup (RATE r) {

/* -------------------------------------- */
/* Initializes a port created by 'create' */
/* -------------------------------------- */

	if (XRate != RATE_inf) {
		Assert (r == RATE_inf,
			"Port create: %s, redefining XRate", getSName ());
	} else
		XRate = r;
}

RATE    Port::setXRate (RATE r) {

/* ------------------------------------- */
/* Dynamically changes transmission rate */
/* ------------------------------------- */

	if (r == RATE_inf && Lnk)
		r = Lnk->DefXRate;

	assert (r != RATE_inf,
		"Port->setXRate: %s, illegal (infinite) rate", getSName ());

	RATE qr;
	qr = XRate;
	XRate = r;
	return qr;
}

Boolean	Port::setAevMode (Boolean b) {

	if (b == YESNO && Lnk)
		b = Lnk->DefAevMode;

	assert (b == YES || b == NO,
		"Port->setAevMode: %s, illegal (undefined) mode", getSName ());

	Boolean old;
	old = AevMode;
	AevMode = b;
	return old;
}

void    Link::setXRate (RATE r) {

/* ---------------------------------------------------------------- */
/* Changes the transmission rate of all ports connected to the link */
/* ---------------------------------------------------------------- */

	Long i;
	Port *np;

	if (r == RATE_inf)
		r = DefXRate;
	else
		DefXRate = r;

	for (i = 0; i < NStations; i++)
	  for (np = idToStation (i) -> Ports; np != NULL; np = np -> nextp)
	    if (np->Lnk == this)
	      np->XRate = r;
}

void    Link::setAevMode (Boolean b) {

	Long i;
	Port *np;

	if (b != YES && b != NO)
		b = DefAevMode;
	else
		DefAevMode = b;

	for (i = 0; i < NStations; i++)
	  for (np = idToStation (i) -> Ports; np != NULL; np = np -> nextp)
	    if (np->Lnk == this)
	      np->AevMode = b;
}

void    Port::connect (Link *lk, int lrid) {

/* --------------------------------------- */
/* Connect this port to the indicated link */
/* --------------------------------------- */

	Assert (Lnk == NULL, "Port->connect: %s connected twice",
		getSName ());
		
	if (lrid == NONE) {
		// Use the 'first free' link-relative id
		for (lrid = 0; lrid < lk->NPorts; lrid++)
			if (RDTB (lk, lrid) == NULL) break;
		Assert (lrid < lk->NPorts,
		 "Port->connect: %s, all ports at link %s are already taken",
		    getSName (), lk->getSName ());
	} else {

		Assert (lrid >= 0 && lrid < lk->NPorts,
		    "Port->connect: %s, port number %1d illegal for link %s",
			getSName (), lrid, lk->getSName ());

		Assert (RDTB (lk, lrid) == NULL,
	   	  "Port->connect: %s, port number %1d at link %s connected "
		    "twice", getSName (), lrid, lk->getSName ());
	}

	RDTB (lk, lrid) = this;          // Add the port to the link

	LRId = lrid;                    // Set the port's link-relative id
	Lnk = lk;                        // Link pointer

	Activity = Interpolator = NULL;
	AevMode = Lnk->DefAevMode;
	if (XRate == RATE_inf)
		XRate = Lnk->DefXRate;
	IJTime = TIME_0;
}

void    Port::connect (Long lk, int lrid) {

/* ---------------------------------------------------------- */
/* Another  version  of connect (with numeric link id instead */
/* of pointer)                                                */
/* ---------------------------------------------------------- */

	Assert (isLinkId (lk), "Port->connect: %s, %1d is not a legal link id",
		getSName (), lk);

	connect (idToLink (lk), lrid);
}

void    ZZ_SYSTEM::makeTopologyL () {

/* --------------------------------- */
/* Cleanup after topology definition */
/* --------------------------------- */

	int     i, j, k;
	Link    *Lnk;
	Port    *Prt;
	Station *s;
	DISTANCE td;

	for (i = 0; i < NStations; i++) {
		s = idToStation (i);
		for (j = 0, Prt = s->Ports; Prt != NULL; j++, Prt = Prt->nextp)
			Assert (Prt->Lnk != NULL,
    				"makeTopology: Port %s is left disconnected",
					Prt->getSName ());
	}

	for (i = 0; i < NLinks; i++) {  // For every link

		Lnk = idToLink (i);

		Lnk->complete ();

		for (j = 0; j < Lnk->NPorts; j++) {     // For every port

			Prt = RDTB (Lnk, j);

			if (Prt == NULL)
	excptn ("makeTopology: port %1d for link %1d undefined", j, i);

			// Create the port's distance vector
			Prt->DV = new DISTANCE [Lnk->NPorts];
			for (k = 0, td = DISTANCE_0; k < Lnk->NPorts; k++) {
				Prt->DV [k] = RTDM (Lnk,  j, k);
				if (Prt->DV [k] == DISTANCE_inf) excptn (
	"makeTopology: distance vector (%1d) of port %1d at link %1d undefined",
		k, j, i);
				if (Lnk->Type < LT_unidirectional || k > j) {
					if (Prt->DV [k] > td) td = Prt->DV [k]; 
				}
			}
			Prt->MaxDistance = td + TIME_1;
		}

		// Deallocate temporary data structures

		delete [] ((void*)TDM(Lnk));
		delete [] ((void*)DTB(Lnk));
		// DTB (Lnk) = TDM (Lnk) = 0;
		Lnk->Alive = Lnk->Archived = 0;
		Lnk->LASTI = Lnk->LASTJ = 0;
					// Clear temporary counters
	}
}

double Port::distTo (Port *p) {

/* --------------------------------------------------- */
/* Gives the distance from this port to the other port */
/* --------------------------------------------------- */

    if (p == this)
	return Distance_0;
    if (Lnk == NULL || Lnk != p->Lnk)
	return Distance_inf;
    if ((Lnk->Type == LT_unidirectional || Lnk->Type == LT_pointtopoint) &&
	p->LRId < LRId)
	  return Distance_inf;
    return ituToDu (DV != NULL ? DV [p->LRId] : RTDM (Lnk, LRId, p->LRId));
}

static void sort_acs_s (TIME *acs, int lo, int up) {

/* ------------------------------------------------------- */
/* Sorts activity intervals according to the starting time */
/* ------------------------------------------------------- */


	int     i, j;
	TIME    t, s;

	while (up > lo) {
		t = acs [i = lo];
		s = acs [i + 1];
		j = up;

		while (i < j) {
			while (acs [j] > t) j -= 2;
			acs [i] = acs [j];
			acs [i+1] = acs [j+1];
			while ((i < j) && (acs [i] <= t)) i += 2;
			acs [j] = acs [i];
			acs [j+1] = acs [i+1];
		}
		acs [i] = t;
		acs [i+1] = s;

		sort_acs_s (acs, lo, i-2);
		lo = i + 2;
	}
}

static void sort_acs_p (struct activity_descr_struct *acs, int lo, int up) {

/* ------------------------------------------------------- */
/* Another version of sort_acs_s (used by 'collisionTime') */
/* ------------------------------------------------------- */

	int                             i, j;
	struct  activity_descr_struct   s;
	TIME                            t;

	while (up > lo) {
		s = acs [i = lo];
		t = s.t_s;
		j = up;

		while (i < j) {
			while (acs [j].t_s > t) j--;
			acs [i] = acs [j];
			while ((i < j) && (acs [i].t_s <= t)) i++;
			acs [j] = acs [i];
		}
		acs [i] = s;

		sort_acs_p (acs, lo, i-1);
		lo = i + 1;
	}
}

static void sort_acs_e (struct activity_heard_struct *acs, int lo, int up) {

/* ------------------------------------------------- */
/* Sort activity intervals according to the end time */
/* ------------------------------------------------- */

int                             i, j;
TIME                            t;
struct activity_heard_struct    s;

	while (up > lo) {

		s = acs [i = lo];
		t = s.t_f;
		j = up;

		while (i < j) {
			while (acs [j].t_f > t) j--;
			acs [i] = acs [j];
			while ((i < j) && (acs [i].t_f <= t)) i++;
			acs [j] = acs [i];
		}
		acs [i] = s;
		sort_acs_e (acs, lo, i-1);

		lo = i + 1;
	}
}

#if     ASSERT
#ifdef  VERIFY_SORTS

/* -------------- */
/* Sort verifiers */
/* -------------- */

static  sorted_acs_s (TIME *acs, int n) {

	int     i;

	for (i = 2; i < n; i += 2) if (acs [i] < acs [i-2]) return (NO);
	return (YES);
}

static  sorted_acs_p (activity_descr_struct *acs, int n) {

	int     i;

	for (i = 1; i < n; i ++) if (acs [i].t_s < acs [i-1].t_s) return (NO);
	return (YES);
}

static  sorted_acs_e (activity_heard_struct *acs, int n) {

	int     i;

	for (i = 1; i < n; i++) if (acs [i].t_f < acs [i-1].t_f) return (NO);
	return (YES);
}

/* --------------------- */
/* End of sort verifiers */
/* --------------------- */

#endif
#endif

TIME    Port::silenceTime () {

/* ------------------------------------------------------ */
/* Determines the earliest silence recognized by the port */
/* ------------------------------------------------------ */

    ZZ_LINK_ACTIVITY   *a;
    TIME               t, te;
    LONG               acs [ACSSIZE * BIG_size];
    int                i, n, kf, ks;

    if (Lnk->NAlive == 0) return (Time);    // The link is silent

    switch (Lnk->Type) {

      case LT_broadcast:
      case LT_cpropagate:

	for (a = Lnk->Alive, n = 0, kf = ks = YES; a != NULL; a = a->next) {

		if (n >= ACSSIZE) excptn ("Port->silenceTime: stack overflow");

		((TIME*)acs) [n++] = t = a->STime + DV [a->LRId];
		if (t <= Time) {
			ks = NO;
			if (undef (a->FTime)) {
				return (TIME_inf);
			} else {
				t = a->FTime + DV [a->LRId];
				if (t > Time)
					kf = NO;
				((TIME*)acs) [n++] = t;
			}
		} else {
			if (undef (a->FTime))
				((TIME*)acs) [n++] = TIME_inf;
			else
				((TIME*)acs) [n++] = a->FTime + DV [a->LRId];
		}
	}

	if (ks || kf) return (Time);

	sort_acs_s (((TIME*)acs), 0, n-2);

#ifdef  VERIFY_SORTS
	assert (sorted_acs_s (((TIME*)acs), n),
		"Port->silenceTime: internal error -- acs not sorted");
#endif
	for (i = 0, te = Time; (i < n) && def (te); i++) {
		if (((TIME*)acs) [i++] > te) break;
		if (((TIME*)acs) [i] > te) te = ((TIME*)acs) [i];
	}

	return (te);

      case LT_unidirectional:

	for (a = Lnk->Alive, n = 0, kf = ks = YES; a != NULL; a = a->next) {

	    if (a->LRId <= LRId) {
		// Ignore downstream activities for unidirectional links

		if (n >= ACSSIZE) excptn ("Port->silenceTime: stack overflow");

		((TIME*)acs) [n++] = t = a->STime + DV [a->LRId];
		if (t <= Time) {
			ks = NO;
			if (undef (a->FTime)) {
				return (TIME_inf);
			} else {
				t = a->FTime + DV [a->LRId];
				if (t > Time)
					kf = NO;
				((TIME*)acs) [n++] = t;
			}
		} else {
			if (undef (a->FTime))
				((TIME*)acs) [n++] = TIME_inf;
			else
				((TIME*)acs) [n++] = a->FTime + DV [a->LRId];
		}
	    }
	}

	if (ks || kf) return (Time);

	sort_acs_s (((TIME*)acs), 0, n-2);

#ifdef  VERIFY_SORTS
	assert (sorted_acs_s (((TIME*)acs), n),
		"Port->silenceTime: internal error -- acs not sorted");
#endif

	for (i = 0, te = Time; (i < n) && def (te); i++) {
		if (((TIME*)acs) [i++] > te) break;
		if (((TIME*)acs) [i] > te) te = ((TIME*)acs) [i];
	}

	return (te);

      case LT_pointtopoint:

	if ((a = initLAI ()) == NULL || a->STime + DV [a->LRId] > Time)
		// Silence heard now
		return (Time);

		// An activity heard now
	if (undef (a->FTime)) return (TIME_inf);

	for (te = a->FTime + DV [a->LRId], a = a->next; a != NULL;
		a = a->next) {

		if (a->LRId > LRId) continue;   // Upstream activity

		if (a->STime + DV [a->LRId] > te) return (te);

		if (undef (a->FTime)) return (TIME_inf);

		t = a->FTime + DV [a->LRId];

		if (t > te) te = t;
	}

	return (te);

      default: excptn ("Port->silenceTime: illegal link type %1d", Lnk->Type);
	       return (TIME_0);
    }
}

TIME    Port::collisionTime (int lookatjams) {

/* ---------------------------------------------------------- */
/* Determines  the earliest collision recognized by the port. */
/* The parameter tells whether jams should be examined.       */
/* ---------------------------------------------------------- */

    ZZ_LINK_ACTIVITY                *a, *b, *aj;
    TIME                            tc, tg, tsb, tfb, tfa, bsa, afa, bfr;
    LONG                            acs [ACSSIZE * (BIG_size + 1)];
    int                             i, j, n;
    DISTANCE                        *dst;

    switch (Lnk->Type) {

      case LT_cpropagate:

	// In this version collisions propagate

	if (Lnk->NAlive == 0) return (TIME_inf);        // Unknown
	tc = TIME_inf;                                  // Unknown yet

	for (a = Lnk->Alive, n = 0; a != NULL; a = a -> next) {

		if (a->Type == JAM) continue;

		if (n >= ACSSIZE)
			excptn ("Port->collisionTime: stack overflow");

		((struct activity_descr_struct*)acs) [n].ba = a;
		((struct activity_descr_struct*)acs) [n++].t_s =
			a->STime + DV [a->LRId];
	}

	if (n > 1) {

		sort_acs_p (((struct activity_descr_struct*)acs), 0, n-1);
#ifdef  VERIFY_SORTS
		assert (sorted_acs_p(((struct activity_descr_struct*)acs), n),
		"Port->collisionTime: internal error -- acs not sorted");
#endif
		a = ((struct activity_descr_struct*)acs) [0] . ba;

		for (tfa = def (a->FTime) ? a->FTime + DV [a -> LRId] :
			TIME_inf, i = 1; i < n; i++) {

			// The second activity

			b = ((struct activity_descr_struct*)acs) [i] . ba;
			tsb = ((struct activity_descr_struct*)acs) [i].t_s;

			if (tfa > Time) {
				// The end of first activity has not passed
				tfb = undef (b -> FTime) ? TIME_inf :
					b -> FTime + DV [b->LRId];
				if (tsb < tfa) {
					// Overlap
					if (tfb > Time) {
						// Second activity not passed
						tc = tsb;
						goto LKJAMS;
					} 
				} else {
					if (a->Type == TRANSMISSION_J) {
						// Followed by jam
						tc = tfa;
						goto LKJAMS;
					}
					// New first activity
					a = b;
					tfa = tfb;
				}

			}

			// General collision check
			// Activity b is a potential second candidate

			dst = (b->GPort) -> DV;

			// Absolute time b started
			bsa = b -> STime;

			// Senser-relative time b finished
			bfr = def (b->FTime) ? b->FTime + DV [b->LRId] :
				TIME_inf;

			for (j = i-1; j >= 0; j--) {

				// The first potential candidate
				aj =
				  ((struct activity_descr_struct*)acs) [j] . ba;

				// Absolute time a finished
				afa = aj -> FTime;

				if (undef (afa)) {
					// Collision
					if (bfr > Time) {
						// Still heard
						tc = tsb;
						goto LKJAMS;
					}
					continue;
				}

				// End of a as perceived by b
				tg = afa + dst [aj->LRId];
				if (tg > bsa) {
					// Collision
					if (tsb >= Time) {
						tc = tsb;
						goto LKJAMS;
					}

					// Examine the duration

					if (b->FTime >= tg) {
						// End at b when b hears
						// a's end
						tg = tg + DV [b->LRId];
						if (tg > Time) {
							tc = tsb;
							goto LKJAMS;
						}
						continue;
					}

					// End of b as perceived by a
					tg = b->FTime + dst [aj->LRId];
					if (afa > tg) {
						// End at a when a hears
						// b's end
						tg += DV [aj->LRId];
						if (tg > Time) {
							tc = tsb;
							goto LKJAMS;
						}
						continue;
					}

					// End when the second end of
					// transfer is heard
					if (bfr > Time) {
						tc = tsb;
						goto LKJAMS;
					}

				}
			}

		}
	}
LKJAMS:
	if (lookatjams) {

		// Check if a jam does not cause an earlier collision

		for (a = Lnk->Alive; (a != NULL) && (tc > Time);
			a = a -> next) {

			if (a->Type != JAM) continue;
			if (def (a->FTime) &&
				(a->FTime + DV [a->LRId] <= Time)) continue;

			tsb = a->STime + DV [a->LRId];
			if (tsb < tc) tc = tsb;
		}
	}

	return (tc < Time ? Time : tc);

      case LT_unidirectional:

	// Unidirectional link

	if (Lnk->NAlive == 0) return (TIME_inf);
	tc = TIME_inf;

	for (a = Lnk->Alive, n = 0; a != NULL; a = a -> next) {

		if ((a->LRId > LRId) || // Ignore downstream activities
		    (a->Type == JAM)    // Ignore jams by now
			|| (def (a->FTime) &&
					// And passed activities
				(a->FTime + DV [a->LRId] <= Time))) continue;

		if (n >= ACSSIZE)
			excptn ("Port->collisionTime: stack overflow");

		((struct activity_descr_struct*)acs) [n].ba = a;
		((struct activity_descr_struct*)acs) [n++].t_s =
			a->STime + DV [a->LRId];
	}

	if (n > 1) {

		// This part is the same as for a regular ether-type link
		// Duplicated for increased efficiency

		sort_acs_p (((struct activity_descr_struct*)acs), 0, n-1);
#ifdef  VERIFY_SORTS
		assert (sorted_acs_p(((struct activity_descr_struct*)acs), n),
		"Port->collisionTime: internal error -- acs not sorted");
#endif
		a = ((struct activity_descr_struct*)acs) [0] . ba;

		for (tfa = def (a->FTime) ? a->FTime + DV [a -> LRId] :
			TIME_inf, i = 1; i < n; i++) {

			// The second activity

			tg = ((struct activity_descr_struct*)acs) [i].t_s;
			if (tg < tfa) {
				// Overlaps with the first
				tc = tg;
				break;
			}
			b = ((struct activity_descr_struct*)acs) [i] . ba;
			if (def (b->FTime)) {
				tfb = b->FTime + DV [b->LRId];
				if (tfb > tfa) tfa = tfb;
			} else
				tfa = TIME_inf;
		}
	}

	if (lookatjams) {

		// Check if a jam does not cause an earlier collision

		for (a = Lnk->Alive; (a != NULL) && tc > Time; a = a -> next) {

			if ((a->LRId > LRId) || (a->Type != JAM)) continue;
			if (def (a->FTime) && (a->FTime + DV [a->LRId] <= Time))
				continue;

			tsb = a->STime + DV [a->LRId];
			if (tsb < tc) tc = tsb;
		}
	}

	return (tc < Time ? Time : tc);

    case LT_broadcast:

	// Regular ether-type carrier

	if (Lnk->NAlive == 0) return (TIME_inf);
	tc = TIME_inf;

	for (a = Lnk->Alive, n = 0; a != NULL; a = a -> next) {

		if ((a->Type == JAM)    // Ignore jams by now
			|| (def (a->FTime) &&
					// And passed activities
				(a->FTime + DV [a->LRId] <= Time))) continue;

		if (n >= ACSSIZE)
			excptn ("Port->collisionTime: stack overflow");

		((struct activity_descr_struct*)acs) [n].ba = a;
		((struct activity_descr_struct*)acs) [n++].t_s =
			a->STime + DV [a->LRId];
	}

	if (n > 1) {

		// This part is the same as for a regular ether-type link
		// Duplicated for increased efficiency

		sort_acs_p (((struct activity_descr_struct*)acs), 0, n-1);
#ifdef  VERIFY_SORTS
		assert (sorted_acs_p(((struct activity_descr_struct*)acs), n),
		"Port->collisionTime: internal error -- acs not sorted");
#endif
		a = ((struct activity_descr_struct*)acs) [0] . ba;

		for (tfa = def (a->FTime) ? a->FTime + DV [a -> LRId] :
			TIME_inf, i = 1; i < n; i++) {

			// The second activity

			tg = ((struct activity_descr_struct*)acs) [i].t_s;
			if (tg < tfa) {
				// Overlaps with the first
				tc = tg;
				break;
			}
			b = ((struct activity_descr_struct*)acs) [i] . ba;
			if (def (b->FTime)) {
				tfb = b->FTime + DV [b->LRId];
				if (tfb > tfa) tfa = tfb;
			} else
				tfa = TIME_inf;
		}
	}

	if (lookatjams) {

		// Check if a jam does not cause an earlier collision

		for (a = Lnk->Alive; (a != NULL) && (tc > Time);
			a = a -> next) {

			if (a->Type != JAM) continue;
			if (def (a->FTime) &&
				(a->FTime + DV [a->LRId] <= Time)) continue;

			tsb = a->STime + DV [a->LRId];
			if (tsb < tc) tc = tsb;
		}
	}

	return (tc < Time ? Time : tc);

      case LT_pointtopoint:

	// Find the first event which is still being heard

	for (a = initLAI (); a != NULL; a = a->next) {

		if (a->LRId > LRId) continue;   // Upstream activity

		if (def (a->FTime)) {
			tg = a->FTime + DV [a->LRId];
			if (tg > Time) break;
		} else {
			tg = TIME_inf;
			break;
		}
	}

	if (a == NULL) return (TIME_inf);

	if (a->Type == JAM) {
		tc = a->STime + DV [a->LRId];
		return ((tc < Time) ? Time : tc);
	}

	for (a = a->next; a != NULL; a = a->next) {
		if (a->LRId > LRId) continue;   // Upstream activity
		tc = a->STime + DV [a->LRId];
		if (a->Type == JAM || (tc < tg))
			return ((tc < Time) ? Time : tc);
		tg = def (a->FTime) ? a->FTime + DV [a->LRId] : TIME_inf;
	}

	return (TIME_inf);

      default:  excptn ("Port->collisionTime: illegal link type %1d", Lnk->Type);
		return (TIME_0);
    }
}

TIME    Port::eojTime () {

/* -------------------------------------------------------- */
/* Determine the earliest end of jam recognized by the port */
/* -------------------------------------------------------- */

	ZZ_LINK_ACTIVITY   *a;
	TIME               te;
	LONG               acs [ACSSIZE * BIG_size];
	DISTANCE           t;
	int                i, n;

	// Note: this part is not optimized for LT_pointtopoint link type

	if (Lnk->NAliveJams == 0) return (TIME_inf);    // Unknown

	for (a = Lnk->Alive, n = 0; a != NULL; a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) && (a->LRId > LRId))
			// Ignore downstream activities for a unidirectional
			// link
			continue;
		if (a->Type != JAM) continue;

		t = DV [a->LRId];

		if (undef (a->FTime)) {
			te = TIME_inf;
		} else {
			te = a->FTime + t;
			if (te < Time) continue;
		}

		if (n >= ACSSIZE) excptn ("Port->eojTime: stack overflow");

		((TIME*)acs) [n++] = a->STime + t;
		((TIME*)acs) [n++] = te;
	}

	if (n < 4) {
		if (n == 0) {
			return (TIME_inf);
		}

		return (((TIME*)acs)[1]);
	}

	sort_acs_s (((TIME*)acs), 0, n-2);
#ifdef  VERIFY_SORTS
	assert (sorted_acs_s (((TIME*)acs), n),
		"Port->eojTime: internal error -- acs not sorted");
#endif
	for (i=2, te = ((TIME*)acs) [1]; (i < n) && def (te); i++) {
		if (((TIME*)acs) [i++] > te) break;
		if (((TIME*)acs) [i] > te) te = ((TIME*)acs) [i];
	}

	return (te);
}

TIME    Port::activityTime () {

/* -------------------------------------------------------- */
/* Determines when the port will hear the earliest activity */
/* -------------------------------------------------------- */

    ZZ_LINK_ACTIVITY   *a;
    TIME               t, ts;

    switch (Lnk->Type) {

      case LT_broadcast:
      case LT_cpropagate:

	for (t = TIME_inf, a = Lnk->Alive, tact = NULL; a != NULL;
		a = a -> next) {

		ts = a->STime + DV [a->LRId];

		if (ts <= Time) {

			// Started soon enough to be heard by the
			// station

			if (undef (a->FTime) ||
				(a->FTime + DV [a->LRId] > Time)) {

				// And still being heard

				t = Time;
				tact = a;
				goto ACT_DONE;
			}
			// Ignore otherwise
		} else {
			if (ts < t) {
				t = ts;
				tact = a;
			}
			// Will hear it in the future
		}
	}

ACT_DONE:

	if ((tact != NULL) && (tact->Type != JAM)) tpckt = &(tact->Pkt);
	return (t);

      case LT_unidirectional:

	for (t = TIME_inf, a = Lnk->Alive, tact = NULL; a != NULL;
		a = a -> next) {

	    if (a->LRId <= LRId) {      // Upstream activities only
	
		ts = a->STime + DV [a->LRId];
	
		if (ts <= Time) {

			// Started soon enough to be heard by the
			// station

			if (undef (a->FTime) ||
				(a->FTime + DV [a->LRId] > Time)) {

				// And still being heard

				t = Time;
				tact = a;
				goto ACT_DONE1;
			}
			// Ignore otherwise
		} else {
			if (ts < t) {
				t = ts;
				tact = a;
			}
			// Will hear it in the future
		}
	    }
	}

ACT_DONE1:

	if ((tact != NULL) && (tact->Type != JAM)) tpckt = &(tact->Pkt);
	return (t);

      case LT_pointtopoint:

	for (a = initLAI () ; a != NULL; a = a->next) {

		if (a->LRId > LRId) continue;   // Upstream activity

		t = a->STime + DV [a->LRId];

		if (t >= Time) {
			// Found
			tact = a;
			if (a->Type != JAM) tpckt = &(a->Pkt);
			return (t);
		}

		if (undef (a->FTime) || (a->FTime + DV [a->LRId] > Time)) {
			// Found
			tact = a;
			if (a->Type != JAM) tpckt = &(a->Pkt);
			return (Time);
		}
	}

	tact = NULL;
	return (TIME_inf);

      default: excptn ("Port->activityTime: illegal link type %1d", Lnk->Type);
	       return (TIME_0);
    }
}

TIME    Port::bojTime () {

/* --------------------------------------------------- */
/* Determines when the port will hear the earliest jam */
/* --------------------------------------------------- */

	TIME               t, ts;
	ZZ_LINK_ACTIVITY   *a;

	// Note: not optimized for LT_pointtopoint links

	for (t = TIME_inf, a = Lnk->Alive; a != NULL;
		a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			// Usual check for a unidirectional link
			(a->LRId > LRId)) continue;

		if (a->Type != JAM) continue;   // Jams only

		ts = a->STime + DV [a->LRId];
		if (ts <= Time) {

			// Started soon enough to be heard by the port.

			if (undef (a->FTime) ||
				(a->FTime + DV [a->LRId] > Time)) {

				// And still being heard

				t = Time;
				tact = a;
				break;
			}
			// Ignore otherwise
		} else {
			if (ts < t) {
				t = ts;
				tact = a;
			}
		}
	}
	return (t);
}

TIME    Port::eotTime () {

/* ---------------------------------------------------------- */
/* Determines  when  the  port  will  hear the nearest end of */
/* packet                                                     */
/* ---------------------------------------------------------- */

    TIME               t, ts;
    ZZ_LINK_ACTIVITY   *a;

    switch (Lnk->Type) {

      case LT_broadcast:
      case LT_cpropagate:

	for (t = TIME_inf, a = Lnk->Alive, tact = NULL; a != NULL;
		a = a -> next) {

		if (a->Type != TRANSMISSION) continue;

		// Note that for a transfer, a->FTime is != TIME_inf

		ts = a->FTime + DV [a->LRId];
		if ((ts >= Time) && (ts < t)) {
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (a->Pkt.Flags, PF_damaged)) continue;
#endif
			t = ts;
			tact = a;
		}
	}

	if (tact != NULL) tpckt = &(tact->Pkt);
	return (t);

      case LT_unidirectional:

	for (t = TIME_inf, a = Lnk->Alive, tact = NULL; a != NULL;
		a = a -> next) {

		if ((a->LRId > LRId) || (a->Type != TRANSMISSION)) continue;

		// Note that for a transfer, a->FTime is != TIME_inf

		ts = a->FTime + DV [a->LRId];
		if ((ts >= Time) && (ts < t)) {
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (a->Pkt.Flags, PF_damaged)) continue;
#endif
			t = ts;
			tact = a;
		}
	}

	if (tact != NULL) tpckt = &(tact->Pkt);
	return (t);

      case LT_pointtopoint:

	if ((a = initLAI ()) == NULL) {
		tact = NULL;
		return (TIME_inf);
	}

	if (a->Type != TRANSMISSION
#if ZZ_FLK
  || (Lnk->FType == FT_LEVEL2 && flagSet (a->Pkt.Flags, PF_damaged))
#endif
				) {
		for (a = a->next; a != NULL; a = a->next) {
			if (a->LRId > LRId || a->Type != TRANSMISSION) continue;
			// Note that now a->FTime must be finite
			t = a->FTime + DV [a->LRId];
			if (t < Time) continue;
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (a->Pkt.Flags, PF_damaged)) continue;
#endif
			goto EOT1;              // Never mind
		}

	} else {
		t = a->FTime + DV [a->LRId];
		goto EOT1;
	}

	tact = NULL;
	return (TIME_inf);
EOT1:
	tact = a;

	for (a = a->next; a != NULL; a = a->next) {

		if (a->Type != TRANSMISSION || a->LRId > LRId) continue;
		if (a->STime + DV [a->LRId] >= t) {
			tpckt = &(tact->Pkt);
			return (t);
		}
		// Note that a->FTime must be finite
		ts = a->FTime + DV [a->LRId];
		if (ts < Time) continue;
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (a->Pkt.Flags, PF_damaged)) continue;
#endif
		if (ts < t) {
			t = ts;
			tact = a;
		}
	}

	tpckt = &(tact->Pkt);
	return (t);

      default: excptn ("Port->eotTime: illegal link type %1d", Lnk->Type);
	       return (TIME_0);
    }
}

TIME    Port::botTime () {

/* ---------------------------------------------------------- */
/* Determines  when  the port will hear the nearest beginning */
/* of a packet                                                */
/* ---------------------------------------------------------- */

    TIME               t, ts;
    ZZ_LINK_ACTIVITY   *a;

    switch (Lnk->Type) {

      case LT_broadcast:
      case LT_cpropagate:

	for (t = TIME_inf, a = Lnk->Alive, tact = NULL; a != NULL;
		a = a -> next) {

		if (a->Type == JAM) continue;

		ts = a->STime + DV [a->LRId];
		if ((ts >= Time) && (ts < t)) {
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (a->Pkt.Flags, PF_hdamaged)) continue;
#endif
			t = ts;
			tact = a;
		}
	}

	if (tact != NULL) tpckt = &(tact->Pkt);
	return (t);

      case LT_unidirectional:

	for (t = TIME_inf, a = Lnk->Alive, tact = NULL; a != NULL;
		a = a -> next) {

		if (a->LRId > LRId) continue;

		if (a->Type == JAM) continue;

		ts = a->STime + DV [a->LRId];
		if ((ts >= Time) && (ts < t)) {
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (a->Pkt.Flags, PF_hdamaged)) continue;
#endif
			t = ts;
			tact = a;
		}
	}

	if (tact != NULL) tpckt = &(tact->Pkt);
	return (t);

      case LT_pointtopoint:

	for (a = initLAI (); a != NULL; a = a->next) {
		if (a->LRId > LRId || a->Type == JAM) continue;
		t = a->STime + DV [a->LRId];
		if (t < Time) continue;
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (a->Pkt.Flags, PF_hdamaged)) continue;
#endif
		tact = a;
		tpckt = &(tact->Pkt);
		return (t);
	}

	tact = NULL;
	return (TIME_inf);

      default: excptn ("Port->botTime: illegal link type %1d", Lnk->Type);
      return (TIME_0);
    }
}

TIME    Port::bmpTime () {

/* ---------------------------------------------------------- */
/* Determines  the nearest beginning of a packet addressed to */
/* this station                                               */
/* ---------------------------------------------------------- */

    TIME               t, ts;
    ZZ_LINK_ACTIVITY   *a;

    switch (Lnk->Type) {

      case LT_broadcast:
      case LT_cpropagate:

	for (t = TIME_inf, a = Lnk->Alive, tact = NULL; a != NULL;
		a = a -> next) {

		if (a->Type == JAM) continue;
#if ZZ_FLK
  if (Lnk->FType > FT_LEVEL0 && flagSet (a->Pkt.Flags, PF_hdamaged)) continue;
#endif
		if (flagSet ((a->Pkt).Flags, PF_broadcast)) {

			// A broadcast packet
			ts = a->STime + DV [a->LRId];
			if ((ts < Time) || (ts >= t)) continue;
			// Check if addressed to this station
			if (((SGroup*)((a->Pkt).Receiver))->
				occurs (TheStation->Id)) {

				t = ts;
				tact = a;
			}
			continue;
		}

		// A non-broadcast packet

		if ((a->Pkt).Receiver != TheStation->Id) continue;
		ts = a->STime + DV [a->LRId];
		if ((ts >= Time) && (ts < t)) {
			t = ts;
			tact = a;
		}
	}

	if (tact != NULL) tpckt = &(tact->Pkt);
	return (t);

      case LT_unidirectional:

	for (t = TIME_inf, a = Lnk->Alive, tact = NULL; a != NULL;
		a = a -> next) {

		if (a->LRId > LRId) continue;

		if (a->Type == JAM) continue;
#if ZZ_FLK
  if (Lnk->FType > FT_LEVEL0 && flagSet (a->Pkt.Flags, PF_hdamaged)) continue;
#endif
		if (flagSet ((a->Pkt).Flags, PF_broadcast)) {

			// A broadcast packet
			ts = a->STime + DV [a->LRId];
			if ((ts < Time) || (ts >= t)) continue;
			// Check if addressed to this station
			if (((SGroup*)((a->Pkt).Receiver))->
				occurs (TheStation->Id)) {

				t = ts;
				tact = a;
			}
			continue;
		}

		// A non-broadcast packet

		if ((a->Pkt).Receiver != TheStation->Id) continue;
		ts = a->STime + DV [a->LRId];
		if ((ts >= Time) && (ts < t)) {
			t = ts;
			tact = a;
		}
	}

	if (tact != NULL) tpckt = &(tact->Pkt);
	return (t);

      case LT_pointtopoint:

	for (a = initLAI (), t = TIME_inf; a != NULL; a = a->next) {

		if (a->Type == JAM || a->LRId > LRId) continue;
#if ZZ_FLK
  if (Lnk->FType > FT_LEVEL0 && flagSet (a->Pkt.Flags, PF_hdamaged)) continue;
#endif
		if (flagSet ((a->Pkt).Flags, PF_broadcast)) {

			ts = a->STime + DV [a->LRId];
			if (ts < Time) continue;

			// Check if addressed to this station
			if (((SGroup*)((a->Pkt).Receiver))->
				occurs (TheStation->Id)) {

				tact = a;
				tpckt = &(tact->Pkt);
				t = ts;
				break;
			}
			continue;
		}

		// A non-broadcast packet

		if ((a->Pkt).Receiver != TheStation->Id) continue;
		ts = a->STime + DV [a->LRId];
		if (ts < Time) continue;
		tact = a;
		tpckt = &(tact->Pkt);
		t = ts;
		break;
	}

	return (t);

      default: excptn ("Port->bmpTime: illegal link type %1d", Lnk->Type);
      return (TIME_0);
    }
}

TIME    Port::empTime () {

/* ---------------------------------------------------------- */
/* Detects  the  first  end  of  a  packet  addressed to this */
/* station                                                    */
/* ---------------------------------------------------------- */

    TIME               t, ts;
    ZZ_LINK_ACTIVITY   *a;

    switch (Lnk->Type) {

      case LT_broadcast:
      case LT_cpropagate:

	for (t = TIME_inf, a = Lnk->Alive, tact = NULL; a != NULL;
		a = a -> next) {

		if (a->Type != TRANSMISSION) continue;
#if ZZ_FLK
  if (Lnk->FType > FT_LEVEL0 && flagSet (a->Pkt.Flags, PF_damaged)) continue;
#endif
		if (flagSet ((a->Pkt).Flags, PF_broadcast)) {

			// A broadcast packet
			ts = a->FTime + DV [a->LRId];
			if ((ts < Time) || (ts >= t)) continue;
			// Check if addressed to this station
			if (((SGroup*)((a->Pkt).Receiver))->
				occurs (TheStation->Id)) {

				t = ts;
				tact = a;
			}
			continue;
		}

		// A non-broadcast packet

		if ((a->Pkt).Receiver != TheStation->Id) continue;
		ts = a->FTime + DV [a->LRId];
		if ((ts >= Time) && (ts < t)) {
			t = ts;
			tact = a;
		}
	}

	if (tact != NULL) tpckt = &(tact->Pkt);
	return (t);

      case LT_unidirectional:

	for (t = TIME_inf, a = Lnk->Alive, tact = NULL; a != NULL;
		a = a -> next) {

		if (a->LRId > LRId) continue;

		if (a->Type != TRANSMISSION) continue;
#if ZZ_FLK
  if (Lnk->FType > FT_LEVEL0 && flagSet (a->Pkt.Flags, PF_damaged)) continue;
#endif
		if (flagSet ((a->Pkt).Flags, PF_broadcast)) {

			// A broadcast packet
			ts = a->FTime + DV [a->LRId];
			if ((ts < Time) || (ts >= t)) continue;
			// Check if addressed to this station
			if (((SGroup*)((a->Pkt).Receiver))->
				occurs (TheStation->Id)) {

				t = ts;
				tact = a;
			}
			continue;
		}

		// A non-broadcast packet

		if ((a->Pkt).Receiver != TheStation->Id) continue;
		ts = a->FTime + DV [a->LRId];
		if ((ts >= Time) && (ts < t)) {
			t = ts;
			tact = a;
		}
	}

	if (tact != NULL) tpckt = &(tact->Pkt);
	return (t);

      case LT_pointtopoint:

	for (a = initLAI (); a != NULL; a = a->next) {

		// Find the first packet addressed to me

		if (a->Type != TRANSMISSION || a->LRId > LRId) continue;

		if (!flagSet ((a->Pkt).Flags, PF_broadcast)) {
			if ((a->Pkt).Receiver != TheStation->Id)
				continue;
		} else {
			if (!(((SGroup*)((a->Pkt).Receiver))->
				occurs (TheStation->Id)))
					continue;
		}
#if ZZ_FLK
  if (Lnk->FType > FT_LEVEL0 && flagSet (a->Pkt.Flags, PF_damaged)) continue;
#endif
		t = a->FTime + DV [a->LRId];
		if (t >= Time) break;
	}

	if (a == NULL) {
		tact = NULL;
		return (TIME_inf);
	}

	tact = a;

	for (a = a->next; a != NULL; a = a->next) {

		if (a->Type != TRANSMISSION || a->LRId > LRId) continue;
		if (a->STime + DV [a->LRId] >= t) break;

		if (flagSet ((a->Pkt).Flags, PF_broadcast)) {
			if (!(((SGroup*)((a->Pkt).Receiver))->
				occurs (TheStation->Id)))
					continue;
		} else {
			if ((a->Pkt).Receiver != TheStation->Id)
					continue;
		}

		ts = a->FTime + DV [a->LRId];
		if (ts < Time) continue;
#if ZZ_FLK
  if (Lnk->FType > FT_LEVEL0 && flagSet (a->Pkt.Flags, PF_damaged)) continue;
#endif
		if (ts < t) {
			t = ts;
			tact = a;
		}
	}

	tpckt = &(tact->Pkt);
	return (t);

      default: excptn ("Port->empTime: illegal link type %1d", Lnk->Type);
      return (TIME_0);
    }
}

TIME    Port::aevTime () {

/* ------------------------------------------------- */
/* Detects the nearest beginning/end of any activity */
/* ------------------------------------------------- */

    TIME               t, ts;
    ZZ_LINK_ACTIVITY   *a;

    switch (Lnk->Type) {

      case LT_broadcast:
      case LT_cpropagate:

	for (t = TIME_inf, a = Lnk->Alive, tact = NULL; a != NULL;
		a = a -> next) {

		ts = a->STime + DV [a->LRId];
		if (ts < Time) {

			// Started too soon

			if (undef (a->FTime)) continue;
			ts = a->FTime + DV [a->LRId];
			if ((ts >= Time) && (ts <= t)) {
				if (ts == t && FLIP) continue;
				t = ts;
				if ((tact = a) -> Type == JAM) {
					rinfo = EOJ;
				} else if (a -> Type == TRANSMISSION) {
					rinfo = EOT;
				} else {
					rinfo = ABTPACKET;
				}
			}
		} else {
			if ((ts < t) || ((ts == t) && FLIP)) {
				t = ts;
				if ((tact = a) -> Type == JAM)
					rinfo = BOJ;
				else
					rinfo = BOT;
			}
		}
	}

	if ((tact != NULL) && (tact->Type != JAM)) tpckt = &(tact->Pkt);
	return (t);

      case LT_unidirectional:

	for (t = TIME_inf, a = Lnk->Alive, tact = NULL; a != NULL;
		a = a -> next) {

		if (a->LRId > LRId) continue;

		ts = a->STime + DV [a->LRId];
		if (ts < Time) {

			// Started too soon

			if (undef (a->FTime)) continue;
			ts = a->FTime + DV [a->LRId];
			if ((ts >= Time) && (ts <= t)) {

				if (ts == t && FLIP) continue;

				t = ts;
				if ((tact = a) -> Type == JAM) {
					rinfo = EOJ;
				} else if (a -> Type == TRANSMISSION) {
					rinfo = EOT;
				} else {
					rinfo = ABTPACKET;
				}
			}
		} else {
			if ((ts < t) || ((ts == t) && FLIP)) {

				t = ts;
				if ((tact = a) -> Type == JAM)
					rinfo = BOJ;
				else
					rinfo = BOT;
			}
		}
	}

	if ((tact != NULL) && (tact->Type != JAM)) tpckt = &(tact->Pkt);
	return (t);

      case LT_pointtopoint:

	for (a = initLAI (); a != NULL; a = a->next) {
		if (a->LRId > LRId) continue;
		t = a->STime + DV [a->LRId];
		if (t >= Time) {
			// Beginning of activity
			if ((tact = a) -> Type != JAM) {
				rinfo = BOT;
				tpckt = &(tact->Pkt);
			} else {
				rinfo = BOJ;
			}
			return (t);
		}
		if (def (a->FTime)) {
			t = a->FTime + DV [a->LRId];
			if (t >= Time) goto LRQAEV;
		}
	}

	tact = NULL;
	return (TIME_inf);

LRQAEV:

	if ((tact = a) -> Type == JAM) {
		rinfo = EOJ;
	} else if (a -> Type == TRANSMISSION) {
		rinfo = EOT;
	} else {
		rinfo = ABTPACKET;
	}

	for (a=a->next; a != NULL; a = a->next) {

		if (a->LRId > LRId) continue;

		ts = a->STime + DV [a->LRId];
		if (ts >= t) return (t);
		if (ts >= Time) {
			if ((tact = a) -> Type != JAM) {
				rinfo = BOT;
				tpckt = &(tact->Pkt);
			} else {
				rinfo = BOJ;
			}
			return (t);
		}

		if (undef (a->FTime)) continue;
		ts = a->FTime + DV [a->LRId];
		if (ts < Time) continue;
		if (ts < t) {
			t = ts;
			if ((tact = a) -> Type == JAM) {
				rinfo = EOJ;
			} else if (a -> Type == TRANSMISSION) {
				rinfo = EOT;
			} else {
				rinfo = ABTPACKET;
			}
		}
	}

	return (t);

      default: excptn ("Port->aevTime: illegal link type %1d", Lnk->Type);
      return (TIME_0);
    }
}

#if  ZZ_TAG
void    Port::wait (int ev, int pstate, LONG tag) {
	int q;
#else
void    Port::wait (int ev, int pstate) {
#endif

/* ---------------------------- */
/* The port wait request server */
/* ---------------------------- */

	TIME               t, ts;
	ZZ_LINK_ACTIVITY   *a;
	int                evid;

	if_from_observer ("Port->wait: called from an observer");

	evid = ev;
	tpckt = NULL;

	if (zz_c_first_wait) {

		zz_c_other = NULL;    // No requests so far

		// Create new simulation event
		zz_c_wait_event = new ZZ_EVENT;
		zz_c_wait_event -> station = TheStation;
		zz_c_wait_event -> process = TheProcess;
	}

	rinfo = (IPointer) this;

	switch (ev)     {

	case    ACTIVITY:

		t = activityTime ();
		break;

	case    SILENCE:

		t = silenceTime ();
		break;

	case    COLLISION:

	  if (Activity != NULL) {

	    // Use a faster method -- determine when the station will hear
	    // the earliest activity originated by another station

	    evid = ZZ_MY_COLLISION;

	    switch (Lnk->Type) {

	      case LT_broadcast:
	      case LT_cpropagate:

		for (t = TIME_inf, a = Lnk->Alive; a != NULL; a = a -> next) {

			if (a->LRId == LRId) continue;  // Ignore this port

			ts = a->STime + DV [a->LRId];
			if (ts < Time) {
				// Started soon enough to be heard by the 
				// station

				if (undef (a->FTime) ||
					(a->FTime + DV [a->LRId] > Time)) {

					// And still being heard

					t = Time;
					break;

				}
				// Ignore otherwise
			} else {
				if (ts < t) t = ts;
			}
		}

		break;

	      case LT_unidirectional:

		for (t = TIME_inf, a = Lnk->Alive; a != NULL; a = a -> next) {

			if (a->LRId >= LRId) continue;

			ts = a->STime + DV [a->LRId];
			if (ts < Time) {
				// Started soon enough to be heard by the 
				// station

				if (undef (a->FTime) ||
					(a->FTime + DV [a->LRId] > Time)) {

					// And still being heard

					t = Time;
					break;

				}
				// Ignore otherwise
			} else {
				if (ts < t) t = ts;
			}
		}

		break;

	      case LT_pointtopoint:

		a = initLAI ();

		assert (a != NULL,
		    "Port->collisionTime: internal error -- missing activity");

		for (t = TIME_inf; a != NULL; a = a->next) {

			if (a->LRId >= LRId) continue;

			if (def (a->FTime) && (a->FTime + DV [a->LRId] <= Time))
				continue;

			t = a->STime + DV [a->LRId];
			if (t < Time) t = Time;
			break;
		}

		break;

	      default:
		excptn ("Port->collisionTime: illegal link type %1d",
		  Lnk->Type);
	    }

	  } else {

	      // General collision

	      t = collisionTime (YES);
	  }

	  break;

	case    BOJ:

		// The nearest beginning of a jamming signal

		t = bojTime ();
		break;

	case    EOJ:

		// The nearest end of a jamming signal

		t = eojTime ();
		break;

	case    EOT:

		// Determine when the port will hear an end of transfer

		t = eotTime ();
		break;

	case    BOT:

		// Beginning of transfer

		t = botTime ();
		break;

	case    BMP:

		// The beginning of a packet addressed to me

		t = bmpTime ();
		break;

	case    EMP:

		// The end of a packet addressed to me

		t = empTime ();
		break;

	case    ANYEVENT:

		// Any beginning/end of activity

		t =  AevMode ? aevTime () : TIME_inf;
		break;

	default : excptn ("Port->wait: illegal event %1d", ev);

	} // END SWITCH

	// Create the request and queue it at the link

	new ZZ_REQUEST (&(Lnk->RQueue [evid]), this, ev, pstate,
		LRId, (void*) tpckt, (void*) rinfo, (void*) this);

	assert (zz_c_other != NULL,
		"Port->wait: internal error -- null request");

	if (zz_c_first_wait) {

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
		zz_c_wait_event -> Info01 = (void*) tpckt;
		zz_c_wait_event -> Info02 = (void*) rinfo;

		zz_c_whead = zz_c_other;
		zz_c_other -> when = zz_c_wait_tmin;
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;

		zz_c_first_wait = NO;

		if (def (t))
			zz_c_wait_event->enqueue ();
		else
			zz_c_wait_event->store ();

	} else {

#if     ZZ_TAG
		if (def (t) && ((q = zz_c_wait_tmin . cmp (t, tag)) > 0 ||
			(q == 0 && FLIP))) {
#else
		if (def (t) && (zz_c_wait_tmin > t || (zz_c_wait_tmin == t &&
			FLIP))) {
#endif

			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = ev;
#if     ZZ_TAG
			zz_c_wait_tmin . set (t, tag);
			zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
			zz_c_wait_event -> waketime = zz_c_wait_tmin = t;
#endif
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = (void*) tpckt;
			zz_c_wait_event -> Info02 = (void*) rinfo;

			zz_c_wait_event -> reschedule ();
		}
#if     ZZ_TAG
		zz_c_other -> when . set (t, tag);
#else
		zz_c_other -> when = t;
#endif
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;
	}
}

#if ZZ_FLK
void    Link::packetDamage (Packet *packet) {

/* ---------------------------------------- */
/* Determines whether the packet is damaged */
/* ---------------------------------------- */

	double r;

	if (FType != NONE) {
	    if ((r = (packet->TLength-packet->ILength) * FRate) > 0.001) {
		// Approximation is unsafe
		r = 1.0 - pow (1.0 - FRate,
		    (double) (packet->TLength-packet->ILength));
	    }
	    if (rnd (SEED_toss) < r) {
		setFlag (packet->Flags, PF_hdamaged);
		setFlag (packet->Flags, PF_damaged);
	    } else {
		clearFlag (packet->Flags, PF_hdamaged);
		if ((r = (packet->ILength) * FRate) > 0.001) {
		    // Approximation is unsafe
		    r = 1.0 - pow (1.0 - FRate, (double) packet->ILength);
		}
		if (rnd (SEED_toss) < r) {
		    setFlag (packet->Flags, PF_damaged);
		} else {
		    clearFlag (packet->Flags, PF_damaged);
		}
	    }
	} else {
	    clearFlag (packet->Flags, PF_hdamaged);
	    clearFlag (packet->Flags, PF_damaged);
	}
};
#endif

void    Port::startTransmit (Packet *packet) {

/* --------------------------- */
/* Start transmitting a packet */
/* --------------------------- */

	ZZ_LINK_ACTIVITY   *a;

	if_from_observer ("Port->startTransmit: called from an observer");

	if (Lnk->FlgSPF == ON) Lnk->NTAttempts ++;
#if ZZ_FLK
	Lnk -> packetDamage (packet);
	if (flagSet (packet->Flags, PF_damaged)) Lnk->spfmPDM (packet);
#endif
	new_activity = zz_gen_activity (LRId, this, TRANSMISSION_A, packet);
	Info01 = (void*) (tpckt = &(new_activity -> Pkt));      // ThePacket
	Info02 = (void*) (tpckt -> TP);                         // TheTraffic

	assert (Activity == NULL,
		"Port->startTransmit: %s, multiple activities on the port",
			getSName ());

	assert (packet->TLength > 0,
		"Port->startTransmit: %s, illegal packet length %1d",
			getSName (), packet->TLength);

	if (Lnk->Type == LT_pointtopoint) {

		// Optimization for a point-to-point unidirectional link

		new_activity -> ae = NULL;

		// Skip to the first activity whose beginning does
		// not precede us
		for (a = initLAI ();
			a != NULL && (a->STime + DV [a->LRId] < Time);
					a = a->next);
		if (a == NULL) {
			// Queue at tail
			if (Lnk -> AliveTail == NULL) {
				pool_in (new_activity, Lnk->Alive);
				Lnk -> AliveTail = new_activity;
			} else {
				Lnk->AliveTail->next = new_activity;
				new_activity->prev = Lnk->AliveTail;
				new_activity->next = NULL;
				Lnk->AliveTail = new_activity;
			}
		} else {
			// Queue before a
			(new_activity -> prev = a -> prev) -> next =
					new_activity;
			new_activity -> next = a;
			a -> prev = new_activity;
		}

		if (Interpolator == NULL ||
			Interpolator->STime + DV [Interpolator->LRId] >= Time) {

			Interpolator = new_activity;
			IJTime = Time + MaxDistance;
			// Finished time unknown -- pessimistic estimate
		}
				
	} else
		pool_in (new_activity, Lnk->Alive);

	Activity = new_activity;

	Lnk->NAlive ++;
	Lnk->NAliveTransfers ++;

	reschedule_act (BOT);   // Reschedule those waiting for activities
	reschedule_col (BOT);   // ... and collisions
	reschedule_sil (BOT);   // ... and silence
#if ZZ_FLK
  if (Lnk->FType <= FT_LEVEL0 || flagCleared (tpckt->Flags, PF_hdamaged))
#endif
	reschedule_mpa ();           // ... and my_packet
	reschedule_aev (BOT); 	     // ... and any_event
}

void    Port::startJam () {

/* ------------------------------- */
/* Start emitting a jamming signal */
/* ------------------------------- */

	ZZ_LINK_ACTIVITY   *a;

	if_from_observer ("Port->startJam: called from an observer");

	if (Lnk->FlgSPF == ON) Lnk->NTJams ++;

	new_activity = zz_gen_activity (LRId, this, JAM);

	switch (Lnk->Type) {

	  case LT_broadcast:
	  case LT_unidirectional:

		pool_in (new_activity, Lnk->Alive);
		break;

	  case LT_cpropagate:

		pool_in (new_activity, Lnk->Alive);

		// Look for a transfer attempt originated by this port
		// ending at Time (optimization for CPROPAGATE link type)

		for (a = Lnk->Alive; a != NULL; a = a -> next)
			if ((a -> LRId == LRId) &&
			    (a -> Type == TRANSMISSION_A) &&
				(a -> FTime >= Time)) {

				a -> Type = TRANSMISSION_J;
				break;
			}
		break;

	  case LT_pointtopoint:

		// Optimization for a point-to-point unidirectional link

		new_activity -> ae = NULL;

		// Skip to the first activity whose beginning does
		// not precede us
		for (a = initLAI ();
			a != NULL && (a->STime + DV [a->LRId] < Time);
					a = a->next);
		if (a == NULL) {
			// Queue at tail
			if (Lnk -> AliveTail == NULL) {
				pool_in (new_activity, Lnk->Alive);
				Lnk -> AliveTail = new_activity;
			} else {
				Lnk->AliveTail->next = new_activity;
				new_activity->prev = Lnk->AliveTail;
				new_activity->next = NULL;
				Lnk->AliveTail = new_activity;
			}
		} else {
			// Queue before a
			(new_activity -> prev = a -> prev) -> next =
					new_activity;
			new_activity -> next = a;
			a -> prev = new_activity;
		}

		if (Interpolator == NULL ||
			Interpolator->STime + DV [Interpolator->LRId] >= Time) {

			Interpolator = new_activity;
			IJTime = Time + MaxDistance;
			// Finished time unknown -- pessimistic estimate
		}

		break;

	  default: excptn ("Port->startJam: illegal link type %1d", Lnk->Type);
	}

	assert (Activity == NULL,
		"Port->startJam: %s, multiple activities at the port",
			getSName ());

	Activity = new_activity;

	Lnk->NAlive ++;
	Lnk->NAliveJams ++;

	reschedule_act (BOJ);   // Reschedule those waiting for activities
	reschedule_col (BOJ);   // ... and collisions
	reschedule_sil (BOJ);   // ... and silence
	reschedule_boj ();      // ... and beginning of jam
	reschedule_eoj (BOJ);
	reschedule_aev (BOJ);   // ... and any event
}

int     Port::stop () {

/* ------------------------------ */
/* Terminate the current activity */
/* ------------------------------ */

	ZZ_LINK_ACTIVITY   *bac;
	TIME               t;
	ZZ_EVENT           *hint;

	if_from_observer ("Port->stop: called from an observer");

	bac = Activity;
	Activity = NULL;

	assert (bac != NULL, "Port->stop: %s, no activity to stop",
		getSName ());
	bac -> FTime = Time;

	// Schedule event to remove the activity from the link

	t = Time + MaxDistance;

	if (Lnk->Type == LT_pointtopoint)  {

		// Update archive time estimate for the interpolator
		if (bac == Interpolator) IJTime = t;

		// Try to find a 'hint' event for scheduling link removal
		// for this activity.
		if ((hint = findHint (bac)) != NULL) {
			bac->ae = new ZZ_EVENT (hint, t, System,
				(void*) bac, NULL, shandle, Lnk, LNK_PURGE,
					RemFromLk, NULL);
		} else {
			bac->ae = new ZZ_EVENT (t, System,
				(void*) bac, NULL, shandle, Lnk, LNK_PURGE,
					RemFromLk, NULL);
		}
	} else
		new ZZ_EVENT (t, System, (void*) bac, NULL,
			shandle, Lnk, LNK_PURGE, RemFromLk, NULL);

	if (bac->Type == JAM) {
		reschedule_sil (EOJ);
		reschedule_eoj (EOJ);
		reschedule_aev (EOJ);
		return (JAM);                    // That's all for a jam
	}

	Info01 = (void*) (tpckt = &(bac -> Pkt));       // ThePacket
	Info02 = (void*) (tpckt->TP);                   // TheTraffic

	bac->Type = TRANSMISSION;		// A properly terminated packet

	// Update link counters

	Lnk->spfmPTR (tpckt);                   // Update link measures
	if (tpckt->isStandard () && tpckt->isLast ())
		Lnk->spfmMTR (tpckt);
#if ZZ_FLK
	if (Lnk->FType <= FT_LEVEL0) {
	   reschedule_emp ();
	   reschedule_eot ();
	} else {
	  if (flagCleared (tpckt->Flags, PF_damaged)) {
	     reschedule_eot ();
	     reschedule_emp ();
	  } else if (Lnk->FType <= FT_LEVEL1) {
	     reschedule_eot ();
	  }
	}
#else
	reschedule_eot ();
	reschedule_emp ();
#endif
	reschedule_aev (EOT);
	reschedule_sil (EOT);
	reschedule_col (EOT);
	return (TRANSMISSION);
}

int     Port::abort () {

/* -------------------------- */
/* Abort the current activity */
/* -------------------------- */

	ZZ_LINK_ACTIVITY   *bac;
	TIME               t;
	ZZ_EVENT           *hint;

	if_from_observer ("Port->abort: called from an observer");

	bac = Activity;
	Activity = NULL;

	assert (bac != NULL, "Port->abort: %s, no activity to abort",
		getSName ());
	bac -> FTime = Time;

	// Schedule event to remove the activity from the link

	t = Time + MaxDistance;

	if (Lnk->Type == LT_pointtopoint)  {

		// Update archive time estimate for the interpolator
		if (bac == Interpolator) IJTime = t;

		// Try to find a 'hint' event for scheduling link removal
		// for this activity.
		if ((hint = findHint (bac)) != NULL)
			bac->ae = new ZZ_EVENT (hint, t, System,
				(void*) bac, NULL, shandle, Lnk, LNK_PURGE,
					RemFromLk, NULL);
		else
			bac->ae = new ZZ_EVENT (t, System,
				(void*) bac, NULL, shandle, Lnk, LNK_PURGE,
					RemFromLk, NULL);
	} else
		new ZZ_EVENT (t, System, (void*) bac, NULL,
			shandle, Lnk, LNK_PURGE, RemFromLk, NULL);

	if (bac->Type == JAM) {
		reschedule_sil (EOJ);
		reschedule_eoj (EOJ);
		reschedule_aev (EOJ);
		return (JAM);
	}

	Info01 = (void*) (tpckt = &(bac -> Pkt));       // ThePacket
	Info02 = (void*) (tpckt->TP);                   // TheTraffic

	reschedule_aev (ABTPACKET);
	reschedule_sil (EOT);
	reschedule_col (EOT);
	return (TRANSMISSION);
}

void    Port::reschedule_act    (int act) {

/* ----------------------------------------------------- */
/* Attempts to reschedule processes waiting for activity */
/* ----------------------------------------------------- */

	ZZ_REQUEST      *rq;
	ZZ_EVENT        *ev;
	TIME            t;
#if ZZ_TAG
	int q;
#endif
	if (act == BOT) {

		for (rq = (Lnk->RQueue) [ACTIVITY]; rq != NULL;
			rq = rq -> next) {

			if ((Lnk->Type >= LT_unidirectional) &&
				// Ignore downstream activities
				(rq->id < LRId)) continue;

			t = Time + DV [rq->id];
			if (rq->when <= t) continue;

			rq -> Info01 = (void*) tpckt;
#if     ZZ_TAG
			rq->when . set (t);
			if (((q=(ev = rq->event)->waketime.cmp (rq->when)) < 0)
			  || ((q == 0) && FLIP))
			    continue;
#else
			rq->when = t;
			if (((ev = rq->event) -> waketime < t) ||
				((ev -> waketime == t) && FLIP))
					continue;
#endif
			ev->new_top_request (rq);
		}
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (tpckt->Flags, PF_hdamaged)) return;
#endif
		for (rq = (Lnk->RQueue) [BOT]; rq != NULL; rq = rq -> next) {

			if ((Lnk->Type >= LT_unidirectional) &&
				(rq->id < LRId)) continue;
			t = Time + DV [rq->id];
			if (rq->when <= t) continue;

			rq -> Info01 = (void*) tpckt;
#if     ZZ_TAG
			rq->when . set (t);
			if (((q=(ev = rq->event)->waketime.cmp (rq->when)) < 0)
			  || ((q == 0) && FLIP))
			    continue;
#else
			rq->when = t;
			if (((ev = rq->event) -> waketime < t) ||
				((ev -> waketime == t) && FLIP))
					continue;
#endif
			ev->new_top_request (rq);
		}

		return;
	} 

	for (rq = (Lnk->RQueue) [ACTIVITY]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		t = Time + DV [rq->id];
		if (rq->when <= t) continue;

		rq -> Info01 = NULL;
#if     ZZ_TAG
		rq->when . set (t);
		if (((q=(ev = rq->event)->waketime.cmp (rq->when)) < 0)
		  || ((q == 0) && FLIP))
		    continue;
#else
		rq->when = t;
		if (((ev = rq->event) -> waketime < t) ||
			((ev -> waketime == t) && FLIP)) continue;
#endif
		ev->new_top_request (rq);
	}
}

void    Port::reschedule_aev (int act) {

/* ------------------------------------------------------ */
/* Attempts to reschedule processes waiting for any_event */
/* ------------------------------------------------------ */

	ZZ_REQUEST      *rq;
	ZZ_EVENT        *ev;
	TIME            t;
#if ZZ_TAG
	int q;
#endif

	for (rq = (Lnk->RQueue) [ANYEVENT]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		t = Time + DV [rq->id];
		if (rq->when < t) continue;
		if ((rq->when == t) && FLIP) continue;

		if ((act == BOT) || (act == EOT)) {
			rq -> Info01 = (void*) tpckt;
		} else {
			rq -> Info01 = NULL;
		}

		rq -> Info02 = (void*) act;
#if     ZZ_TAG
		rq->when . set (t);
		if (((q=(ev = rq->event)->waketime.cmp (rq->when)) < 0)
		  || ((q == 0) && FLIP))
		    continue;
#else
		rq->when = t;
		if (((ev = rq->event) -> waketime < t) ||
			((ev -> waketime == t) && FLIP)) continue;
#endif
		ev->new_top_request (rq);

	}
}

void    Port::reschedule_mpa () {

/* ------------------------------------------------------ */
/* Attempts to reschedule processes waiting for my_packet */
/* ------------------------------------------------------ */

	ZZ_REQUEST         *rq;
	ZZ_EVENT           *ev;
	TIME               t;
#if ZZ_TAG
	int q;
#endif

	// The parameter is ignored

	if (flagSet (tpckt->Flags, PF_broadcast)) {

		// A broadcast packet

		for (rq = (Lnk->RQueue) [BMP]; rq != NULL;
			rq = rq -> next) {

			if ((Lnk->Type >= LT_unidirectional) &&
				(rq->id < LRId)) continue;

			t = Time + DV [rq->id];
			if (rq->when <= t) continue;
			if (!(((SGroup*) (tpckt->Receiver))->occurs (((ev=
				rq->event)->station)->Id))) continue;
			rq -> Info01 = (void*) tpckt;
#if     ZZ_TAG
			rq->when . set (t);
			if (((q=ev->waketime.cmp (rq->when)) < 0)
			  || ((q == 0) && FLIP))
			    continue;
#else
			rq->when = t;
			if ((ev -> waketime < t) ||
				((ev -> waketime == t) && FLIP))
					continue;
#endif
			ev->new_top_request (rq);
		}

		return;
	}

	for (rq = (Lnk->RQueue) [BMP]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		if ((ev = rq->event) -> station -> Id !=
			tpckt -> Receiver) continue;

		t = Time + DV [rq->id];
		if (rq->when <= t) continue;

		rq -> Info01 = (void*) tpckt;
#if     ZZ_TAG
		rq->when . set (t);
		if (((q=ev->waketime.cmp (rq->when)) < 0)
		  || ((q == 0) && FLIP))
		    continue;
#else
		rq->when = t;
		if ((ev -> waketime < t) ||
			((ev -> waketime == t) && FLIP))
				continue;
#endif
		ev->new_top_request (rq);
	}
}

void    Port::reschedule_emp () {

/* ---------------------------------------------------------- */
/* Attempts  to  reschedule  processes  waiting  for  end  of */
/* my_packet                                                  */
/* ---------------------------------------------------------- */

	ZZ_REQUEST      *rq;
	ZZ_EVENT        *ev;
	TIME            t;
#if ZZ_TAG
	int q;
#endif

	// The parameter is ignored

	if (flagSet (tpckt->Flags, PF_broadcast)) {

		// A broadcast packet

		for (rq = (Lnk->RQueue) [EMP]; rq != NULL;
			rq = rq -> next) {

			if ((Lnk->Type >= LT_unidirectional) &&
				(rq->id < LRId)) continue;

			t = Time + DV [rq->id];
			if (rq->when <= t) continue;
			if (!(((SGroup*) (tpckt->Receiver))->occurs (((ev=
				rq->event)->station)->Id))) continue;
			rq -> Info01 = (void*) tpckt;
#if     ZZ_TAG
			rq->when . set (t);
			if (((q=ev->waketime.cmp (rq->when)) < 0)
			  || ((q == 0) && FLIP))
			    continue;
#else
			rq->when = t;
			if ((ev -> waketime < t) ||
				((ev -> waketime == t) && FLIP))
					continue;
#endif
			ev->new_top_request (rq);
		}

		return;
	}

	for (rq = (Lnk->RQueue) [EMP]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		if ((ev = rq->event) -> station -> Id !=
			tpckt -> Receiver) continue;

		t = Time + DV [rq->id];
		if (rq->when <= t) continue;

		rq -> Info01 = (void*) tpckt;
#if     ZZ_TAG
		rq->when . set (t);
		if (((q=ev->waketime.cmp (rq->when)) < 0)
		  || ((q == 0) && FLIP))
		    continue;
#else
		rq->when = t;
		if ((ev -> waketime < t) ||
			((ev -> waketime == t) && FLIP))
				continue;
#endif
		ev->new_top_request (rq);
	}
}

void    Port::reschedule_col  (int act) {

/* ------------------------------------------------------ */
/* Attempts to reschedule processes waiting for collision */
/* ------------------------------------------------------ */

	ZZ_REQUEST      *rq, *rm;
	ZZ_EVENT        *ev;
	TIME            t;
#if ZZ_TAG
	int q;
#endif
	switch (act) {

	case    BOJ:

	for (rq = (Lnk->RQueue) [COLLISION]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		t = Time + DV [rq->id];
		if (rq->when <= t) continue;
#if     ZZ_TAG
		rq->when . set (t);
		if (((q=(ev = rq->event)->waketime.cmp (rq->when)) < 0)
		  || ((q == 0) && FLIP))
		    continue;
#else
		rq->when = t;
		if (((ev = rq->event) -> waketime < t) ||
			((ev -> waketime == t) && FLIP)) continue;
#endif
		ev->new_top_request (rq);
	}

	break;

	case    BOT:

	for (rq = (Lnk->RQueue) [COLLISION]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		if (rq->when <= Time + DV [rq->id]) continue;

		t = ((Port*)(rq->what))->collisionTime (NO);

		if (t >= rq->when) {
			// This is not an error. Only transfers have been
			// considered, so an earlier collision may be due
			// to a jam.
			continue;
		}
#if     ZZ_TAG
		rq->when . set (t);
		if (((q=(ev = rq->event)->waketime.cmp (rq->when)) < 0)
		  || ((q == 0) && FLIP))
		    continue;
#else
		rq->when = t;
		if (((ev = rq->event) -> waketime < t) ||
			((ev -> waketime == t) && FLIP)) continue;
#endif
		ev->new_top_request (rq);
	}

	break;

	case    EOT:

	// Some collisions may be postponed

	for (rq = (Lnk->RQueue) [COLLISION]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		if (rq->when < Time + DV [rq->id]) continue;

		t = ((Port*)(rq->what))->collisionTime (YES);

		if (t <= rq->when) {
			assert (t == rq->when,
		"Port->reschedule_col: internal error -- earlier after EOT");
			continue;
		}

#if     ZZ_TAG
		rq->when . set (t);     // Collision postponed
#else
		rq->when = t;           // Collision postponed
#endif
		ev = rq -> event;

		if (ev->chain == rq) {  // Find new minimum
			rm = rq->min_request ();
			ev->new_top_request (rm);
		}
	}

	break;

	} /* END SWITCH */

	switch  (act) {         // Takes care of MY_COLLISION

	case    BOJ:
	case    BOT:

	for (rq = (Lnk->RQueue) [ZZ_MY_COLLISION]; rq != NULL; rq = rq->next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		t = Time + DV [rq->id];
		if (rq->when <= t) continue;

		ev = rq -> event;
#if     ZZ_TAG
		rq->when . set (t);
		if (((q=ev->waketime.cmp (rq->when)) < 0)
		  || ((q == 0) && FLIP))
		    continue;
#else
		rq->when = t;
		if ((ev -> waketime < t) || ((ev -> waketime == t) && FLIP))
				continue;
#endif
		ev->new_top_request (rq);
	}

	case    EOT    :

	break;

	} /* END SWITCH */
}

void    Port::reschedule_sil  (int act) {

/* ---------------------------------------------------- */
/* Attempts to reschedule processes waiting for silence */
/* ---------------------------------------------------- */

	ZZ_REQUEST      *rq, *rm;
	ZZ_EVENT        *ev;
	TIME            t;
#if ZZ_TAG
	int q;
#endif

	switch (act) {

	case    EOT:
	case    EOJ:

	for (rq = (Lnk->RQueue) [SILENCE]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		t = Time + DV [rq->id];
		if (rq->when <= t) continue;

		t = ((Port*)(rq->what))->silenceTime ();

		if (t >= rq -> when) {
			assert (t == rq->when,
		"Port->reschedule_sil: internal error -- later after EOT");
			continue;
		}

		ev = rq -> event;
#if     ZZ_TAG
		rq->when . set (t);
		if (((q=ev->waketime.cmp (rq->when)) < 0)
		  || ((q == 0) && FLIP))
		    continue;
#else
		rq->when = t;
		if ((ev -> waketime < t) || ((ev -> waketime == t) && FLIP))
				continue;
#endif
		ev->new_top_request (rq);
	}

	break;

	case    BOT:
	case    BOJ:

	// Some 'silences' may be postponed

	for (rq = (Lnk->RQueue) [SILENCE]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		t = Time + DV [rq->id];
		if (rq->when < t) continue;

		t = ((Port*)(rq->what))->silenceTime ();

		if (t <= rq->when) {
			assert (t == rq->when,
	"Port->reschedule_sil: internal error -- earlier after BOT/BOJ");
			continue;
		}

		ev = rq -> event;
#if     ZZ_TAG
		rq->when . set (t);             // Silence postponed
#else
		rq->when = t;                   // Silence postponed
#endif
		if (ev->chain == rq) {          // Find new minimum
			rm = rq->min_request ();
			ev->new_top_request (rm);
		}
	}

	break;

	} // END CASE
}

void    Port::reschedule_boj () {

/* ------------------------------------------------ */
/* Attempts to reschedule processes waiting for jam */
/* ------------------------------------------------ */

	ZZ_REQUEST      *rq;
	ZZ_EVENT        *ev;
	TIME            t;
#if ZZ_TAG
	int q;
#endif
	for (rq = (Lnk->RQueue) [BOJ]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		t = Time + DV [rq->id];
		if (rq->when <= t) continue;
#if     ZZ_TAG
		rq->when . set (t);
		if (((q=(ev = rq->event)->waketime.cmp (rq->when)) < 0)
		  || ((q == 0) && FLIP))
		    continue;
#else
		rq->when = t;
		if (((ev = rq->event) -> waketime < t) ||
			((ev -> waketime == t) && FLIP)) continue;
#endif
		ev->new_top_request (rq);

	}
}

void    Port::reschedule_eoj    (int act) {

/* ------------------------------------------------------- */
/* Attempts to reschedule processes waiting for end of jam */
/* ------------------------------------------------------- */

	ZZ_REQUEST      *rq, *rm;
	ZZ_EVENT        *ev;
	TIME            t;
#if ZZ_TAG
	int q;
#endif
	switch (act) {

	case    EOJ:

	for (rq = (Lnk->RQueue) [EOJ]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		t = Time + DV [rq->id];
		if (rq->when <= t) continue;

		t = ((Port*) (rq->what))->eojTime ();

		if (t >= rq -> when) {
			assert (t == rq->when,
		"Port->reschedule_eoj: internal error -- later after EOJ");
			continue;
		}

		ev = rq->event;
#if     ZZ_TAG
		rq->when . set (t);
		if (((q=ev->waketime.cmp (rq->when)) < 0)
		  || ((q == 0) && FLIP))
		    continue;
#else
		rq->when = t;
		if ((ev -> waketime < t) || ((ev -> waketime == t) && FLIP))
				continue;
#endif
		ev->new_top_request (rq);

	}

	break;

	case    BOJ:

	// Some eoj's may be postponed

	for (rq = (Lnk->RQueue) [EOJ]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		t = Time + DV [rq->id];
		if (rq->when < t) continue;

		t = ((Port*) (rq->what))->eojTime ();

		if (t <= rq->when) {
			assert (t == rq->when,
		"Port->reschedule_eoj: internal error -- earlier after BOJ");
			continue;
		}

#if     ZZ_TAG
		rq->when . set (t);   // Eoj postponed
#else
		rq->when = t;   // Eoj postponed
#endif
		ev = rq->event;

		if (ev->chain == rq) {          // Find new minimum
			rm = rq->min_request ();
			ev->new_top_request (rm);
		}
	}

	break;

	} // END CASE
}

void    Port::reschedule_eot () {

/* ---------------------------------------------------------- */
/* Attempts  to  reschedule  processes  waiting  for  end  of */
/* transfer                                                   */
/* ---------------------------------------------------------- */

	ZZ_REQUEST      *rq;
	ZZ_EVENT        *ev;
	TIME            t;
#if ZZ_TAG
	int q;
#endif
	// Parameter ignored - called only after successful EOT

	for (rq = (Lnk->RQueue) [EOT]; rq != NULL; rq = rq -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(rq->id < LRId)) continue;

		t = Time + DV [rq->id];
		if (rq->when <= t) continue;

		rq->Info01 = (void*) tpckt;
#if     ZZ_TAG
		rq->when . set (t);
		if (((q=(ev = rq->event)->waketime.cmp (rq->when)) < 0)
		  || ((q == 0) && FLIP))
		    continue;
#else
		rq->when = t;
		if (((ev = rq->event) -> waketime < t) ||
			((ev -> waketime == t) && FLIP)) continue;
#endif
		ev->new_top_request (rq);
	}
}

/* --------- */
/* Inquiries */
/* --------- */

TIME    Port::lastCOL () {

/* ---------------------------------------------------------- */
/* Link  archive  inquiry  for  the  last sensed beginning of */
/* collision                                                  */
/* ---------------------------------------------------------- */

	ZZ_LINK_ACTIVITY                        *a;
	struct activity_heard_struct            *aa;
	LONG               acs [ACSSIZE * (BIG_size + BIG_size + 1)];
	TIME                                    ts, tc, cs;
	int                                     i, n;
	int                                     j;
	ZZ_LINK_ACTIVITY                        *pa, *la;
	DISTANCE                                *dst;

	tc = TIME_inf;          // The first estimate

	Info01 = NULL;
	Info02 = (void*) this;

	for (a = Lnk->Alive, n = 0; a != NULL; a = a -> next) {

		// Make a list of all interesting activities, alive first

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

		ts = a->STime + DV [a->LRId];
		if (ts > Time) continue;

		// Ignore activities that start after the current time

		if (n >= ACSSIZE)
			excptn ("Port->lastCOL: stack overflow");

		((struct activity_heard_struct*)acs) [n  ].t_s = ts;
		((struct activity_heard_struct*)acs) [n  ].t_f =
			undef (a->FTime) ? TIME_inf : a->FTime + DV [a->LRId];
		((struct activity_heard_struct*)acs) [n++].up = (void*) a;
	}

	for (a = Lnk->Archived; a != NULL; a = a -> next) {

		// Take care of the archive

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

		if (n >= ACSSIZE)
			excptn ("Port->lastCOL: stack overflow");

		((struct activity_heard_struct*)acs) [n].t_s =
			a->STime + DV [a->LRId];
		((struct activity_heard_struct*)acs) [n].t_f =
			a->FTime + DV [a->LRId];
		((struct activity_heard_struct*)acs) [n++].up  = (void*) a;
	}

	if (n > 0) {

		// Sort the activities according to the 'finished' time
		// as perceived by the inquiring station.

		sort_acs_e (((struct activity_heard_struct*)acs), 0, n-1);
#ifdef  VERIFY_SORTS
		assert (sorted_acs_e (((struct activity_heard_struct*)acs), n),
		"Port->lastCOL: internal error -- acs not sorted");
#endif
		for (cs = (aa = &(((struct activity_heard_struct*)acs)
			[n-1]))->t_s,
			tc = ((pa = (ZZ_LINK_ACTIVITY*)(aa->up))->Type == JAM) ?
				cs : TIME_inf, i = n - 2; i >= 0; i--) {

			a = (ZZ_LINK_ACTIVITY*) ((aa =
			    &(((struct activity_heard_struct*)acs) [i])) -> up);

			if (aa -> t_f <= cs) {

			    if (Lnk->Type == LT_cpropagate) {

				if (tc > cs) {

					// Check if dealing with weird acti-
					// vities  that  have  to be checked
					// against general propagating coll.

					dst = (pa->GPort) -> DV;
					ts = pa -> STime;
					for (la = a, j = i; ;) {
						if (la->FTime >
							ts + dst [la->LRId]) {

							tc = cs;
							break;
						}
						if (--j < 0) break;
						la = (ZZ_LINK_ACTIVITY*)
				(((struct activity_heard_struct*)acs)[j].up);
					}
				}
			    }

				// A gap encountered - terminate if collision
				// already found.

				if (def (tc)) return (tc);

				cs = aa->t_s;

				pa = a;

				if (a -> Type == JAM) tc = cs;
			} else {
				ts = aa->t_s;
				if (a -> Type == JAM) {
					if (ts < tc) tc = ts;
				} else {
					if (ts < cs) {
						if (cs < tc) tc = cs;
					} else {
						if (ts < tc) tc = ts;
					}
				}
				if (ts < cs) {
					cs = ts;
					pa = a;
				}
			}
		}
	}

	return (def (tc) ? tc : TIME_0);
}

TIME    Port::lastEOA () {

/* -------------------------------------------------------- */
/* Link archive inquiry for the last sensed end of activity */
/* -------------------------------------------------------- */

	// Note: if an activity is heard at the current moment, TIME_inf is
	// returned.  If no activity can be found in the monitored time
	// interval, TIME_0 is returned.

	ZZ_LINK_ACTIVITY   *a, *b;
	TIME               tc, td;

	tc = TIME_0;    // Initial estimate

	Info01 = NULL;
	Info02 = (void*) this;

	for (a = Lnk->Alive; a != NULL; a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

		// Ignore activities that start after the current time
		if (a->STime + DV [a->LRId] > Time) continue;
		if (undef (a->FTime)) return (TIME_inf);
		td = a->FTime + DV [a->LRId];
		if (td > Time) return (TIME_inf);

		if (td > tc) {
			b = a;
			tc = td;
		}
	}

	for (a = Lnk->Archived; a != NULL; a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

		td = a->FTime + DV [a->LRId];
		if (td > tc) {
			b = a;
			tc = td;
		}
	}

	if (tc != TIME_0 && b->Type != JAM) Info01 = (void*) (&(b->Pkt));

	return (tc);
}

TIME    Port::lastBOT () {

/* ---------------------------------------------------------- */
/* Link  archive  inquiry  for  the  last sensed beginning of */
/* transfer                                                   */
/* ---------------------------------------------------------- */

	ZZ_LINK_ACTIVITY   *a, *b;
	TIME               t, tc;

	tc = TIME_0;
	Info01 = NULL;
	Info02 = (void*) this;

	for (a = Lnk->Alive; a != NULL; a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

		// Ignore activities that begin after the current time
		if (a->Type == JAM) continue;
		t = a->STime + DV [a->LRId];
		if (t > Time) continue;
		if (t > tc) {
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (a->Pkt.Flags, PF_hdamaged)) continue;
#endif
			b = a;
			tc = t;
		}
	}

	if (tc == TIME_0) {     // Must check in archive

		for (a = Lnk->Archived; a != NULL; a = a -> next) {

			if ((Lnk->Type >= LT_unidirectional) &&
				(a->LRId > LRId)) continue;

			if (a->Type != JAM) {
				t = a->STime + DV [a->LRId];
				if (t > tc) {
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (a->Pkt.Flags, PF_hdamaged)) continue;
#endif
					b = a;
					tc = t;
				}
			}
		}
	}

	if (tc != TIME_0) Info01 = (void*) (&(b->Pkt));

	return (tc);
}

TIME    Port::lastEOT () {

/* -------------------------------------------------------- */
/* Link archive inquiry for the last sensed end of transfer */
/* -------------------------------------------------------- */

	ZZ_LINK_ACTIVITY   *a, *b;
	TIME               t, tc;

	tc = TIME_0;
	Info01 = NULL;
	Info02 = (void*) this;

	for (a = Lnk->Alive; a != NULL; a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

		// Ignore activities other than completed transfers that end
		// before the current time
		if (a->Type != TRANSMISSION) continue;
		t = a->FTime + DV [a->LRId];
		if (t > Time) continue;

		if (t > tc) {
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (a->Pkt.Flags, PF_damaged)) continue;
#endif
			b = a;
			tc = t;
		}
	}

	if (tc == TIME_0) {     // Must check the archive

		for (a = Lnk->Archived; a != NULL; a = a -> next) {

			if ((Lnk->Type >= LT_unidirectional) &&
				(a->LRId > LRId)) continue;

			if (a->Type == TRANSMISSION) {
				t = a->FTime + DV [a->LRId];
				if (t > tc) {
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (a->Pkt.Flags, PF_damaged)) continue;
#endif
					b = a;
					tc = t;
				}
			}
		}
	}

	if (tc != TIME_0) Info01 = (void*) (&(b->Pkt));
	return (tc);
}

TIME    Port::lastBOJ () {

/* --------------------------------------------------------- */
/* Link archive inquiry for the last sensed beginning of jam */
/* --------------------------------------------------------- */

	ZZ_LINK_ACTIVITY                *a;
	struct activity_heard_struct    *aa;
	LONG       acs [ACSSIZE * (BIG_size + BIG_size + 1)];
	TIME                            ts, tc;
	int                             i, n;

	tc = TIME_inf;
	Info01 = NULL;
	Info02 = (void*) this;

	for (a = Lnk->Alive, n = 0; a != NULL; a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

		if (a->Type != JAM) continue;   // Jams only

                ts = a->STime + DV [a->LRId];

		if (ts > Time) continue;

		// Ignore activities that start after the current time

		if (n >= ACSSIZE) excptn ("Port->lastBOJ: stack overflow");

		((struct activity_heard_struct*)acs) [n  ].t_s = ts;
		((struct activity_heard_struct*)acs) [n++].t_f =
			def (a->FTime) ? a->FTime + DV [a->LRId] : TIME_inf;
	}

	// Now the archive
	for (a = Lnk->Archived; a != NULL; a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

		if (a->Type != JAM) continue;
		if (n >= ACSSIZE) excptn ("Port->lastBOJ: stack overflow");
		((struct activity_heard_struct*)acs) [n  ].t_s =
			a->STime + DV [a->LRId];
		((struct activity_heard_struct*)acs) [n++].t_f =
			a->FTime + DV [a->LRId];
	}

	if (n > 0) {

		sort_acs_e (((struct activity_heard_struct*)acs), 0, n-1);
#ifdef  VERIFY_SORTS
		assert (sorted_acs_e (((struct activity_heard_struct*)acs), n),
			"Port->lastBOJ: internal error -- acs not sorted");
#endif
		for (tc = ((struct activity_heard_struct*)acs) [n-1].t_s,
		    i = n-2; i >= 0; i--) {
			aa = &(((struct activity_heard_struct*)acs) [i]);
			if (aa->t_f < tc) return (tc);
			if (aa->t_s < ts) ts = aa->t_s;
		}
	}

	return (undef (tc) ? TIME_0 : tc);
}

TIME    Port::lastEOJ () {

/* --------------------------------------------------- */
/* Link archive inquiry for the last sensed end of jam */
/* --------------------------------------------------- */

	// Note: returns 'TIME_inf' if a jam is curently heard

	ZZ_LINK_ACTIVITY   *a;
	TIME               ts, tc;

	tc = TIME_0;
	Info01 = NULL;
	Info02 = (void*) this;

	for (a = Lnk->Alive; a != NULL; a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

		if (a->Type != JAM) continue;   // Jams only
		if (a->STime + DV [a->LRId] > Time) continue;
		if (undef (a->FTime)) return (TIME_inf);
		ts = a->FTime + DV [a->LRId];
		if (ts >= Time) return (TIME_inf);
		if (ts >= tc) tc = ts;
	}

	for (a = Lnk->Archived; a != NULL; a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

		if (a->Type == JAM) {
			ts = a->FTime + DV [a->LRId];
			if (ts >= tc) tc = ts;
		}
	}

	return (tc);
}

TIME    Port::lastBOA () {

/* ---------------------------------------------------------- */
/* Link  archive  inquiry  for  the  last sensed beginning of */
/* activity                                                   */
/* ---------------------------------------------------------- */

	ZZ_LINK_ACTIVITY                *a;
	struct activity_heard_struct    *aa;
	LONG     acs [ACSSIZE * (BIG_size + BIG_size + 1)];
	TIME                            ts, tc;
	int                             i, n;

	tc = TIME_inf;
	Info01 = NULL;
	Info02 = (void*) this;

	for (a = Lnk->Alive, n = 0; a != NULL; a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

                ts = a->STime + DV [a->LRId];

		if (ts > Time) continue;

		// Ignore activities that start after the current time

		if (n >= ACSSIZE) excptn ("Port->lastBOA: stack overflow");

		((struct activity_heard_struct*)acs) [n  ].up  = (void*) a;
		((struct activity_heard_struct*)acs) [n  ].t_s = ts;
		((struct activity_heard_struct*)acs) [n++].t_f =
			undef (a->FTime) ? TIME_inf :
			a->FTime + DV [a->LRId];
	}

	for (a = Lnk->Archived; a != NULL; a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

		if (n >= ACSSIZE) excptn ("Port->lastBOA: stack overflow");
		((struct activity_heard_struct*)acs) [n  ].up  = (void*) a;
		((struct activity_heard_struct*)acs) [n  ].t_s =
			a->STime + DV [a->LRId];
		((struct activity_heard_struct*)acs) [n++].t_f =
			a->FTime + DV [a->LRId];
	}

	if (n > 0) {

		sort_acs_e (((struct activity_heard_struct*)acs), 0, n-1);
#ifdef  VERIFY_SORTS
		assert (sorted_acs_e (((struct activity_heard_struct*)acs), n),
			"Port->lastBOA: internal error -- acs not sorted");
#endif
		for (tc = ((struct activity_heard_struct*)acs) [n-1].t_s,
		 a = (ZZ_LINK_ACTIVITY*) (((struct activity_heard_struct*)acs)
		  [n-1].up), i = n-2; i >= 0; i--) {

			aa = &(((struct activity_heard_struct*)acs) [i]);
			if (aa->t_f < tc) {
				if (a->Type != JAM)
					Info01 = (void*) (&(a->Pkt));
				return (tc);
			}
			if (aa->t_s < tc) {
				tc = aa->t_s;
				a = (ZZ_LINK_ACTIVITY*) (aa->up);
			}
		}
	}

	if (def (tc)) {
		if (a->Type != JAM)
			Info01 = (void*) (&(a->Pkt));
		return (tc);
	}

	return (TIME_0);
}

/* ---------------------------------------------------------- */
/* Variables used by 'anotherPacket' (see below) to recognize */
/* the  first  call  and  check the consistency of subsequent */
/* calls                                                      */
/* ---------------------------------------------------------- */
static  ZZ_LINK_ACTIVITY   *apca = NULL;   // Last activity pointer
static  Long               apevn = -2;     // Simulation event number
#if     ZZ_ASR
static  int                appid;          // Port Id
#endif

	// Note: the above variables should really be Port attributes.
	//       They have been put here to save some space in the Port
	//       data structure, as most protocols don't care about
	//       'anotherPacket'.

int     Port::activities (int &transfers, int &jams) {  // Return parameters

/* ---------------------------------------------------------- */
/* Link inquiry for the number of currently sensed activities */
/* ---------------------------------------------------------- */

	ZZ_LINK_ACTIVITY   *a;

	apevn = NONE;
	zz_temp_event_id = NONE;        // Flag for 'anotherPacket'
					// = look for active transfers
#if     ZZ_ASR
	Info02 = (void*) (appid = (int) GYID (Id)); // The port identifier
#else
	Info02 = (void*) GYID (Id);                 // The port identifier
#endif

	for (Info01 = NULL, transfers = jams = 0, a = Lnk->Alive; a != NULL;
		a = a -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(a->LRId > LRId)) continue;

		if (a->STime + DV [a->LRId] > Time) continue;
		if (def (a->FTime) && (a->FTime + DV [a->LRId] <= Time))
			continue;

		if (a->Type == JAM) jams++; else {
			transfers++;
			if (Info01 == NULL)
				Info01 = (void*) (& (a->Pkt));
		}
	}
	return (transfers + jams);
}

int     Port::events (int etype) {

/* ---------------------------------------------------------- */
/* Link  inquiry  for  the  currently  sensed  events  of the */
/* specified type                                             */
/* ---------------------------------------------------------- */

	ZZ_LINK_ACTIVITY   *a;
	int                result;

	Info02 = (void*) GYID (Id);

	switch  (etype)  {

		case    BOT:

			zz_temp_event_id = BOT;
			apevn = NONE;
#if     ZZ_ASR
			appid = (int) GYID (Id);
#endif
			for (Info01 = NULL, result = 0, a = Lnk->Alive;
				a != NULL; a = a -> next) {

				if ((Lnk->Type >= LT_unidirectional) &&
					(a->LRId > LRId)) continue;

				if ((a->Type != JAM) &&
					(a->STime + DV [a->LRId] == Time)) {
					result++;
					if (Info01 == NULL)
						Info01 = (void*) (&(a->Pkt));
				}
			}

			return (result);

		case    EOT:

			zz_temp_event_id = EOT;
			apevn = NONE;
#if     ZZ_ASR
			appid = (int) GYID (Id);
#endif
			for (Info01 = NULL, result = 0, a = Lnk->Alive;
				a != NULL; a = a -> next) {

				if ((Lnk->Type >= LT_unidirectional) &&
					(a->LRId > LRId)) continue;

				if ((a->Type == TRANSMISSION) &&
				    def (a->FTime) &&
					(a->FTime + DV [a->LRId] == Time)) {
					result++;
					if (Info01 == NULL)
						Info01 = (void*) (&(a->Pkt));
				}
			}

			return (result);

		case    BOJ:

			for (result = 0, a = Lnk->Alive; a != NULL;
				a = a -> next) {

				if ((Lnk->Type >= LT_unidirectional) &&
					(a->LRId > LRId)) continue;

				if ((a->Type == JAM) &&
					(a->STime + DV [a->LRId] == Time))
						result++;
			}
			Info01 = NULL;
			return (result);

		case    EOJ:

			for (result = 0, a = Lnk->Alive; a != NULL;
				a = a -> next) {

				if ((Lnk->Type >= LT_unidirectional) &&
					(a->LRId > LRId)) continue;

				if ((a->Type == JAM) && def (a->FTime) &&
					(a->FTime + DV [a->LRId] == Time))
						result++;
			}
			Info01 = NULL;
			return (result);

		default: excptn ("Port->events: illegal event type %1d", etype);
	        return 0;
	}
}

Packet  *Port::anotherPacket () {

/* ---------------------------------------------------------------------- */
/* Traverses all packets that simultaneously trigger  BOT, BMP, EOT, EMP. */
/* Each  subsequent call returns the next packet which is  also stored in */
/* ThePacket (Info01).   Value NULL means that there are no more packets. */
/* The function can be called  after receiving any of the above-mentioned */
/* events, and also after 'events' and 'activities'.  After 'activities', */
/* the function returns  (in its subsequent calls)  all  currently  heard */
/* packets.                                                               */
/* ---------------------------------------------------------------------- */

	if (zz_npre != apevn) {
		// The first time around - initialize things
#if     ZZ_ASR
		if (apevn == NONE)
			// Port id already known
			assert (appid == GYID (Id),
			"Port->anotherPacket: %s, called in an illegal context",
				getSName ());
		else
			appid = (int) GYID (Id);
#endif
		apca = Lnk->Alive;
		apevn = zz_npre;
	}

	// A subsequent call

	assert (appid == GYID (Id),
		"Port->anotherPacket: %s, called in an illegal context",
			getSName ());

	for ( ; apca != NULL; apca = apca -> next) {

		if ((Lnk->Type >= LT_unidirectional) &&
			(apca->LRId > LRId)) continue;

		if (apca -> Type == JAM) continue;

		switch (zz_temp_event_id) {

			case    BOT:

				if (apca->STime + DV [apca->LRId] != Time)
					continue;
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (apca->Pkt.Flags, PF_hdamaged))
    continue;
#endif
				Info01 = (void*) (& (apca->Pkt));
				apca = apca -> next;
				return (ThePacket);

			case    EOT:

				if (undef (apca->FTime) ||
					(apca->FTime + DV [apca->LRId] != Time))
						continue;
#if ZZ_FLK
  if (Lnk->FType == FT_LEVEL2 && flagSet (apca->Pkt.Flags, PF_damaged))
    continue;
#endif
				Info01 = (void*) (& (apca->Pkt));
				apca = apca -> next;
				return (ThePacket);

			case    BMP:

				if (apca->STime + DV [apca->LRId] != Time)
					continue;
				if (!((apca->Pkt).isMy ())) continue;
				Info01 = (void*) (& (apca->Pkt));
				apca = apca -> next;
				return (ThePacket);

			case    EMP:

				if (undef (apca->FTime) ||
					(apca->FTime + DV [apca->LRId] != Time))
						continue;
#if ZZ_FLK
  if (Lnk->FType >= FT_LEVEL1 && flagSet (apca->Pkt.Flags, PF_damaged))
    continue;
#endif
				if (!((apca->Pkt).isMy ())) continue;
				Info01 = (void*) (& (apca->Pkt));
				apca = apca -> next;
				return (ThePacket);

			case    NONE:   // All packets currently heard

				if (apca->STime + DV [apca->LRId] > Time)
					continue;
				if (def (apca->FTime) &&
					(apca->FTime + DV [apca->LRId]) <=
						Time) continue;

				Info01 = (void*) (& (apca->Pkt));
				apca = apca -> next;
				return (ThePacket);

			default:
				excptn ("Port->anotherPacket: illegal call for "
					"%s", getSName ());
		}
	}

	Info01 = NULL;
	return (ThePacket);
}

sexposure (Port)

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr);         // Request queue

		sexmode (1)

			exPrint1 (Hdr);         // Activities

		sexmode (2)

			exPrint2 (Hdr);         // Predicted events
	}

	sonscreen {

		sfxmode (0)

			exDisplay0 ();          // Request queue

		sexmode (1)

			exDisplay1 ();          // Activities

		sexmode (2)

			exDisplay2 ();          // Predicted events
	}
	USESID;
}

void    Port::exPrint0 (const char *hdr) {

/* ------------------------ */
/* Prints the request queue */
/* ------------------------ */

	ZZ_REQUEST                      *r;
	ZZ_EVENT                        *e;
	Long                            l;
	Station                         *s;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Full AI wait list:\n\n";
	}

	Ouf << "           Time ";

	Ouf << "    Process/Idn     PState      Event      State\n";

	s = idToStation (GSID (Id)); 		// Station owning the port

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (e->station != s || (r = e->chain) == NULL) continue;

		while (1) {

			if (r->ai == this) {    // Request to this port

				ptime (r->when, 15);

				if (pless (e->waketime, r->when))
					Ouf << ' ';     // Obsolete
				else if (e->chain == r)
					Ouf << '*';     // Current
				else
					Ouf << '?';

				print (e->process->getTName (), 10);
				Ouf << form ("/%03d ",
						zz_trunc (e->process->Id, 3));
				print (e->process->zz_sn (r->pstate), 10);

				Ouf << ' ';
				print (r->ai->zz_eid (r->event_id), 10);

				Ouf << ' ';

				print (e->process->zz_sn (e->chain->pstate),10);
				Ouf << '\n';
			}

			if ((r = r->other) == e->chain) break;
		}
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Port::exPrint1 (const char *hdr) {

/* ----------------------------- */
/* Prints the list of activities */
/* ----------------------------- */

	int                             l, i;
	LONG   acs [ACSSIZE * (BIG_size + 1)];
	ZZ_LINK_ACTIVITY                *a;
	TIME                            ct;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") List of activities:\n\n";
	}

	for (a = Lnk->Archived, l = 0; a != NULL; a = a -> next) {

		if (Lnk->Type >= LT_unidirectional && a->LRId > LRId) continue;

		if (l >= ACSSIZE) excptn ("Port->exposure: stack overflow");

		((struct activity_descr_struct*)acs) [l].ba = a;
		((struct activity_descr_struct*)acs) [l++].t_s =
			a->STime + DV [a->LRId];
	}

	for (a = Lnk->Alive; a != NULL; a = a -> next) {

		if (Lnk->Type >= LT_unidirectional && a->LRId > LRId) continue;

		if (l >= ACSSIZE) excptn ("Port->exposure: stack overflow");

		((struct activity_descr_struct*)acs) [l].ba = a;
		((struct activity_descr_struct*)acs) [l++].t_s =
			a->STime + DV [a->LRId];
	}

	if (l > 1) {

		sort_acs_p (((struct activity_descr_struct*)acs), 0, l-1);
#ifdef  VERIFY_SORTS
		assert (sorted_acs_p (((struct activity_descr_struct*)acs), l),
			"Port->exposure: internal error -- acs not sorted");
#endif
	}

	Ouf << "T           STime           FTime   St Port  Rcv   TP" <<
#if ZZ_DBG
		"     Length  Signature\n";
#else
		"     Length\n";
#endif

	for (i = 0, ct = collisionTime (YES); i < l; i++) {

		a = (((struct activity_descr_struct*)acs) [i].ba);

		if (((struct activity_descr_struct*)acs) [i].t_s >= ct) {
			Ouf << 'C';
			Ouf << ' ';
			print (ct, 15);
			Ouf <<
#if ZZ_DBG
	" --------------- ---- ---- ---- ---- ----------\n";
#else
	" --------------- ---- ---- ---- ---- ---------- ----------\n";
#endif
			ct = TIME_inf;
		}

		if (a->Type != JAM)
			Ouf << 'T';
		else
			Ouf << 'J';
		Ouf << ' ';

		print (((struct activity_descr_struct*)acs) [i].t_s, 15);
		Ouf << ' ';
		if (def (a->FTime))
			print (a->FTime + DV [a->LRId], 15);
		else
			print ("undefined", 15);

		Ouf << ' ';
		print (zz_trunc (GSID (a->GPort->Id), 4), 4);
		Ouf << ' ';
		print (zz_trunc (GYID (a->GPort->Id), 4), 4);
		Ouf << ' ';
		if (a->Type != JAM) {
			if (isStationId (a->Pkt.Receiver))
				print (zz_trunc (a->Pkt.Receiver, 4), 4);
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

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Port::exPrint2 (const char *hdr) {

/* ----------------------------------- */
/* Prints the list of predicted events */
/* ----------------------------------- */

	ZZ_LINK_ACTIVITY                *a;
	TIME                            ct;

	if (hdr != NULL) {
		Ouf << hdr;
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Timing of events";
		Ouf << ':';
	}

	Ouf << "\n\n";

#if ZZ_DBG
	Ouf <<
 "     Event            When T   St Port Rcvr   TP     Length  Signature\n";
#else
	Ouf << "     Event            When T   St Port Rcvr   TP     Length\n";
#endif

	print ("ACTIVITY", 10); Ouf << ' ';
	if (def (ct = activityTime ())) {
		if (ct <= Time) print ("Now", 15); else print (ct, 15);
		if (tact != NULL) {
			if (tact->Type != JAM)
				Ouf << " T ";
			else
				Ouf << " J ";
			print (zz_trunc (GSID (tact->GPort->Id), 4), 4);
			print (zz_trunc (GYID (tact->GPort->Id), 4), 5);

			Ouf << ' ';
			if (tpckt != NULL) {
				if (isStationId (tpckt->Receiver))
					print (zz_trunc (tpckt->Receiver, 4),
						4);
				else if (tpckt->Receiver == NONE)
					print ("none", 4);
				else
					print ("bcst", 4);
				print (zz_trunc (tpckt->TP, 4), 5);
				print (zz_trunc (tpckt->TLength, 10), 11);
#if ZZ_DBG
				print (zz_trunc (tpckt->Signature, 10), 11);
			} else
				Ouf << "---- ---- ---------- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ---------- ----------";
	} else
	 Ouf << "--------------- - ---- ---- ---- ---- ---------- ----------";
#else
			} else
				Ouf << "---- ---- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ----------";
	} else
		Ouf << "--------------- - ---- ---- ---- ---- ----------";
#endif
	Ouf << '\n';

	print ("SILENCE", 10); Ouf << ' ';
	if (def (ct = silenceTime ())) {
		if (ct <= Time) print ("Now", 15); else print (ct, 15);
		// Find the first activity that ends at 'ct'
		for (a = Lnk->Archived; a != NULL; a = a->next) {
			if (Lnk->Type >= LT_unidirectional &&
						a->LRId > LRId) continue;
			if (a->FTime + DV [a->LRId] == ct) break;
		}
		if (a == NULL) {
			for (a = Lnk->Alive; a != NULL; a = a->next) {
				if (Lnk->Type >= LT_unidirectional &&
						a->LRId > LRId) continue;
				if (def (a->FTime) && a->FTime + DV [a->LRId]
					== ct) break;
			}
		}

		if (a != NULL) {
			if (a->Type != JAM)
				Ouf << " T ";
			else
				Ouf << " J ";
			print (zz_trunc (GSID (a->GPort->Id), 4), 4);
			print (zz_trunc (GYID (a->GPort->Id), 4), 5);

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
			} else
				Ouf << "---- ---- ---------- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ---------- ----------";
	} else
	 Ouf << "--------------- - ---- ---- ---- ---- ---------- ----------";
#else
			} else
				Ouf << "---- ---- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ----------";
	} else
		Ouf << "--------------- - ---- ---- ---- ---- ----------";
#endif
	Ouf << '\n';

	print ("BOT", 10); Ouf << ' ';
	if (def (ct = botTime ())) {
		if (ct == Time) print ("Now", 15); else print (ct, 15);
		if (tact != NULL) {
			if (tact->Type != JAM)
				Ouf << " T ";
			else
				Ouf << " J ";
			print (zz_trunc (GSID (tact->GPort->Id), 4), 4);
			print (zz_trunc (GYID (tact->GPort->Id), 4), 5);

			Ouf << ' ';
			if (tpckt != NULL) {
				if (isStationId (tpckt->Receiver))
					print (zz_trunc (tpckt->Receiver, 4),
						4);
				else if (tpckt->Receiver == NONE)
					print ("none", 4);
				else
					print ("bcst", 4);
				print (zz_trunc (tpckt->TP, 4), 5);
				print (zz_trunc (tpckt->TLength, 10), 11);
#if ZZ_DBG
				print (zz_trunc (tpckt->Signature, 10), 11);
			} else
				Ouf << "---- ---- ---------- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ---------- ----------";
	} else
	 Ouf << "--------------- - ---- ---- ---- ---- ---------- ----------";
#else
			} else
				Ouf << "---- ---- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ----------";
	} else
		Ouf << "--------------- - ---- ---- ---- ---- ----------";
#endif
	Ouf << '\n';

	print ("EOT", 10); Ouf << ' ';
	if (def (ct = eotTime ())) {
		if (ct == Time) print ("Now", 15); else print (ct, 15);
		if (tact != NULL) {
			if (tact->Type != JAM)
				Ouf << " T ";
			else
				Ouf << " J ";
			print (zz_trunc (GSID (tact->GPort->Id), 4), 4);
			print (zz_trunc (GYID (tact->GPort->Id), 4), 5);

			Ouf << ' ';
			if (tpckt != NULL) {
				if (isStationId (tpckt->Receiver))
					print (zz_trunc (tpckt->Receiver, 4),
						4);
				else if (tpckt->Receiver == NONE)
					print ("none", 4);
				else
					print ("bcst", 4);
				print (zz_trunc (tpckt->TP, 4), 5);
				print (zz_trunc (tpckt->TLength, 10), 11);
#if ZZ_DBG
				print (zz_trunc (tpckt->Signature, 10), 11);
			} else
				Ouf << "---- ---- ---------- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ---------- ----------";
	} else
	 Ouf << "--------------- - ---- ---- ---- ---- ---------- ----------";
#else
			} else
				Ouf << "---- ---- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ----------";
	} else
		Ouf << "--------------- - ---- ---- ---- ---- ----------";
#endif
	Ouf << '\n';

	print ("BMP", 10); Ouf << ' ';
	if (def (ct = bmpTime ())) {
		if (ct == Time) print ("Now", 15); else print (ct, 15);
		if (tact != NULL) {
			if (tact->Type != JAM)
				Ouf << " T ";
			else
				Ouf << " J ";
			print (zz_trunc (GSID (tact->GPort->Id), 4), 4);
			print (zz_trunc (GYID (tact->GPort->Id), 4), 5);

			Ouf << ' ';
			if (tpckt != NULL) {
				if (isStationId (tpckt->Receiver))
					print (zz_trunc (tpckt->Receiver, 4),
						4);
				else if (tpckt->Receiver == NONE)
					print ("none", 4);
				else
					print ("bcst", 4);
				print (zz_trunc (tpckt->TP, 4), 5);
				print (zz_trunc (tpckt->TLength, 10), 11);
#if ZZ_DBG
				print (zz_trunc (tpckt->Signature, 10), 11);
			} else
				Ouf << "---- ---- ---------- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ---------- ----------";
	} else
	 Ouf << "--------------- - ---- ---- ---- ---- ---------- ----------";
#else
			} else
				Ouf << "---- ---- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ----------";
	} else
		Ouf << "--------------- - ---- ---- ---- ---- ----------";
#endif
	Ouf << '\n';

	print ("EMP", 10); Ouf << ' ';
	if (def (ct = empTime ())) {
		if (ct == Time) print ("Now", 15); else print (ct, 15);
		if (tact != NULL) {
			if (tact->Type != JAM)
				Ouf << " T ";
			else
				Ouf << " J ";
			print (zz_trunc (GSID (tact->GPort->Id), 4), 4);
			print (zz_trunc (GYID (tact->GPort->Id), 4), 5);

			Ouf << ' ';
			if (tpckt != NULL) {
				if (isStationId (tpckt->Receiver))
					print (zz_trunc (tpckt->Receiver, 4),
						4);
				else if (tpckt->Receiver == NONE)
					print ("none", 4);
				else
					print ("bcst", 4);
				print (zz_trunc (tpckt->TP, 4), 5);
				print (zz_trunc (tpckt->TLength, 10), 11);
#if ZZ_DBG
				print (zz_trunc (tpckt->Signature, 10), 11);
			} else
				Ouf << "---- ---- ---------- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ---------- ----------";
	} else
	 Ouf << "--------------- - ---- ---- ---- ---- ---------- ----------";
#else
			} else
				Ouf << "---- ---- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ----------";
	} else
		Ouf << "--------------- - ---- ---- ---- ---- ----------";
#endif
	Ouf << '\n';

	print ("BOJ", 10); Ouf << ' ';
	if (def (ct = bojTime ())) {
		if (ct == Time) print ("Now", 15); else print (ct, 15);
		if (tact != NULL) {
			Ouf << " J ";
			print (zz_trunc (GSID (tact->GPort->Id), 4), 4);
			print (zz_trunc (GYID (tact->GPort->Id), 4), 5);

			Ouf << ' ';
#if ZZ_DBG
			Ouf << "---- ---- ---------- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ---------- ----------";
	} else
	 Ouf << "--------------- - ---- ---- ---- ---- ---------- ----------";
#else
			Ouf << "---- ---- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ----------";
	} else
		Ouf << "--------------- - ---- ---- ---- ---- ----------";
#endif
	Ouf << '\n';

	print ("EOJ", 10); Ouf << ' ';
	if (def (ct = eojTime ())) {
		if (ct == Time) print ("Now", 15); else print (ct, 15);
		// Find the first jam that ends at 'ct'
		for (a = Lnk->Archived; a != NULL; a = a->next) {
			if (Lnk->Type >= LT_unidirectional &&
						a->LRId > LRId) continue;
			if (a->Type != JAM) continue;
			if (a->FTime + DV [a->LRId] == ct) break;
		}
		if (a == NULL) {
			for (a = Lnk->Alive; a != NULL; a = a->next) {
				if (Lnk->Type >= LT_unidirectional &&
						a->LRId > LRId) continue;
				if (a->Type != JAM) continue;
				if (def (a->FTime) && a->FTime + DV [a->LRId]
					== ct) break;
			}
		}

		if (a != NULL) {
			Ouf << " J ";
			print (zz_trunc (GSID (a->GPort->Id), 4), 4);
			print (zz_trunc (GYID (a->GPort->Id), 4), 5);

			Ouf << ' ';
#if ZZ_DBG
			Ouf << "---- ---- ---------- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ---------- ----------";
	} else
	 Ouf << "--------------- - ---- ---- ---- ---- ---------- ----------";
#else
			Ouf << "---- ---- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ----------";
	} else
		Ouf << "--------------- - ---- ---- ---- ---- ----------";
#endif
	Ouf << '\n';

	print ("COLLISION", 10); Ouf << ' ';
	if (def (ct = collisionTime (YES))) {
		if (ct == Time) print ("Now", 15); else print (ct, 15);
		// Find the first activity that starts at 'ct'
		for (a = Lnk->Archived; a != NULL; a = a->next) {
			if (Lnk->Type >= LT_unidirectional &&
						a->LRId > LRId) continue;
			if (a->STime + DV [a->LRId] == ct) break;
		}
		if (a == NULL) {
			for (a = Lnk->Alive; a != NULL; a = a->next) {
				if (Lnk->Type >= LT_unidirectional &&
						a->LRId > LRId) continue;
				if (a->STime + DV [a->LRId] == ct) break;
			}
		}

		if (a != NULL) {
			if (a->Type != JAM)
				Ouf << " T ";
			else
				Ouf << " J ";
			print (zz_trunc (GSID (a->GPort->Id), 4), 4);
			print (zz_trunc (GYID (a->GPort->Id), 4), 5);

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
			} else
				Ouf << "---- ---- ---------- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ---------- ----------";
	} else
	 Ouf << "--------------- - ---- ---- ---- ---- ---------- ----------";
#else
			} else
				Ouf << "---- ---- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ----------";
	} else
		Ouf << "--------------- - ---- ---- ---- ---- ----------";
#endif
	Ouf << '\n';

	print ("ANYEVENT", 10); Ouf << ' ';
	if (def (ct = aevTime ())) {
		if (ct == Time) print ("Now", 15); else print (ct, 15);
		if (tact != NULL) {
			if (tact->Type != JAM)
				Ouf << " T ";
			else
				Ouf << " J ";
			print (zz_trunc (GSID (tact->GPort->Id), 4), 4);
			print (zz_trunc (GYID (tact->GPort->Id), 4), 5);

			Ouf << ' ';
			if (tpckt != NULL) {
				if (isStationId (tpckt->Receiver))
					print (zz_trunc (tpckt->Receiver, 4),
						4);
				else if (tpckt->Receiver == NONE)
					print ("none", 4);
				else
					print ("bcst", 4);
				print (zz_trunc (tpckt->TP, 4), 5);
				print (zz_trunc (tpckt->TLength, 10), 11);
#if ZZ_DBG
				print (zz_trunc (tpckt->Signature, 10), 11);
			} else
				Ouf << "---- ---- ---------- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ---------- ----------";
	} else
	 Ouf << "--------------- - ---- ---- ---- ---- ---------- ----------";
#else
			} else
				Ouf << "---- ---- ----------";
		} else
			Ouf << " - ---- ---- ---- ---- ----------";
	} else
		Ouf << "--------------- - ---- ---- ---- ---- ----------";
#endif
	Ouf << '\n';

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Port::exDisplay0 () {

/* ------------------------- */
/* Display the request queue */
/* ------------------------- */

	ZZ_REQUEST                      *r;
	ZZ_EVENT                        *e;
	Long                            l;
	Station                         *s;

	s = idToStation (GSID (Id)); 		// Station owning the port

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (e->station != s || (r = e->chain) == NULL) continue;

		while (1) {

			if (r->ai == this) {    // Request to this port

				dtime (r->when);

				if (pless (e->waketime, r->when))
					display (' ');
				else if (e->chain == r)
					display ('*');
				else
					display ('?');

				display (e->process->getTName ());
				display (e->process->Id);
				display (e->process->zz_sn (r->pstate));
				display (r->ai->zz_eid (r->event_id));
				display (e->process->zz_sn (e->chain->pstate));
			}
			if ((r = r->other) == e->chain) break;
		}
	}
}

void    Port::exDisplay1 () {

/* ------------------------------ */
/* Display the list of activities */
/* ------------------------------ */

	int                             l, i;
	LONG   acs [ACSSIZE * (BIG_size + 1)];
	ZZ_LINK_ACTIVITY                *a;
	TIME                            ct;

	for (a = Lnk->Archived, l = 0; a != NULL; a = a -> next) {

		if (Lnk->Type >= LT_unidirectional && a->LRId > LRId) continue;

		if (l >= ACSSIZE) {     // Stack overflow
			display (' ');
			return;
		}

		((struct activity_descr_struct*)acs) [l].ba = a;
		((struct activity_descr_struct*)acs) [l++].t_s =
			a->STime + DV [a->LRId];
	}

	for (a = Lnk->Alive; a != NULL; a = a -> next) {

		if (Lnk->Type >= LT_unidirectional && a->LRId > LRId) continue;

		if (l >= ACSSIZE) {     // Stack overflow
			display (' ');
			return;
		}

		((struct activity_descr_struct*)acs) [l].ba = a;
		((struct activity_descr_struct*)acs) [l++].t_s =
			a->STime + DV [a->LRId];
	}

	if (l > 1) sort_acs_p (((struct activity_descr_struct*)acs), 0, l-1);

	for (i = 0, ct = collisionTime (YES); i < l; i++) {

		a = (((struct activity_descr_struct*)acs) [i].ba);

		if (((struct activity_descr_struct*)acs) [i].t_s >= ct) {
			display ('C');
			display (ct);
			display ("???");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			ct = TIME_inf;
		}

		if (a->Type != JAM)
			display ('T');
		else
			display ('J');

		display (((struct activity_descr_struct*)acs) [i].t_s);

		if (def (a->FTime))
			display (a->FTime + DV [a->LRId]);
		else
			display ("undefined");

		display (GSID (a->GPort->Id));
		display (GYID (a->GPort->Id));

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

void    Port::exDisplay2 () {

/* ------------------------------------ */
/* Display the list of predicted events */
/* ------------------------------------ */

	ZZ_LINK_ACTIVITY                *a;
	TIME                            ct;

	display ("ACTIVITY");
	if (def (ct = activityTime ())) {
		if (ct <= Time) display ("Now"); else display (ct);
		if (tact != NULL) {
			if (tact->Type != JAM)
				display ('T');
			else
				display ('J');

			display (GSID (tact->GPort->Id));
			display (GYID (tact->GPort->Id));

			if (tpckt != NULL) {
				if (isStationId (tpckt->Receiver))
					display (tpckt->Receiver);
				else if (tpckt->Receiver == NONE)
					display ("none");
				else
					display ("bcst");

				display (tpckt->TP);
				display (tpckt->TLength);
#if ZZ_DBG
				display (tpckt->Signature);
#else
				display ("---");
#endif
			} else {
				display ("---");
				display ("---");
				display ("---");
				display ("---");
			}
		} else {
			display ('-');
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
		}
	} else {
		display ("---");
		display ('-');
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
	}

	display ("SILENCE");
	if (def (ct = silenceTime ())) {
		if (ct <= Time) display ("Now"); else display (ct);
		// Find the first activity that ends at 'ct'
		for (a = Lnk->Archived; a != NULL; a = a->next) {
			if (Lnk->Type >= LT_unidirectional &&
						a->LRId > LRId) continue;
			if (a->FTime + DV [a->LRId] == ct) break;
		}
		if (a == NULL) {
			for (a = Lnk->Alive; a != NULL; a = a->next) {
				if (Lnk->Type >= LT_unidirectional &&
						a->LRId > LRId) continue;
				if (def (a->FTime) && a->FTime + DV [a->LRId]
					== ct) break;
			}
		}

		if (a != NULL) {
			if (a->Type != JAM)
				display ('T');
			else
				display ('J');
			display (GSID (a->GPort->Id));
			display (GYID (a->GPort->Id));

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
		} else {
			display ('-');
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
		}
	} else {
		display ("---");
		display ('-');
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
	}

	display ("BOT");
	if (def (ct = botTime ())) {
		if (ct == Time) display ("Now"); else display (ct);
		if (tact != NULL) {
			if (tact->Type != JAM)
				display ('T');
			else
				display ('J');
			display (GSID (tact->GPort->Id));
			display (GYID (tact->GPort->Id));

			if (tact->Type != JAM) {
				if (isStationId (tact->Pkt.Receiver))
					display (tact->Pkt.Receiver);
				else if (tact->Pkt.Receiver == NONE)
					display ("none");
				else
					display ("bcst");

				display (tact->Pkt.TP);
				display (tact->Pkt.TLength);
#if ZZ_DBG
				display (tact->Pkt.TLength);
#else
				display ("---");
#endif
			} else {
				display ("---");
				display ("---");
				display ("---");
				display ("---");
			}
		} else {
			display ('-');
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
		}
	} else {
		display ("---");
		display ('-');
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
	}

	display ("EOT");
	if (def (ct = eotTime ())) {
		if (ct == Time) display ("Now"); else display (ct);
		if (tact != NULL) {
			if (tact->Type != JAM)
				display ('T');
			else
				display ('J');
			display (GSID (tact->GPort->Id));
			display (GYID (tact->GPort->Id));

			if (tact->Type != JAM) {
				if (isStationId (tact->Pkt.Receiver))
					display (tact->Pkt.Receiver);
				else if (tact->Pkt.Receiver == NONE)
					display ("none");
				else
					display ("bcst");

				display (tact->Pkt.TP);
				display (tact->Pkt.TLength);
#if ZZ_DBG
				display (tact->Pkt.Signature);
#else
				display ("---");
#endif
			} else {
				display ("---");
				display ("---");
				display ("---");
				display ("---");
			}
		} else {
			display ('-');
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
		}
	} else {
		display ("---");
		display ('-');
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
	}

	display ("BMP");
	if (def (ct = bmpTime ())) {
		if (ct == Time) display ("Now"); else display (ct);
		if (tact != NULL) {
			if (tact->Type != JAM)
				display ('T');
			else
				display ('J');
			display (GSID (tact->GPort->Id));
			display (GYID (tact->GPort->Id));

			if (tact->Type != JAM) {
				if (isStationId (tact->Pkt.Receiver))
					display (tact->Pkt.Receiver);
				else if (tact->Pkt.Receiver == NONE)
					display ("none");
				else
					display ("bcst");

				display (tact->Pkt.TP);
				display (tact->Pkt.TLength);
#if ZZ_DBG
				display (tact->Pkt.Signature);
#else
				display ("---");
#endif
			} else {
				display ("---");
				display ("---");
				display ("---");
				display ("---");
			}
		} else {
			display ('-');
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
		}
	} else {
		display ("---");
		display ('-');
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
	}

	display ("EMP");
	if (def (ct = empTime ())) {
		if (ct == Time) display ("Now"); else display (ct);
		if (tact != NULL) {
			if (tact->Type != JAM)
				display ('T');
			else
				display ('J');
			display (GSID (tact->GPort->Id));
			display (GYID (tact->GPort->Id));

			if (tact->Type != JAM) {
				if (isStationId (tact->Pkt.Receiver))
					display (tact->Pkt.Receiver);
				else if (tact->Pkt.Receiver == NONE)
					display ("none");
				else
					display ("bcst");

				display (tact->Pkt.TP);
				display (tact->Pkt.TLength);
#if ZZ_DBG
				display (tact->Pkt.Signature);
#else
				display ("---");
#endif
			} else {
				display ("---");
				display ("---");
				display ("---");
				display ("---");
			}
		} else {
			display ('-');
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
		}
	} else {
		display ("---");
		display ('-');
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
	}

	display ("BOJ");
	if (def (ct = bojTime ())) {
		if (ct == Time) display ("Now"); else display (ct);
		if (tact != NULL) {
			display ('J');
			display (GSID (tact->GPort->Id));
			display (GYID (tact->GPort->Id));
			display ("---");
			display ("---");
			display ("---");
			display ("---");
		} else {
			display ('-');
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
		}
	} else {
		display ("---");
		display ('-');
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
	}

	display ("EOJ");
	if (def (ct = eojTime ())) {
		if (ct == Time) display ("Now"); else display (ct);
		// Find the first jam that ends at 'ct'
		for (a = Lnk->Archived; a != NULL; a = a->next) {
			if (Lnk->Type >= LT_unidirectional &&
						a->LRId > LRId) continue;
			if (a->Type != JAM) continue;
			if (a->FTime + DV [a->LRId] == ct) break;
		}
		if (a == NULL) {
			for (a = Lnk->Alive; a != NULL; a = a->next) {
				if (Lnk->Type >= LT_unidirectional &&
						a->LRId > LRId) continue;
				if (a->Type != JAM) continue;
				if (def (a->FTime) && a->FTime + DV [a->LRId]
					== ct) break;
			}
		}

		if (a != NULL) {
			display ('J');
			display (GSID (a->GPort->Id));
			display (GYID (a->GPort->Id));
			display ("---");
			display ("---");
			display ("---");
			display ("---");
		} else {
			display ('-');
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
		}
	} else {
		display ("---");
		display ('-');
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
	}

	display ("COLLISION");
	if (def (ct = collisionTime (YES))) {
		if (ct == Time) display ("Now"); else display (ct);
		// Find the first activity that starts at 'ct'
		for (a = Lnk->Archived; a != NULL; a = a->next) {
			if (Lnk->Type >= LT_unidirectional &&
						a->LRId > LRId) continue;
			if (a->STime + DV [a->LRId] == ct) break;
		}
		if (a == NULL) {
			for (a = Lnk->Alive; a != NULL; a = a->next) {
				if (Lnk->Type >= LT_unidirectional &&
						a->LRId > LRId) continue;
				if (a->STime + DV [a->LRId] == ct) break;
			}
		}
		if (a != NULL) {
			if (a->Type != JAM)
				display ('T');
			else
				display ('J');
			display (GSID (a->GPort->Id));
			display (GYID (a->GPort->Id));

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
		} else {
			display ('-');
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
		}
	} else {
		display ("---");
		display ('-');
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
	}

	display ("ANYEVENT");
	if (def (ct = aevTime ())) {
		if (ct == Time) display ("Now"); else display (ct);
		if (tact != NULL) {
			if (tact->Type != JAM)
				display ('T');
			else
				display ('J');
			display (GSID (tact->GPort->Id));
			display (GYID (tact->GPort->Id));

			if (tact->Type != JAM) {
				if (isStationId (tact->Pkt.Receiver))
					display (tact->Pkt.Receiver);
				else if (tact->Pkt.Receiver == NONE)
					display ("none");
				else
					display ("bcst");

				display (tact->Pkt.TP);
				display (tact->Pkt.TLength);
#if ZZ_DBG
				display (tact->Pkt.Signature);
#else
				display ("---");
#endif
			} else {
				display ("---");
				display ("---");
				display ("---");
				display ("---");
			}
		} else {
			display ('-');
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
			display ("---");
		}
	} else {
		display ("---");
		display ('-');
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
		display ("---");
	}
}

sexposure (Link)

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr, (int) SId);    // Request queue

		sexmode (1)

			exPrint1 (Hdr);               // Performance

		sexmode (2)

			exPrint2 (Hdr, (int) SId);    // Activities
	}

	sonscreen {

		sfxmode (0)

			exDisplay0 ((int) SId);       // Request queue

		sexmode (1)

			exDisplay1 ();                // Performance

		sexmode (2)

			exDisplay2 ((int) SId);       // Activities
	}
}

void    Link::exPrint0 (const char *hdr, int sid) {

/* ----------------------- */
/* Print the request queue */
/* ----------------------- */

	ZZ_REQUEST                      *r;
	ZZ_EVENT                        *e;
	Long                            l;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Full AI wait list";
		if (isStationId (sid))
			Ouf << " for " << idToStation (sid)->getOName ();
		Ouf << ":\n\n";
	}

	if (isStationId (sid))
		Ouf << "           Time ";
	else
		Ouf << "       Time   St";

	Ouf << "    Process/Idn     LState      Event         AI/Idn" <<
		"      State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && (e->station == NULL ||
			e->chain == NULL || (e->station)->Id != sid))
				continue;

		if ((r = e->chain) == NULL && e->ai == this) {

			// Internal event
			if (zz_flg_nosysdisp) continue;

			ptime (e->waketime, 11);
			Ouf << "* --- ";
			print (e->process->getTName (), 10);
			Ouf << form ("/%03d ", zz_trunc (e->process->Id, 3));
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

		} else if (r != NULL) {

			while (1) {
				if (r->ai->Class == AIC_port &&
					((Port*)(r->ai))->Lnk == this) {

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

void    Link::exPrint1 (const char *hdr) {

/* ----------------------------------- */
/* Print the link performance measures */
/* ----------------------------------- */

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Performance measures:\n\n";
	}

	print (NTJams         , "    Number of jamming signals:      ");
	print (NTAttempts     , "    Number of transmit attempts:    ");
	print (NTPackets      , "    Number of transmitted packets:  ");
	print (NTBits         , "    Number of transmitted bits:     ");
	print (NRPackets      , "    Number of received packets:     ");
	print (NRBits         , "    Number of received bits:        ");
	print (NTMessages     , "    Number of transmitted messages: ");
	print (NRMessages     , "    Number of received messages:    ");
#if ZZ_FLK
	print (NDPackets      , "    Number of damaged packets:      ");
	print (NHDPackets     , "    Number of h-damaged packets:    ");
	print (NDBits         , "    Number of damaged bits:         ");
#endif
	print (Etu * ((double)NRBits / (Time == TIME_0 ? TIME_1 : Time)),
				"    Throughput (by received bits):  ");
	print (Etu * ((double)NTBits / (Time == TIME_0 ? TIME_1 : Time)),
				"    Throughput (by trnsmtd bits):   ");

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Link::exPrint2 (const char *hdr, int sid) {

/* ---------------------------- */
/* Print the list of activities */
/* ---------------------------- */

	ZZ_LINK_ACTIVITY                *a, *cl;

	if (hdr != NULL) {
		Ouf << hdr;
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") List of activities";
		if (isStationId (sid))
			Ouf << " for " << idToStation (sid) -> getOName ();
		Ouf << ':';
	}

	Ouf << "\n\n";

	Ouf << "T            STime           FTime ";
	if (isStationId (sid))
		Ouf << "     Port";
	else
		Ouf << "  St Port";
#if ZZ_DBG
	Ouf << " Rcvr   TP     Length  Signature\n";
#else
	Ouf << " Rcvr   TP     Length\n";
#endif

	for (a = cl = Archived; ; a = a->next) {
		if (a == NULL) {
			if (cl == Alive) break;         // End of list
			if ((a = cl = Alive) == NULL) break;
		};

		if (isStationId (sid) && GSID (a->GPort->Id) != sid)
			continue;

		if (a->Type == JAM)
			Ouf << 'J';
		else
			Ouf << 'T';
		if (cl == Archived)
			Ouf << "* ";
		else
			Ouf << "  ";
		print (a->STime, 15); Ouf << ' ';
		ptime (a->FTime, 15);
		Ouf << ' ';

		if (!isStationId (sid)) {
			print (zz_trunc (GSID (a->GPort->Id), 4), 4);
			print (zz_trunc (GYID (a->GPort->Id), 4), 5);
		} else
			print (zz_trunc (GYID (a->GPort->Id), 9), 9);
		Ouf << ' ';

		if (a->Type != JAM) {
			if (isStationId (a->Pkt.Receiver))
				print (zz_trunc (a->Pkt.Receiver, 4), 4);
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

	Ouf << '\n';

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Link::exDisplay0 (int sid) {

/* ------------------------- */
/* Display the request queue */
/* ------------------------- */

	ZZ_REQUEST                      *r;
	ZZ_EVENT                        *e;
	Long                            l;

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && (e->station == NULL ||
			e->chain == NULL || (e->station)->Id != sid))
				continue;

		if ((r = e->chain) == NULL && e->ai == this) {

			// Internal event
			if (zz_flg_nosysdisp) continue;

			dtime (e->waketime);
			display ('*');
			display ("---");
			display (e->process->getTName ());
			display (e->process->Id);
			display (e->process->zz_sn (e->pstate));
			display (e->ai->zz_eid (e->event_id));
			display (e->ai->getTName());
			if ((l = e->ai->zz_aid ()) != NONE)
				display (l);
			else
				display (' ');
			display (e->process->zz_sn (e->pstate));

		} else if (r != NULL) {

			while (1) {
				if (r->ai->Class == AIC_port &&
					((Port*)(r->ai))->Lnk == this) {

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

void    Link::exDisplay1 () {

/* ------------------------------------- */
/* Display the link performance measures */
/* ------------------------------------- */

	display (Etu * ((double)NRBits / (Time == TIME_0 ? TIME_1 : Time)));
	display (Etu * ((double)NTBits / (Time == TIME_0 ? TIME_1 : Time)));
	display (NTJams         );
	display (NTAttempts     );
	display (NTPackets      );
	display (NTBits         );
	display (NRPackets      );
	display (NRBits         );
	display (NTMessages     );
	display (NRMessages     );
#if ZZ_FLK
	display (NDPackets      );
	display (NHDPackets     );
	display (NDBits         );
#endif
}

void    Link::exDisplay2 (int sid) {

/* ------------------------------ */
/* Display the list of activities */
/* ------------------------------ */

	ZZ_LINK_ACTIVITY                   *a, *cl;

	for (a = cl = Archived; ; a = a->next) {
		if (a == NULL) {
			if (cl == Alive) break;         // End of list
			if ((a = cl = Alive) == NULL) break;
		};

		if (isStationId (sid) && GSID (a->GPort->Id) != sid)
			continue;

		if (a->Type == JAM)
			display ('J');
		else
			display ('T');
		if (cl == Archived)
			display ('*');
		else
			display (' ');
		display (a->STime);
		if (undef (a->FTime))
			display ("undefined");
		else
			display (a->FTime);

		if (!isStationId (sid))
			display (GSID (a->GPort->Id));

		display (GYID (a->GPort->Id));

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

#endif	/* NOL */
