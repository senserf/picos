/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-06   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

/* -------------------------------------------- */
/* The main program: execution starts from here */
/* -------------------------------------------- */

// #define CDEBUG 1

#include        "system.h"
#include	"cdebug.h"

#define SHF (BIL-1)
#define	SH_1 ((LONG)1)

jmp_buf         zz_mloop;		// Return to the main loop
jmp_buf		zz_waker;		// For out of process sleep
int             zz_setjmp_done = NO;	// Flag == longjmp legal

#if     ZZ_OBS
#if     ZZ_ATT
#define smatch(a,b)     (!strcmp (a, b))
#else
static  inline  int     smatch (register char *s1, register char *s2) {

/* ------------------------- */
/* A local version of strcmp */
/* ------------------------- */

	if (s2 == NULL) return (0);
	while (*s1 == *s2++) if (*s1++ == '\0') return (1);
	return (0);
}
#endif
#endif

#if     ZZ_TOL
void    setTolerance (double t, int q) {

	if ((zz_tolerance = t) == 0.0)
		zz_quality = 0;
	else
		zz_quality = q;
	Assert ((zz_quality >=0) && (zz_quality <= 10) && (t >= 0.0) &&
		(t < 1.0), "setTolerance: illegal clock tolerance parameters: "
			"%f, %1d", t, q);
}

double	getTolerance (int *q) {

	if (q)
		*q = zz_quality;
	return zz_tolerance;
}

#else

void    setTolerance (double, int) {
	excptn ("setTolerance illegal: smurph created with '-n'");
}

double	getTolerance (int *q) {
	excptn ("getTolerance illegal: smurph created with '-n'");
}

#endif

void    setDu (double e) {

#if	(ZZ_NFP || ((ZZ_NOR == 0) && (ZZ_NOL == 0)))
	excptn ("setDu: illegal with no links and no rfchannels");
#else
	if (NStations
#if	ZZ_NOL
            + NLinks
#endif
#if	ZZ_NOR
            + NRFChannels
#endif
#if	ZZ_NOC
            + NTraffics
#endif
              != 0)
		excptn ("setDu: illegal -- some objects have been created");
	Du = e;
#endif	/* Function is illegal */
}

void    setEtu (double e) {
#if	ZZ_NFP
	zz_nfp ("setEtu");
#else
	if (NStations
#if	ZZ_NOL
            + NLinks
#endif
#if	ZZ_NOR
            + NRFChannels
#endif
#if	ZZ_NOC
            + NTraffics
#endif
              != 0)
		excptn ("setEtu: illegal -- some objects have been created");
	Itu = 1.0 / (Etu = e);
#if  ZZ_REA || ZZ_RSY
        ITUPerMSec = 1.0/(MSecPerITU = 1000.0 * Itu);
#if  ZZ_RSY
	Assert (ZZ_ResyncMsec == 0,
		"setEtu: illegal while visualization mode is on");
#endif
#endif	/* REA + RSY */
#endif	/* Function is illegal with NFP */
}

void    setItu (double e) {
#if	ZZ_NFP
	zz_nfp ("setItu");
#else
	if (NStations
#if	ZZ_NOL
            + NLinks
#endif
#if	ZZ_NOC
            + NTraffics
#endif
#if	ZZ_NOR
            + NRFChannels
#endif
              != 0)
		excptn ("setItu: illegal -- some objects have been created");
	Etu = 1.0 / (Itu = e);
#if  ZZ_REA || ZZ_RSY
        ITUPerMSec = 1.0/(MSecPerITU = 1000.0 * Itu);
#if  ZZ_RSY
	Assert (ZZ_ResyncMsec == 0,
		"setItu: illegal while visualization mode is on");
#endif
#endif	/* REA + RSY */
#endif	/* Function is illegal with NFP */
}

#if	ZZ_NFP
void    setLimit (Long nm, TIME mt, Long ct) {
#else
void    setLimit (Long nm, TIME mt, double ct) {
#endif

	Assert (nm >= 0, "setLimit: illegal message number limit %d", nm);
#if	ZZ_NOC
	zz_max_NMessages = (nm == 0) ? MAX_Long : nm;
#endif
	zz_max_Time = (mt == TIME_0) ? TIME_inf : mt;
#if	ZZ_NFP
	Assert (ct >= 0, "setLimit: illegal CPU time limit %1d", ct);
	zz_max_cpuTime = (ct < 1) ? MAX_Long : ct;
#else
	Assert (ct >= 0.0, "setLimit: illegal CPU time limit %f", ct);
	zz_max_cpuTime = (ct < 0.00001) ? HUGE : ct;
#endif
}

void    setLimit (Long nm, TIME mt) {
	setLimit (nm, mt, zz_max_cpuTime);
}

void    setLimit (Long nm) {
	setLimit (nm, zz_max_Time);
}

#if	ZZ_NOC
void    setFlush () {
	zz_flg_emptyMQueues = YES;
}
#endif

void    buildNetwork () {

/* -------------------------- */
/* Forces topology definition */
/* -------------------------- */

	Station *s;
	Process *p;

	if (!zz_flg_started) {

		s = TheStation;
		p = TheProcess;
		TheStation = System;
		TheProcess = Kernel;

#if	ZZ_NOC
		zz_start_client ();
#endif

		// Postprocess the network topolgy definitions by
		// cleaning up temporary data structures and creating
		// port distance vectors.

		System->makeTopology ();

		// Don't do it the next time
		zz_flg_started = YES;

		TheProcess = p;
		TheStation = s;

	        zz_adjust_ownership ();  // Move level-0 processes to stations
	}
}

#if     ZZ_R48 != 1
static void initrnd () {

/* ------------------------------- */
/* Initializes the divisor for rnd */
/* ------------------------------- */
#if	ZZ_NFP
#else
	double  f, g, h, u, p;

	u = f = (double) MAX_Long;
	g = 0.5;

	while (1) {
		h = f + g;
		p = u / h;
		if (p >= 1.0) break;
		zz_rndd = h;
		g = g/2.0;
	}
#endif
}
#endif

#if  ZZ_REA || ZZ_RSY
/* ================== */
/* Real time + resync */
/* ================== */
static struct timeval RealTime;

// This is the time offset for running the program at a different time from
// a journal
static time_t TimeOffset = 0;

static void openSourceJournals () {
  // Opens the feeding journals
  ZZ_Journal *jn;
  for (jn = ZZ_Journals; jn != NULL; jn = jn -> next)
    if (jn->JType != 'J') jn->openFile ();
};

static void openTracingJournals () {
  // Opens the journals used for tracing
  ZZ_Journal *jn;
  for (jn = ZZ_Journals; jn != NULL; jn = jn -> next)
    if (jn->JType == 'J') jn->openFile ();
};

static void setTimeOffset () {
  // Sets the time offset based on the specified time/date
  // The format is:
  //    yy/mm/dd,hh:mm:ss
  //    mm/dd............
  //    dd,..............
  //    hh:..............
  //    and so on
  struct tm bt, *btc;
  struct timeval RT;
  int val;
  Long off;
  char *tss, *ts;
  ZZ_Journal *jn;
  if ((ts = ZZ_OffsetArg) == NULL) return;  // Do nothing
  TimeOffset = 0;
  // Initialize the structure
  off = getEffectiveTimeOfDay ();
  if (strcmp ("J", ts) == 0) {
    // Set it to the earliest time among the journals
    for (jn = ZZ_Journals; jn != NULL; jn = jn -> next)
      if (jn->JType != 'J' && jn->WhenSec < off)
        off = jn->WhenSec;
  } else {
    // The time is specified explicitly
    btc = localtime ((time_t*)(&off));
    bt = *btc;
    bt.tm_isdst = -1;  // Recalculate this for the new date/time

#define isDigit(d)        ((d) <= '9' && (d) >= '0')
#define getTwo            do {\
                            if (isDigit (*ts) && isDigit (*(ts+1)) ) {\
                              val = ((*ts) - '0') * 10 + ((*(ts+1)) - '0');\
                              ts += 2;\
                            } else\
                              val = -1;\
                          } while (0)

    tss = ts;
    while (*ts != '\0') {
      // Process date
      getTwo;
      if (val < 0) {
SD_Fail:
        excptn ("Main: -D argument format error");
      }
      if (*ts == '/') {
        if (*(ts+3) == '/')
          bt.tm_year = (val < 95) ? val + 100 : val;
        else
          bt.tm_mon = val - 1;
      } else if (*ts == ',') {
        bt.tm_mday = val;
      } else if (*ts == ':') {
        if (*(ts+3) == ':')
          bt.tm_hour = val;
        else
          bt.tm_min = val;
      } else if (*ts == '\0') {
        if (ts-3 >= tss && *(ts-3) == '/')
          bt.tm_mday = val;
        else
          bt.tm_sec = val;
      } else
        goto SD_Fail;
      if (*ts != '\0') ts++;
    }
    if (bt.tm_mon  < 0 || bt.tm_mon  > 11 ||
        bt.tm_mday < 1 || bt.tm_mday > 31 ||
        bt.tm_hour < 0 || bt.tm_hour > 23 ||
        bt.tm_min  < 0 || bt.tm_min  > 59 ||
        bt.tm_sec  < 0 || bt.tm_sec  > 59    ) goto SD_Fail;
    if ((off = mktime (&bt)) < 0) goto SD_Fail;
  }
  gettimeofday (&RT, NULL);
  TimeOffset = off - RT.tv_sec;
};

static inline Long msecdiff (struct timeval &early, struct timeval &late) {
  // Returns the difference in milliseconds between late and early
  Long diff;
  if (late.tv_sec > early.tv_sec)
    diff = (late.tv_sec - early.tv_sec) * 1000;
  else
    diff = 0;
  diff += (late.tv_usec - early.tv_usec) / 1000;
  return diff;
};

void zz_wait_for_sockets (Long ms) {

  // This function waits for ms milliseconds for I/O on
  // mailbox sockets and returns when there is any, or that much time
  // has elapsed

  Mailbox *sok;
  int sfd, nws, mwr;

  if (ms == 0) return;

#define	NOSELECT 0

#if ZZ_CYW
#undef NOSELECT
#define	NOSELECT 1
#endif

#if NOSELECT
  // ----------------
  // select is absent
  // ----------------
  for (nws = 0, sok = zz_socket_list; sok != NULL; sok = sok->nexts) {
    // Check if there is a socket with an outstanding request
    if (sok->isWaiting () >= 0) {
      nws++;
      break;
    }
  }
  if (nws || zz_msock_stat (mwr) >= 0) {
    usleep (SOCKCHKINT * 1000);
  } else {
    // Just a straightforward delay without waiting for anything
    while (ms > 1000000) {
      sleep (1000);
      ms -= 1000000;
    }
    usleep (ms * 1000);
  }
#else
  // -----------------
  // select is present
  // -----------------
  fd_set soks, swrite;
  struct timeval delay;

  FD_ZERO (&soks);
  FD_ZERO (&swrite);
  for (nws = 0, sok = zz_socket_list; sok != NULL; sok = sok->nexts) {
    // Check if there is a socket with an outstanding request
    if ((sfd = sok->isWaiting ()) >= 0) {
      FD_SET (sfd, &soks);
      nws++;
    }
  }

  if ((sfd = zz_msock_stat (mwr)) >= 0) {
    // Include the DSD socket in the game
    nws++;
    FD_SET (sfd, &soks);
    if (mwr) FD_SET (sfd, &swrite);
  }

  if (nws) {
    // There are some sockets to wait for
    delay.tv_sec = ms / 1000;
    delay.tv_usec = (ms % 1000) * 1000;
    select (zz_nfdesc, &soks, &swrite, &soks, &delay);
  } else {
    while (ms > 1000000) {
      sleep (1000);
      ms -= 1000000;
    }
    usleep (ms * 1000);
  }
#endif
};

#endif	/* ZZ_REA + ZZ_RSY */

#if  ZZ_REA
/* ------------------- */
/* Real-time functions */
/* ------------------- */

static struct timeval RT_LastEvent, RT_LastDisplay;

Long getEffectiveTimeOfDay (Long *usec) {
  if (usec != NULL) *usec = RealTime.tv_usec;
  return RealTime.tv_sec + TimeOffset;
};

static inline void adjust_RT (Long diff) {
  // Subtracts diff milliseconds from the time of the last event
  diff *= 1000;
  while (diff > RT_LastEvent.tv_usec) {
    RT_LastEvent.tv_sec --;
    RT_LastEvent.tv_usec += 1000000;
    // We cannot be dragging behind more than a few seconds
  }
  RT_LastEvent.tv_usec -= diff;
};

static void mark_real_time () {
  gettimeofday (&RT_LastEvent, NULL);
  RealTime = RT_LastEvent;
  // This function tags TIME 0 with the current time in microseconds
};

inline void zz_advance_real_time () {
/*
 * Referenced only once, so let it be inline
 */

  Mailbox *sok;
  Long NMS, EMS;

  TIME t;

  while (1) {
    zz_check_asyn_io ();
    // Check for display
    if (DisplayActive) {
      gettimeofday (&RealTime, NULL);
      if (msecdiff (RT_LastDisplay, RealTime) >= zz_events_to_display) {
Display:
        RT_LastDisplay = RealTime; 
        refreshDisplay ();
        // Note that refreshDisplay resets zz_events_to_display to the
        // current default after it is done.
      }
    }

    // This may be reset by readDataBlock from ZZ_Journal when it
    // detects that the nearest Journal update event is closer than
    // that.
    zz_max_sleep_time = IOCHECKINT;

    for (sok = zz_socket_list; sok != NULL; sok = sok->nexts) {
      // Update the status of socket buffers
      sok->flushOutput ();
      sok->readInput ();
    }

    // Determine the time of the next event

#if ZZ_TAG
    (zz_eq -> waketime).get (t);
    if (t <= Time) {
#else
    if ((t = zz_eq -> waketime) <= Time) {
#endif
      // Make sure it doesn't flow backwards
      Time = t;
      // Simulated time isn't going to be advanced -- fine
      delete zz_CE;
      // Note: we carry over zz_CE from the previous turn. We need it for
      // display, so the deallocation occurs just before a new allocation
      zz_CE = zz_eq->get ();
      // mark_real_time ();
      return;
    }
  
    // The time is different
  
    if (def (t)) {
      // How many milliseconds apart are we?
#if   ZZ_NFP
      NMS = (t - Time) / MILLISECOND;
#else
      NMS = (Long)(((double) (t - Time)) * MSecPerITU);
#endif
      // Do we have to sleep at all?
      gettimeofday (&RealTime, NULL);
      // How many milliseconds have elapsed since the last time update
      EMS = msecdiff (RT_LastEvent, RealTime);
      if (EMS >= NMS) {
        // No need to wait -- process immediately
        RT_LastEvent = RealTime;
        adjust_RT ((long) (EMS-NMS));  // If we happen to be dragging behind
        delete zz_CE;
        zz_CE = zz_eq->get ();
        return;
      }
      // We have to sleep for some time, but we shouldn't be missing
      // socket events or display intervals

      // We should wait for NMS - EMS
      if ((NMS -= EMS) > zz_max_sleep_time) NMS = zz_max_sleep_time;
    } else {
      // We get here if the time of the next event is unknown
      if (zz_eq == &zz_rsent) zz_done ();
      NMS = zz_max_sleep_time;
    }
    if (DisplayActive) {
      if ((EMS = zz_events_to_display - msecdiff (RT_LastDisplay, RealTime))
        < NMS) {
           if ((NMS = EMS) <= 0) goto Display;
      }
    }
    // cdebugl ("WFS");
    // cdhex (NMS);
    zz_wait_for_sockets (NMS);
    gettimeofday (&RealTime, NULL);
    // Make sure time is advanced
#if ZZ_NFP
    Time += msecdiff (RT_LastEvent, RealTime) * MILLISECOND;
#else
    Time += (long) (msecdiff (RT_LastEvent, RealTime) * ITUPerMSec);
#endif
    RT_LastEvent = RealTime;
  }
};

#endif	/* ZZ_REA */

#if ZZ_RSY
/* =========== */
/* Resync mode */
/* =========== */

Long		ZZ_ResyncMsec = 0;	// Also used as a flag

static TIME	LastRT,		// Virtual time when last RealTime was taken
		NextResync,	// Next Time of resync to Real Time
		ResyncInt;	// Resync interval (in ITUs)

static struct timeval ResyncTime,	// Real time of next resync
		      ResyncDelta;	// Real time resync interval

static inline void add_rtime (struct timeval &res, struct timeval &inc) {
/*
 * Increment res by inc
 */
	res.tv_sec += inc.tv_sec;
	if ((res.tv_usec += inc.tv_usec) >= 1000000) {
		res.tv_sec += (res.tv_usec / 1000000);
		res.tv_usec %= 1000000;
	}
}

static inline void mark_real_time () {

	gettimeofday (&RealTime, NULL);
	LastRT = Time;
};

void setResync (Long msecs, double tm) {
/*
 * Sets the resync interval to the prescribed number of milliseconds and
 * associates that interval with Etu factored by tm
 */
	if (msecs == 0) {
		// This is also the default
		ZZ_ResyncMsec = 0;
		return;
	}

	// I am not sure about this
	Assert (msecs <= 1000,
		"setResync: the number of miliseconds (%1d) must be <= 1000",
			msecs);

	ResyncInt = (TIME)(Etu * tm);
	if (msecs == 1000) {
		ResyncDelta.tv_sec = 1;
		ResyncDelta.tv_usec = 0;
	} else {
		ResyncDelta.tv_sec = 0;
		ResyncDelta.tv_usec = msecs * 1000;
	}
	mark_real_time ();
	NextResync = Time + ResyncInt;
	ResyncTime = RealTime;
	add_rtime (ResyncTime, ResyncDelta);
	ZZ_ResyncMsec = msecs;
}

Long getEffectiveTimeOfDay (Long *usec) {

	Long	UDelta, SDelta;

	// Add microseconds elapsed from last take
	UDelta = RealTime.tv_usec +
		(Long)((Time - LastRT) / (ITUPerMSec * 1000.0));
	if (UDelta >= 1000000) {
		// Adjustment for seconds
		SDelta = UDelta / 1000000;
		if (usec != NULL)
			// Corrected microseconds
			*usec = UDelta % 1000000;
	} else {
		SDelta = 0;
		if (usec != NULL)
			*usec = UDelta;
	}

	// Return seconds
	return RealTime.tv_sec + TimeOffset + SDelta;
};

inline void zz_advance_real_time () {

	TIME t;
  	Mailbox *sok;
	Long	delta;

	if (ZZ_ResyncMsec == 0)
		// This is also used as a flag
		return;
#if ZZ_TAG
	(zz_eq -> waketime).get (t);
#else
	t = zz_eq -> waketime;
#endif
	if (t < NextResync)
		return;

    	// Resync to real time

    	zz_check_asyn_io ();
    	if (DisplayActive)
		// Do not check any further. If we are displaying in this mode,
		// we should be refreshing at sync intervals
	        refreshDisplay ();

	do {
		for (sok = zz_socket_list; sok != NULL; sok = sok->nexts) {
	            // Update the status of socket buffers
	            sok->flushOutput ();
	            sok->readInput ();
	    	}

		// Do it again: the mailboxes may have changed things
#if ZZ_TAG
	    	(zz_eq -> waketime).get (t);
#else
	    	t = zz_eq -> waketime;
#endif
		if (t < NextResync)
			return;

		if (zz_eq == &zz_rsent)
			zz_done ();

		mark_real_time ();

		// Distance to catch up in milliseconds
		delta = msecdiff (RealTime, ResyncTime);

		if (delta > 0) {
			// Have to wait
			zz_wait_for_sockets (delta > zz_max_sleep_time ?
				zz_max_sleep_time : delta);
    			zz_max_sleep_time = IOCHECKINT;
		} else {
			// Update the resync time
			Time = NextResync;
			NextResync += ResyncInt;
			add_rtime (ResyncTime, ResyncDelta);
		}

	} while (1);
}

#endif	/* ZZ_RSY */

#if (ZZ_REA == 0) && (ZZ_RSY == 0)

Long getEffectiveTimeOfDay (Long *usec) {
  struct timeval RT;
  gettimeofday (&RT, NULL);
  if (usec != NULL) *usec = RT.tv_usec;
  return RT.tv_sec;
};

#endif	/* Neither ZZ_REA nor ZZ_RSY */

extern  char    *ProtocolId;

int main (int argc, char *argv []) {

	int        ic;
	ZZ_REQUEST *chain;
	ZZ_EVENT   *ev, *eh;
	Mailbox    *m;
	ZZ_QITEM   *q;
#if  ZZ_TAG
	int qq;
#endif
	register     ZZ_REQUEST    *cp, *cq;

#if  ZZ_REA
        mark_real_time ();
#endif

#if     ZZ_R48 != 1
	// Initialize the divisor for rnd
	initrnd ();
#endif

	TracingTime = TIME_inf;         // Cannot initialize statically
	zz_max_Time = TIME_inf;

	// Initialize event queue sentinels

	zz_rsent.next = NULL;           // This is the last event
	zz_rsent.prev = &zz_fsent;      // This is the first event
	zz_rsent.chain = NULL;          // Nullify all attributes
#if  ZZ_TAG
	zz_rsent.waketime . set (TIME_inf, MAX_LONG);
#else
	zz_rsent.waketime = TIME_inf;
#endif
	zz_rsent.station = NULL;
	zz_rsent.process = NULL;
	zz_rsent.pstate = 0;
	zz_rsent.event_id = 0;
	zz_rsent.ai = NULL;
	zz_rsent.Info01 = zz_rsent.Info02 = NULL;

	zz_fsent.next = &zz_rsent;
	zz_fsent.prev = NULL;
	zz_fsent.chain = NULL;
#if  ZZ_TAG
	zz_fsent.waketime . set (TIME_0, MIN_LONG);
#else
	zz_fsent.waketime = TIME_0;
#endif
	zz_fsent.station = NULL;
	zz_fsent.process = NULL;
	zz_fsent.pstate = 0;
	zz_rsent.event_id = 0;
	zz_fsent.ai = NULL;
	zz_fsent.Info01 = zz_rsent.Info02 = NULL;

	zz_sentinel_event = &zz_rsent;

	zz_init_system (argc, argv);    // Files + options

#if  ZZ_REA || ZZ_RSY
        // Source journals must be opened first, so that we can use their
        // creation times in setTimeOffset. This in turn is needed to setup
        // the headers of tracing yournals.
        openSourceJournals ();
        setTimeOffset ();
        openTracingJournals ();
#endif
#if  ZZ_REA
	mark_real_time ();
	Ouf << "SIDE Version " << VERSION << "      ";
#else
	Ouf << "SMURPH Version " << VERSION << "      ";
#endif
	if (ProtocolId != NULL)
		Ouf << ProtocolId << "    ";
	Ouf << tDate () << '\n';

	// Include simulator call arguments
	Ouf << "Call arguments:";
	for (argc--, argv++; argc > 0; argc--, argv++) Ouf << ' ' << *argv;
	Ouf << "\n\n";

	// Create the system process and the system station

	TheProcess = NULL;

	// System = create ZZ_SYSTEM;
	System = new ZZ_SYSTEM;
	System->zz_start ();
	System->setup ();
	TheStation = System;

	// ZZ_Kernel = Kernel = create ZZ_KERNEL;
	ZZ_Kernel = Kernel = new ZZ_KERNEL;
	Kernel->zz_start ();
	Kernel->setup ();
	TheProcess = Kernel;

	// Create and start the user process
	
	zz_c_first_wait = YES;
	ZZRoot ();                      // Provided by the user

	if (ZZ_Main == NULL) excptn ("smurph: Root process not provided");

	// Turn the starting event for Root into a priority event -- to make
	// sure that before the Client is started, the network has been
	// initialized properly

	Assert (zz_pe == NULL,
		"smurph: illegal Mailbox->putP issued upon Root startup");

	for (ev = zz_eq; ev != zz_sentinel_event; ev = ev->next) {
		if (ev->process == ZZ_Main && ev->ai == ZZ_Main &&
			ev->event_id == START) {

			zz_pe = ev;
			zz_pr = ev->chain;
			break;
		}
	}

	Assert (zz_pe != NULL, "smurph: failed to create Root");

	// Process the first event -- the one starting the user's coordinator --
	// 'by hand'. A subtle postprocessing is required after this event has
	// been serviced.

	zz_CE = zz_pe;
	zz_CE -> remove ();
	zz_pe = NULL;

#if  ZZ_TAG
	zz_CE -> waketime . get (Time);
#else
	Time        = zz_CE -> waketime;
#endif
	chain       = zz_CE -> chain;
	TheStation  = zz_CE -> station;
	TheProcess  = zz_CE -> process;
	zz_event_id = zz_CE -> event_id;
	Info01      = zz_CE -> Info01;
	Info02      = zz_CE -> Info02;
	TheState    = zz_CE -> pstate;

	Assert ((Time == TIME_0) && (chain != NULL) && (TheStation == System) &&
		(TheProcess == ZZ_Main) && (zz_event_id == START) &&
		(TheState == 0), "smurph: problems starting Root");

	// Deallocate request chain (it should consist of exactly one entry,
	// but who knows ...)
	for (cp = chain; ; cp = cq) {
		cq = cp -> other;
		queue_out (cp);
		delete ((void*) cp);
		if (cq == chain) break;
	}
	zz_c_first_wait = YES;

	if (setjmp (zz_waker))
		// For remote 'sleep'
		goto CoRet;

	// Run the coordinator
	TheProcess->zz_code ();

CoRet:

	TheProcess  = Kernel;
	TheStation  = System;
	TheState    = 0;
	zz_event_id = 0;

	// Start counting events. Note that the very first event initializing
	// the coordinator is not counted.
	ic = 0;
	zz_chckint = CHCKINT;
	buildNetwork ();                // Force topology definition

	if (zz_flg_printDefs) {
		// Output network configuration and more garbage
		System->printTop ("Network topology:");
#if	ZZ_NOC
		Client->printDef ("Traffic:");
#endif
	}

	zz_init_windows ();             // Initialize the window system

	// Deallocate input file
	if ((zz_ifn != NULL) && ((zz_ifn[0] != '.') || (zz_ifn[1] != '\0'))) {
		// Inf.close ();
		delete (zz_ifpp);
		zz_ifpp = NULL;
	}

	zz_setjmp_done = YES;
	setjmp (zz_mloop);

	if (setjmp (zz_waker))
		goto LoRet;

#if  ZZ_REA
        mark_real_time ();
        RT_LastDisplay = RT_LastEvent;
#endif
	for ( ; ; ic++) {

		// The main loop

		// Check exit conditions
#if	ZZ_NOC
		if (!zz_flg_emptyMQueues) {

			// Continue flag is down, check received messages */
			if (zz_NRMessages >= zz_max_NMessages) {
				zz_exit_code = EXIT_msglimit;
			        cdebug ("Terminating on msg limit");
				Kernel->terminate ();
			}

		} else if (zz_flg_emptyMQueues == YES) {

			// Client still active, check generated messages
			if (zz_NGMessages >= zz_max_NMessages) {

			    // Remove Client events from  the  event queue
			    // and continue until message queues are empty

				for (ev = zz_eq; ev != zz_sentinel_event; ) {

					// System events only
					if (ev -> ai != Client || ev -> chain
						!= NULL) {

						ev = ev -> next;
						continue;
					}

					// Release the event
					eh = ev -> next;
					ev -> cancel ();
					delete ev;
					ev = eh;
				}

				// Flag -- flushing message queues
				zz_flg_emptyMQueues = NONE;
				// Restart the main loop
				ic --;
				continue;
			}
		} else {
			// Flushing message queues
			if (zz_NRMessages >= zz_NGMessages) {
				zz_flg_emptyMQueues = NO;
				zz_exit_code = EXIT_msglimit;
			        cdebug ("Terminating on msg limit");
				Kernel->terminate ();
			}
		}
#endif	/* NOC */

		// Virtual time limit
		if (Time >= zz_max_Time) {
			zz_exit_code = EXIT_stimelimit;
		        cdebug ("Terminating on time limit");
			Kernel->terminate ();
		}

		if (ic >= zz_chckint) {
			// Do not check execution time limit too often
			if (zz_chckint == 0) {
				// Checkpoint request
				zz_chckint = CHCKINT;
			} else {
				ic = 0;
				zz_check_asyn_io ();
				if (cpuTime () >= zz_max_cpuTime) {
					zz_exit_code = EXIT_rtimelimit;
		                        cdebug ("Terminating on CPU limit");
					Kernel->terminate ();
				}
			}
		}

		if (zz_pe != NULL) {
			// Priority event
			zz_pe -> new_top_request (zz_pr);
                        delete (zz_CE);
			zz_CE = zz_pe;
			zz_pe -> remove ();
			zz_pe = NULL;
			zz_ai = zz_CE->ai;
			if (zz_ai->Class == AIC_process) {
			  if (zz_CE->event_id == SIGNAL) {
                            ((Process*)zz_ai) -> erase ();
			    assert (Info01 != NULL,
			      "Internal error -- priority signal lost");
			  } else {
			    Info01 = zz_CE -> Info01;
			    Info02 = zz_CE -> Info02;
			  }
			} else {
			  Info02 = zz_CE -> Info02;
			  if (zz_ai->Class == AIC_mailbox &&
			    zz_CE->event_id == RECEIVE) {
                     // Note that priority put is illegal on a socket mailbox
			      // The item must be there
			      m = (Mailbox*) (zz_CE->ai);
    assert (m->count, "Internal error -- mailbox item lost at priority event");
			      (m->count)--;
			      if (m->head != NULL) {
				zz_mbi = Info01 = (q = m->head)->item;
				m->head = q->next;
				m->zz_outitem ();
				delete (q);
			      } else
				Info01 = NULL;
			  } else
			    Info01 = zz_CE -> Info01;
			}
		} else {
			// Regular event
NEXTEV:                

#if ZZ_REA || ZZ_RSY
                        zz_advance_real_time ();
#endif
#if ZZ_REA == 0
                        delete zz_CE;
                        zz_CE = zz_eq->get ();
			if (zz_CE == zz_sentinel_event) {
			  cdebug ("Terminating on no events");
                          break;
                        }
#endif
			zz_ai = zz_CE->ai;
			if (zz_ai->Class == AIC_process) {
			  if (zz_CE->event_id == SIGNAL) {
			    if ((Info01 = ((Process*)zz_ai)->ISender) == NULL) {
			      // Back to the queue
#if     ZZ_TAG
				zz_CE->chain->when . set (TIME_inf);
#else
				zz_CE->chain->when = TIME_inf;
#endif
				chain = zz_CE->chain->min_request ();
				// Simulate new_top_request
				zz_CE->waketime = chain -> when;
				zz_CE->pstate   = chain -> pstate;
				zz_CE->ai       = chain -> ai;
				zz_CE->event_id = chain -> event_id;
				zz_CE->Info01   = chain -> Info01;
				zz_CE->Info02   = chain -> Info02;
				zz_CE->chain    = chain;
				zz_CE->enqueue ();
                                // Avoid deleting this event. We assume that
                                // delete works OK with NULL pointers.
                                zz_CE = NULL;
				goto NEXTEV;
			    }
                            ((Process*)zz_ai) -> erase ();
			  } else {
			    Info01 = zz_CE -> Info01;
			    Info02 = zz_CE -> Info02;
			  }
			} else {
			  Info02 = zz_CE -> Info02;
			  if (zz_ai->Class == AIC_mailbox &&
			    zz_CE->event_id == RECEIVE) {
			      // Check if the item can actually be acquired
			      m = (Mailbox*) (zz_CE->ai);
			      if (m->count == 0) {
				// Sorry, back to the queue
#if     ZZ_TAG
				zz_CE->chain->when . set (TIME_inf);
#else
				zz_CE->chain->when = TIME_inf;
#endif
				chain = zz_CE->chain->min_request ();
				// Simulate new_top_request
				zz_CE->waketime = chain -> when;
				zz_CE->pstate   = chain -> pstate;
				zz_CE->ai       = chain -> ai;
				zz_CE->event_id = chain -> event_id;
				zz_CE->Info01   = chain -> Info01;
				zz_CE->Info02   = chain -> Info02;
				zz_CE->chain    = chain;
				zz_CE->enqueue ();
                                zz_CE = NULL;
				goto NEXTEV;
			      }
			      // If we get here, the item can be acquired
			      (m->count)--;
#if	ZZ_REA || ZZ_RSY
			      if (m->sfd != NONE) {
                                zz_mbi = Info01 = (void*) m->ibuf [m->iout];
                                if (++(m->iout) == m->limit) m->iout = 0;
				m->zz_outitem ();
                              } else
#endif
			      if (m->head != NULL) {
				zz_mbi = Info01 = (q = m->head)->item;
				m->head = q->next;
				m->zz_outitem ();
				delete (q);
			      } else
				Info01 = NULL;
			  } else
			    Info01 = zz_CE -> Info01;
			}
		}

#if     ZZ_TAG
		zz_CE -> waketime . get (Time);      // Advance the time
#else
		Time = zz_CE -> waketime;            // Advance the time
#endif
		chain = zz_CE->chain;
		TheStation  = zz_CE -> station;
		TheProcess  = zz_CE -> process;

		zz_event_id = zz_CE -> event_id;
		TheState    = zz_CE -> pstate;
		zz_ai = zz_CE->ai;

#if	(ZZ_NOL || ZZ_NOR)

#if     ZZ_TAG
	// No implicit priorities are forced, if explicit priorities are
	// in effect
#else
		if (TheStation != NULL) {
		    if (
#if	ZZ_NOL
			zz_ai->Class == AIC_port
#else
			0
#endif
				||
#if	ZZ_NOR
			zz_ai->Class == AIC_transceiver
#else
			0
#endif
			) {
			/* ----------------------------------------------- */
			/* Port/Transceiver AI: force priorities of simul- */
			/* taneous events:                                 */
			/*						   */
			/*         **  BMP                                 */
			/*             BOT                                 */
			/*             ACTIVITY                            */
			/*                                                 */
			/*         **  EMP                                 */
			/*             EOT                                 */
			/*             SILENCE                             */
			/* ----------------------------------------------- */

			cp = chain;

			switch (zz_temp_event_id = (int) zz_event_id) {

			  case  ACTIVITY:

				for (cp = cp->other ; cp != chain;
					cp = cp->other) {

					if ((cp->ai == zz_ai) &&
					     (cp->when == Time)) {
						if (cp->event_id == BOT) {
						  Info01      = cp -> Info01;
						  // Info02 is the same
						  TheState  = cp -> pstate;
						  zz_event_id = cp -> event_id;
						  goto CHMYP;
						} else if (cp->event_id ==
						  BMP) {
						  Info01      = cp -> Info01;
						  // Info02 is the same
						  TheState  = cp -> pstate;
						  zz_event_id = cp -> event_id;
						  goto CSTOP;
						}
					}
				}
				break;

CHMYP:
			  case  BOT:

				for (cp = cp->other; cp != chain;
					cp = cp->other) {

					if ((cp->ai == zz_ai) &&
					     (cp->when == Time) && 
					    (cp->event_id == BMP)) {
						  Info01      = cp -> Info01;
						  // Info02 is the same
						  TheState  = cp -> pstate;
						  zz_event_id = cp -> event_id;
						  goto CSTOP;
					}
				}
				break;

			  case  SILENCE:

				for (cp = cp->other; cp != chain;
					cp = cp->other) {

					if ((cp->ai == zz_ai) &&
					     (cp->when == Time)) {
						if (cp->event_id == EOT) {
						  Info01      = cp -> Info01;
						  // Info02 is the same
						  TheState  = cp -> pstate;
						  zz_event_id = cp -> event_id;
						  goto CHEMP;
						} else if (cp->event_id ==
						  EMP) {
						  Info01      = cp -> Info01;
						  // Info02 is the same
						  TheState  = cp -> pstate;
						  zz_event_id = cp -> event_id;
						  goto CSTOP;
						}
					}
				}
				break;
CHEMP:
			  case  EOT:

				for (cp = cp->other; cp != chain;
					cp = cp->other) {

					if ((cp->ai == zz_ai) &&
					     (cp->when == Time) && 
					    (cp->event_id == EMP)) {
						  Info01      = cp -> Info01;
						  // Info02 is the same
						  TheState  = cp -> pstate;
						  zz_event_id = cp -> event_id;
						  goto CSTOP;
					}
				}
				break;

			   default: ;   // None
			}
		    }
		}
CSTOP:

#endif	/* TAG */

#endif	/* NOL || NOR */

#if     ZZ_DBG
		// Check if tracing is on and, if appropriate, print out debug
		// information

		if (Debugging) zz_print_debug_info ();
#endif
		// Initialize new request chain for the process
		zz_c_first_wait = YES;

		// Restart processes waiting for this process entering this
		// state

		if (TheProcess->SWList != NULL) {
                    cdebug ("Somebody waiting for process: ");
                    cdebugl (TheProcess->getSName ());
		    for (cp = TheProcess->SWList; cp != NULL; cp = cp->next) {
			if (cp->event_id == TheState) {
                            cdebugl (cp->event->process->getSName ());
			    ev = cp->event;
			    // Detect the special case of this process
			    // waiting for itself
			    if (cp->event->process == TheProcess) {
                                cdebugl ("Waiting for itself");
				// Initialize the process wait list (as if
				// the wait request was issued now)
			      if (zz_c_first_wait) {
				zz_c_other = NULL;
				// Create new event
				zz_c_wait_event = new ZZ_EVENT;
				zz_c_wait_event -> station = TheStation;
				zz_c_wait_event -> process = TheProcess;
				new ZZ_REQUEST (&(TheProcess->SWList),
				  TheProcess, TheState, cp->pstate, RQTYPE_PRC);
				zz_c_wait_event -> pstate = cp->pstate;
				zz_c_wait_event -> ai = TheProcess;
				zz_c_wait_event -> event_id = TheState;
#if     ZZ_TAG
				zz_c_wait_tmin . set (Time, cp->when . gett ());
				zz_c_wait_event -> waketime = 
					zz_c_other -> when = zz_c_wait_tmin;
#else
				zz_c_wait_event -> waketime = zz_c_wait_tmin =
					zz_c_other -> when = Time;
#endif
				zz_c_wait_event -> chain = zz_c_other;
				zz_c_wait_event -> Info01 = NULL;
				zz_c_wait_event -> Info02 = NULL;
				zz_c_whead = zz_c_other;
				zz_c_first_wait = NO;
				zz_c_wait_event->enqueue ();
				zz_c_other -> event = zz_c_wait_event;
				zz_c_other -> other = zz_c_whead;
			      } else {
				// The only thing possibly present is another
				// state request of the current process
#if     ZZ_TAG
				if (cp->when . gett () <
				  zz_c_other->when . gett () ||
				    (cp->when . gett () ==
				      zz_c_other->when . gett () && FLIP)) {
#else
				if (FLIP) {
#endif
				  zz_c_wait_event -> pstate =
				    zz_c_other -> pstate = cp->pstate;
				}
			      }
			    } else {
                                cdebugl ("Rescheduling");
#if     ZZ_TAG
				cp -> when . set (Time);
				if ((qq = ev->waketime . cmp (cp->when)) > 0 ||
				  (qq == 0 && FLIP)) {
#else
				cp -> when = Time;
				if (ev->waketime > Time || FLIP) {
#endif
				    ev->new_top_request (cp);
                                    cdebugl ("Rescheduled");
				}
			    }
			}
		    }
		}

		// Release request chain

		if (chain != NULL) {
		    for (cp = chain; ; cp = cq) {
			cq = cp -> other;
			queue_out (cp);

			if (cp->id == RQTYPE_PRC) {
			    // Process AI wait request
			    if (((Process*)(cp->ai))->Owner == NULL &&
				((Process*)(cp->ai))->TWList == NULL &&
				    ((Process*)(cp->ai))->SWList == NULL) {
				// This is a zombie process that couldn't
				// be destroyed in its right time because
				// some processes were waiting for its events.
				// We are at the last such waiting process, so
				// now the zombie can be destroyed.
				assert (((Process*)(cp->ai))->ChList == NULL,
					"Can't terminate %s, ownership list not"
						" empty", cp->ai->getOName ());
				zz_queue_out (cp->ai);
				zz_DREM (cp->ai);
				if (cp->ai->zz_nickname != NULL)
					delete (cp->ai->zz_nickname);
				delete ((void*)(cp->ai));
			    }
			}
			delete ((void*) cp);
			if (cq == chain) break;
		    }
		}
		
		// Run the process
		TheProcess->zz_code ();
LoRet:

#if     ZZ_OBS
		// Check whether should call an observer

		if ((zz_obslist != NULL) && (chain != NULL) &&
			(zz_CE->station != System)) {

			LONG            smask, pmask, nmask, amask;
			ZZ_INSPECT      *ins, *jns;

			/* -------------------------------------------- */
			/* Note: the following instructions assume that */
			/*       words are 32 or 64-bit LONG.           */
			/* -------------------------------------------- */

			smask = SH_1 << ((TheStation->Id) & SHF);
			pmask = SH_1 << ((((int)(TheProcess->zz_typeid)) >> 2) &
				SHF);
			if (TheProcess->zz_nickname != NULL)
			   nmask = SH_1 << ((*(TheProcess->zz_nickname)) & SHF);
			else
			   nmask = SH_1;
			amask = SH_1 << (TheState & SHF);

			zz_observer_running = YES;

			for (zz_current_observer = zz_obslist;
				zz_current_observer != NULL;
				zz_current_observer = zz_current_observer ->
				next) {

				if ((zz_current_observer->amask & amask) &&
				    (zz_current_observer->pmask & pmask) &&
				    (zz_current_observer->nmask & nmask) &&
				    (zz_current_observer->smask & smask)  )  {

					// Examine the inspect list

					for (ins =
						zz_current_observer->inlistHead;
							ins != NULL;
							ins = ins->next) {
	// <------

	if (((ins->pstate == ANY) || (ins->pstate == TheState)) &&
	   (((int)(ins->typeidn) == ANY) || (ins->typeidn ==
	   TheProcess->zz_typeid)) && (((int)(ins->station) == ANY) ||
	   (ins->station == TheStation)) && (((int)(ins->nickname) == ANY) ||
	   smatch(ins->nickname, TheProcess->zz_nickname))) {
		
		TheObserverState = ins->ostate;
		
		// Erase the inspects
		
		for (ins = zz_current_observer->inlistHead; ins != NULL;
			ins=jns) {

			jns = ins -> next;
			delete ((void*) ins);
		}

		zz_current_observer->inlistHead = zz_inlist_tail = NULL;

		// Clear inspect masks

		zz_current_observer->amask = zz_current_observer->pmask =
			zz_current_observer->nmask =
				zz_current_observer->smask = 0;

		// Remove pending timeout event

		if (zz_current_observer->tevent != NULL) {
			zz_current_observer->tevent->cancel ();
			delete (zz_current_observer->tevent);
			zz_current_observer->tevent = NULL;
		}

		// Call the observer

		do {
		  zz_jump_flag = NO;
		  zz_current_observer->zz_code ();
		  if (DisplayActive) {
		    // Observer stepping check
		    ZZ_SIT   *ws;

		    for (ws = zz_steplist; ws != NULL; ws = ws->next) {
		      if (ws->obs != zz_current_observer) continue;
		      if (ws->station != NULL && ws->station != TheStation)
			continue;
		      if (ws->process != NULL && ws->process != TheProcess)
			continue;
		      if (ws->ai != NULL && ws->ai != zz_CE->ai)
			continue;

		      zz_events_to_display = 0;
		      zz_send_step_phrase = YES;
		      break;
		    }
		  }

		  TheObserverState = zz_state_to_jump;

		} while (zz_jump_flag);

		break;          // Continue with the next observer
	}

	// ------>
					}
				}
			}
		}

		zz_observer_running = NO;
#endif
		if (DisplayActive) {
		  if (zz_steplist_delayed != NULL) {
		    // There are windows waiting to be stepped
		    if (zz_npre >= zz_commit_event || Time >= zz_commit_time) {
		      // Find the windows to be added to steplist
		      ZZ_WSIT  *wsd, *wsq;
		      ZZ_SIT   *ws;

		      for (wsd = zz_steplist_delayed; wsd != NULL; wsd = wsq) {
			wsq = wsd->next;
			if (wsd->cevent <= zz_npre || wsd->ctime <= Time) {
			  // Move it to steplist
			  ws = new ZZ_SIT;
			  ws->win = wsd->win;
			  ws->station = wsd->station;
			  ws->process = wsd->process;
			  ws->ai = wsd->ai;
			  ws->obs = wsd->obs;
			  queue_head (ws, zz_steplist, ZZ_SIT);
			  queue_out (wsd);
			  delete (wsd);
			}
		      }

		      // Calculate new minima

		      for (zz_commit_time = TIME_inf,
			zz_commit_event = MAX_Long, wsd = zz_steplist_delayed;
				wsd != NULL; wsd = wsd->next) {

			if (wsd->ctime < zz_commit_time)
			  zz_commit_time = wsd->ctime;
			if (wsd->cevent < zz_commit_event)
			  zz_commit_event = wsd->cevent;
		      }
		    }
		  }

		  // Done with the delayed list, now regular step test

		  if (zz_steplist != NULL) {
		    // There are windows to be stepped: look for match
		    ZZ_SIT   *ws;

		    for (ws = zz_steplist; ws != NULL; ws = ws->next) {
		      if (ws->station != NULL && ws->station != TheStation)
			continue;
		      if (ws->process != NULL && ws->process != TheProcess)
			continue;
		      if (ws->ai != NULL && ws->ai != zz_CE->ai)
			continue;
		      if (ws->obs == NULL) break;
		      // Observers tested elsewhere
		    }
		    if (ws != NULL) {
		      // Step it
		      zz_events_to_display = 0;
		      zz_send_step_phrase = YES;
		    }
		  }
#if ZZ_REA || ZZ_RSY
                  if (zz_send_step_phrase) refreshDisplay ();
                  // We will block here awaiting a "go" from DSD
#else
                  // In real mode this is checked elsewhere
		  zz_check_asyn_io ();
		  if (--zz_events_to_display <= 0) refreshDisplay ();
#endif
		}

		zz_npre++;              // Number of processed events

		if (zz_c_first_wait && TheStation != System && TheProcess !=
		  Kernel) {
		    // The wait list is empty -- terminate the process
		    terminate (TheProcess);
                    // We need these two for display routines. The event
                    // may end up hanging around for some time and being
                    // displayed possibly several times. We don't wont the
                    // display routines to grab null pointers.
                    if (zz_CE->ai == TheProcess) zz_CE->ai = NULL;
                    if (zz_ai == TheProcess) zz_ai = NULL;
                    TheProcess = NULL;
                }
	}
	zz_done ();
}

/* ----------------------------------- */
/* Non-inline random number generators */
/* ----------------------------------- */

TIME    tRndTolerance   (TIME a, TIME b, int q) {

/* ---------------------------------------------------------- */
/* Generates  a  number between a and b according to the Beta */
/* (q,q) distribution (BIG version)                           */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("tRndTolerance");
#else
	double                  y;
	register        double  p;
	register        int     i;
	TIME                    r;

	assert (q > 0, "tRndTolerance: q (%1d) must be > 0", q);
	for (p = 1.0, i = 1; i <= q; i++, p *= 1.0 - rnd (SEED_delay));
	y = -log (p);
	for (p = 1.0, i = 1; i <= q; i++, p *= 1.0 - rnd (SEED_delay));
	y = y / (y - log (p));
	r = a + (TIME)(((double)(b - a) + 1.0) * y);
	if (r > b) return (b); else return (r);
#endif
};

/* ============================================================================
 * This part borrowed from gsl 1.7:
 * 
 * Copyright (C) 1996-2003 James Theiler, Brian Gough
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 * ============================================================================
 */

#define SMALL_MEAN 	14
#define BINV_CUTOFF	110
#define FAR_FROM_MEAN 	20

inline static double Stirling (double y1) {

	double y2 = y1 * y1;
	double s = (13860.0 - (462.0 - (132.0 - (99.0 - 140.0 /
					y2) / y2) / y2) / y2) / y1 / 166320.0;
	return s;
}

double pow_int (double x, int n) {

/* ------------------------------- */
/* This one may be generally handy */
/* ------------------------------- */

  double value = 1.0;

  if(n < 0) {
    x = 1.0/x;
    n = -n;
  }

  /* repeated squaring method 
   * returns 0.0^0 = 1.0, so continuous in x
   */
  do {
     if(n & 1) value *= x;  /* for n odd */
     n >>= 1;
     x *= x;
  } while (n);

  return value;
}

Long lRndBinomial (double p, Long n) {

  int ix;                       /* return value */
  int flipped = 0;
  double q, s, np;

  if (n == 0)
    return 0;

  if (p > 0.5)
    {
      p = 1.0 - p;              /* work with small p */
      flipped = 1;
    }

  q = 1 - p;
  s = p / q;
  np = n * p;

  /* Inverse cdf logic for small mean (BINV in K+S) */

  if (np < SMALL_MEAN)
    {
      double f0 = pow_int (q, n);   /* f(x), starting with x=0 */

      while (1)
        {
          /* This while(1) loop will almost certainly only loop once; but
           * if u=1 to within a few epsilons of machine precision, then it
           * is possible for roundoff to prevent the main loop over ix to
           * achieve its proper value.  following the ranlib implementation,
           * we introduce a check for that situation, and when it occurs,
           * we just try again.
           */

          double f = f0;
          double u = rnd (SEED_toss);

          for (ix = 0; ix <= BINV_CUTOFF; ++ix)
            {
              if (u < f)
                goto Finish;
              u -= f;
              /* Use recursion f(x+1) = f(x)*[(n-x)/(x+1)]*[p/(1-p)] */
              f *= s * (n - ix) / (ix + 1);
            }

          /* It should be the case that the 'goto Finish' was encountered
           * before this point was ever reached.  But if we have reached
           * this point, then roundoff has prevented u from decreasing
           * all the way to zero.  This can happen only if the initial u
           * was very nearly equal to 1, which is a rare situation.  In
           * that rare situation, we just try again.
           *
           * Note, following the ranlib implementation, we loop ix only to
           * a hardcoded value of SMALL_MEAN_LARGE_N=110; we could have
           * looped to n, and 99.99...% of the time it won't matter.  This
           * choice, I think is a little more robust against the rare
           * roundoff error.  If n>LARGE_N, then it is technically
           * possible for ix>LARGE_N, but it is astronomically rare, and
           * if ix is that large, it is more likely due to roundoff than
           * probability, so better to nip it at LARGE_N than to take a
           * chance that roundoff will somehow conspire to produce an even
           * larger (and more improbable) ix.  If n<LARGE_N, then once
           * ix=n, f=0, and the loop will continue until ix=LARGE_N.
           */
        }
    }
  else
    {
      /* For n >= SMALL_MEAN, we invoke the BTPE algorithm */

      int k;

      double ffm = np + p;      /* ffm = n*p+p             */
      int m = (int) ffm;        /* m = int floor[n*p+p]    */
      double fm = m;            /* fm = double m;          */
      double xm = fm + 0.5;     /* xm = half integer mean (tip of triangle)  */
      double npq = np * q;      /* npq = n*p*q            */

      /* Compute cumulative area of tri, para, exp tails */

      /* p1: radius of triangle region; since height=1, also: area of region */
      /* p2: p1 + area of parallelogram region */
      /* p3: p2 + area of left tail */
      /* p4: p3 + area of right tail */
      /* pi/p4: probability of i'th area (i=1,2,3,4) */

      /* Note: magic numbers 2.195, 4.6, 0.134, 20.5, 15.3 */
      /* These magic numbers are not adjustable...at least not easily! */

      double p1 = floor (2.195 * sqrt (npq) - 4.6 * q) + 0.5;

      /* xl, xr: left and right edges of triangle */
      double xl = xm - p1;
      double xr = xm + p1;

      /* Parameter of exponential tails */
      /* Left tail:  t(x) = c*exp(-lambda_l*[xl - (x+0.5)]) */
      /* Right tail: t(x) = c*exp(-lambda_r*[(x+0.5) - xr]) */

      double c = 0.134 + 20.5 / (15.3 + fm);
      double p2 = p1 * (1.0 + c + c);

      double al = (ffm - xl) / (ffm - xl * p);
      double lambda_l = al * (1.0 + 0.5 * al);
      double ar = (xr - ffm) / (xr * q);
      double lambda_r = ar * (1.0 + 0.5 * ar);
      double p3 = p2 + c / lambda_l;
      double p4 = p3 + c / lambda_r;

      double var, accept;
      double u, v;              /* random variates */

    TryAgain:

      /* generate random variates, u specifies which region: Tri, Par, Tail */
      u = rnd (SEED_toss) * p4;
      v = rnd (SEED_toss);

      if (u <= p1)
        {
          /* Triangular region */
          ix = (int) (xm - p1 * v + u);
          goto Finish;
        }
      else if (u <= p2)
        {
          /* Parallelogram region */
          double x = xl + (u - p1) / c;
          v = v * c + 1.0 - fabs (x - xm) / p1;
          if (v > 1.0 || v <= 0.0)
            goto TryAgain;
          ix = (int) x;
        }
      else if (u <= p3)
        {
          /* Left tail */
          ix = (int) (xl + log (v) / lambda_l);
          if (ix < 0)
            goto TryAgain;
          v *= ((u - p2) * lambda_l);
        }
      else
        {
          /* Right tail */
          ix = (int) (xr - log (v) / lambda_r);
          if (ix > (double) n)
            goto TryAgain;
          v *= ((u - p3) * lambda_r);
        }

      /* At this point, the goal is to test whether v <= f(x)/f(m) 
       *
       *  v <= f(x)/f(m) = (m!(n-m)! / (x!(n-x)!)) * (p/q)^{x-m}
       *
       */

      k = abs (ix - m);

      if (k <= FAR_FROM_MEAN)
        {
          /* 
           * If ix near m (ie, |ix-m|<FAR_FROM_MEAN), then do
           * explicit evaluation using recursion relation for f(x)
           */
          double g = (n + 1) * s;
          double f = 1.0;

          var = v;

          if (m < ix)
            {
              int i;
              for (i = m + 1; i <= ix; i++)
                {
                  f *= (g / i - s);
                }
            }
          else if (m > ix)
            {
              int i;
              for (i = ix + 1; i <= m; i++)
                {
                  f /= (g / i - s);
                }
            }

          accept = f;
        }
      else
        {
          /* If ix is far from the mean m: k=ABS(ix-m) large */

          var = log (v);

          if (k < npq / 2 - 1)
            {
              /* "Squeeze" using upper and lower bounds on
               * log(f(x)) The squeeze condition was derived
               * under the condition k < npq/2-1 */
              double amaxp =
                k / npq * ((k * (k / 3.0 + 0.625) + (1.0 / 6.0)) / npq + 0.5);
              double ynorm = -(k * k / (2.0 * npq));
              if (var < ynorm - amaxp)
                goto Finish;
              if (var > ynorm + amaxp)
                goto TryAgain;
            }

          /* Now, again: do the test log(v) vs. log f(x)/f(M) */

          /* The "#define Stirling" above corresponds to the first five
           * terms in asymptoic formula for
           * log Gamma (y) - (y-0.5)log(y) + y - 0.5 log(2*pi);
           * See Abramowitz and Stegun, eq 6.1.40
           */

          /* Note below: two Stirling's are added, and two are
           * subtracted.  In both K+S, and in the ranlib
           * implementation, all four are added.  I (jt) believe that
           * is a mistake -- this has been confirmed by personal
           * correspondence w/ Dr. Kachitvichyanukul.  Note, however,
           * the corrections are so small, that I couldn't find an
           * example where it made a difference that could be
           * observed, let alone tested.  In fact, define'ing Stirling
           * to be zero gave identical results!!  In practice, alv is
           * O(1), ranging 0 to -10 or so, while the Stirling
           * correction is typically O(10^{-5}) ...setting the
           * correction to zero gives about a 2% performance boost;
           * might as well keep it just to be pendantic.  */

          {
            double x1 = ix + 1.0;
            double w1 = n - ix + 1.0;
            double f1 = fm + 1.0;
            double z1 = n + 1.0 - fm;

            accept = xm * log (f1 / x1) + (n - m + 0.5) * log (z1 / w1)
              + (ix - m) * log (w1 * p / (x1 * q))
              + Stirling (f1) + Stirling (z1) - Stirling (x1) - Stirling (w1);
          }
        }

      if (var <= accept)
        {
          goto Finish;
        }
      else
        {
          goto TryAgain;
        }
    }

Finish:

  return (flipped) ? (n - ix) : ix;
}

/* ============================================================================
 * End of borrowed part
 * ============================================================================
 */

#if 	ZZ_NFP
void zz_nfp (const char *em) {
  char tmp [256];
  strcpy (tmp, "FLOATING POINT OPERATION: ");
  strcat (tmp, em);
  excptn (tmp);
};

double  ceil (double d)		{ zz_nfp ("ceil"); 		};
double  cos(double d)   	{ zz_nfp ("cos"); 		};
double  exp(double d)		{ zz_nfp ("exp");		};
double  fabs(double d)		{ zz_nfp ("fabs");		};
double  floor(double d)		{ zz_nfp ("floor");		};
double  log(double d)		{ zz_nfp ("log");		};
double  log10(double d)		{ zz_nfp ("log10");		};
double  pow(double d, double e)	{ zz_nfp ("pow");		};
double  sin(double d)		{ zz_nfp ("sin");		};
double  sqrt(double d)		{ zz_nfp ("sqrt");		};
double  tan(double d)		{ zz_nfp ("tan");		};
double	atof(const char *s) 	{ zz_nfp ("atof");		};

#endif
