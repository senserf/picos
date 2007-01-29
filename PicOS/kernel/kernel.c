/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"

/* ========================================================================== */
/*                     PicOS                                                  */
/*                                                                            */
/* The kernel                                                                 */
/* ========================================================================== */

/*
 * Put all lword variables here to avoid losing memory on alignment
 */
lword 	zz_nseconds;

#if	ENTROPY_COLLECTION
lword	zzz_ent_acc;
#endif

#if	RANDOM_NUMBER_GENERATOR > 1
lword	zz_seed = 327672838L;
#endif

#include "pins.h"

/* Task table */
pcb_t __PCB [MAX_TASKS];

#if MAX_DEVICES
/* Device table */
static devreqfun_t ioreq [MAX_DEVICES];
#endif

/* System status word */
systat_t 		zz_systat;

/* =============== */
/* Current process */
/* =============== */
pcb_t	*zz_curr;

/* ========= */
/* The clock */
/* ========= */
static 		word  	setticks, millisec;
		word 	zz_mintk;
volatile	word 	zz_lostk;

/* ================================ */
/* User-defineable countdown timers */
/* ================================ */
address		zz_utims [MAX_UTIMERS];

const char	zz_hex_enc_table [] = {
				'0', '1', '2', '3', '4', '5', '6', '7',
				'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
			      };

#if	RANDOM_NUMBER_GENERATOR == 1
word	zz_seed = 30011;
#endif

void zz_badstate (void) {
	syserror (ESTATE, "no such state");
}

int utimer (address ut, Boolean add) {
/* ================= */
/* Add/clear utimers */
/* ================= */
	int i;

	/* Check if the timer is not in SDRAM */
#if	SDRAM_PRESENT
	if (ut >= (address)SDRAM_ADDR && ut < (address)
		((word)SDRAM_ADDR + ((word)1 << SDRAM_SIZE)))
		syserror (EREQPAR, "utimer in sdram");
#endif

	for (i = 0; i < MAX_UTIMERS; i++)
		if (zz_utims [i] == NULL)
			break;

	if (add) {
		/* Install a new timer */
		if (i == MAX_UTIMERS)
			/* The pool is full */
			return 0;
		zz_utims [i] = ut;
		/* Return the number of all utimers */
		return i+1;
	}

	/* Delete */
	if (ut == NULL) {
		/* Clear all utimers */
		zz_utims [0] = NULL;
		return i;
	}

	/* Delete a single specific timer */
	for (i = 0; i < MAX_UTIMERS; i++) {
		if (zz_utims [i] == 0)
			/* Not found */
			return 0;
		if (zz_utims [i] == ut)
			break;
	}
	cli_tim;
	while (i < MAX_UTIMERS-1) {
		zz_utims [i] = zz_utims [i+1];
		if (zz_utims [i] == NULL)
			break;
		i++;
	}
	sti_tim;
	return i+1;
}

lword seconds () {

	return zz_nseconds;
}

void zzz_tservice () {

	word nticks;
	pcb_t *i;

	cli_tim;
	nticks = zz_lostk;
	zz_lostk = 0;
	sti_tim;

	if (nticks == 0)
		return;

	/* Keep the seconds clock running */
	millisec += nticks;
	while (millisec >= JIFFIES) {
		millisec -= JIFFIES;
		zz_nseconds++;

#include "board_second.h"

		if ((zz_nseconds & 63) == 0)
			/* Do this every "minute" */
			ldtrigger ((word) (zz_nseconds >> 6));
	}

	do {
		if (zz_mintk == 0)
			// Minimum ticks to a wakeup
			return;
		if (zz_mintk > nticks) {
			// More than elapsed to this run: just decrement the
			// count
			zz_mintk -= nticks;
			return;
		}

		// nticks >= zz_mintk; normally, we will have
		// nticks == zz_mintk, but lets us play it safe (in case we have
		// lost a tick

		nticks -= zz_mintk;
		zz_mintk = 0;

		for_all_tasks (i) {
			if (i->Timer == 0)
				// Not waiting for Timer
				continue;
			if (!twaiting (i)) {
				// Not waiting either; this test is more costly,
				// so fix it to speed up future checks
				i->Timer = 0;
				continue;
			}
			if (i->Timer <= setticks) {
				// setticks is the number of ticks for which the
				// timer was last set; thus, this means that
				// this particular process must be awakened
				i->Timer = 0;
				wakeuptm (i);
			} else {
				// Reset the timer
				i->Timer -= setticks;
				if (zz_mintk == 0 || i->Timer < zz_mintk)
					// And calculate new setticks as the
					// minimum of them all
					zz_mintk = i->Timer;
			}
		}
		// This is the new minimum
		setticks = zz_mintk;
		// Keep going in case we have a lag
	} while (nticks);
}

/* ==================== */
/* Launch a new process */
/* ==================== */
int zzz_fork (code_t func, address data) {

	pcb_t *i;
	int pc;

	/* Limit for the number of instances */
	if ((pc = func (0xffff, NULL)) > 0) {
		for_all_tasks (i)
			if (i->code == func)
				pc--;
		if (pc <= 0)
			return 0;
	}

	for_all_tasks (i)
		if (i->code == NULL)
			break;

	if (i == LAST_PCB)
		return (int) NONE;

	i -> Timer = 0;
	i -> code = func;
	i -> data = (address) data;
	i -> Status = 0;
#if SCHED_PRIO
	i -> prio = tasknum (i) << 3;
#endif
	return (int) i;
}

void savedata (void *d) {

	zz_curr -> data = (address) d;
}

/* ======================== */
/* Proceed to another state */
/* ======================== */
void proceed (word state) {

	prcdstate (zz_curr, state);
	release;
}

/* ============*/
/* System wait */
/* =========== */
void zz_swait (word etype, word event, word state) {

	int j = nevents (zz_curr);

	if (j == MAX_EVENTS_PER_TASK)
		syserror (ENEVENTS, "swait");

	setestatus (zz_curr->Events [j], etype, state);
	zz_curr->Events [j] . Event = event;
	incwait (zz_curr);
}

/* ========= */
/* User wait */
/* ========= */
void wait (word event, word state) {

	int j = nevents (zz_curr);

	if (j == MAX_EVENTS_PER_TASK)
		syserror (ENEVENTS, "wait");

	setestatus (zz_curr->Events [j], ETYPE_USER, state);
	zz_curr->Events [j] . Event = event;

	incwait (zz_curr);
}

/* ========== */
/* Timer wait */
/* ========== */
void delay (word d, word state) {

	word t;
	pcb_t *i;

	settstate (zz_curr, state);
	if (d != 0) {
		if (zz_mintk == 0) {
			// Alarm not set, we are alone, this case is easy. Note
			// that setticks tells the last setting of the timer,
			// while zz_mintk runs down at the clock rate to zero.
			zz_curr->Timer = zz_mintk = setticks = d;
		} else if (zz_mintk <= d) {
			// The alarm clock will go off earlier than required
			// to wake us up. Thus, we have to increase our
			// requested delay by the time elapsed since the
			// last setting.
			if ((t = d + (setticks - zz_mintk)) < d) {
HardWay:
				// We have run out of precision, use the 
				// precision-safe version that involves no
				// incrementation
				sysassert (setticks >= zz_mintk, "delay mintk");
				setticks -= zz_mintk;
				zz_mintk = 0;
				for_all_tasks (i) {
					if (i->Timer == 0)
						// Not waiting
						continue;
					if (!twaiting (i)) {
						i->Timer = 0;
						continue;
					}
					sysassert (i->Timer > setticks,
						"delay setticks");
					i->Timer -= setticks;
					if (zz_mintk == 0 || i->Timer <
					    zz_mintk)
						zz_mintk = i->Timer;
				}
				sysassert (zz_mintk > 0, "delay minwait");
				zz_curr->Timer = d;
				if (zz_mintk > d)
					zz_mintk = d;
				setticks = zz_mintk;

			} else {
				zz_curr->Timer = t;
			}
		} else {
			// The alarm clock would go off later than required, so
			// we have to reset it.
			if ((t = d + (setticks - zz_mintk)) < d) {
				// Out of precision
				goto HardWay;
			}
			zz_curr->Timer = setticks = t;
			zz_mintk = d;
		}
	} else {
		zz_curr->Timer = 0;
	}
	/* Indicate that we are waiting for the Timer */
	inctimer (zz_curr);
}

/* ======================================================================== */
/* Return the number of milliseconds that the indicated process is going to */
/* sleep for                                                                */
/* ======================================================================== */
word dleft (int pid) {

	pcb_t *i;

	ver_pid (i, pid);
	if (i->code == NULL || !twaiting (i))
		return MAX_UINT;

	return (i->Timer > (setticks - zz_mintk)) ?
		i->Timer - (setticks - zz_mintk) : 0;
}

/* =========== */
/* Minute wait */
/* =========== */
void ldelay (word d, word state) {

	int j = nevents (zz_curr);

	if (j == MAX_EVENTS_PER_TASK)
		syserror (ENEVENTS, "ldelay");

	if (d == 0)
		// There is no way to wait for zero minutes, use delay for that
		syserror (EREQPAR, "ldelay");

	setestatus (zz_curr->Events [j], ETYPE_LDELAY, state);
	zz_curr->Events [j] . Event = (word) ((zz_nseconds >> 6) + d);

	incwait (zz_curr);
}

/* ======================================================================= */
/* Return the number of minutes (and optionally seconds) remaining for the */
/* process to long-sleep                                                   */
/* ======================================================================= */
word ldleft (int pid, word *s) {

	pcb_t	*i;
	word	j, nmin, ldel;

	ver_pid (i, pid);

	if (i->code == NULL)
		return MAX_UINT;

	j = ((word) zz_nseconds & 0x3f);
	if (s != NULL)
		*s = j ? 64 - j : 0;

	nmin = (word)(zz_nseconds >> 6);
	if (s == NULL) {
		// Round it to the nearest minute
		if (j > 32)
			nmin++;
	} else {
		// Keep it exact
		if (j)
			nmin++;
	}

	ldel = MAX_UINT;

	for (j = 0; j < nevents (i); j++)
		if (getetype (i->Events [j]) == ETYPE_LDELAY)
			if (i->Events [j] . Event - nmin < ldel)
				ldel = i->Events [j] . Event - nmin;

	return ldel;
}

/* =============================== */
/* Continue interrupted timer wait */
/* =============================== */
void snooze (word state) {

	settstate (zz_curr, state);
	if (zz_mintk == 0 || zz_mintk > zz_curr->Timer) {
		/*
		 * This is a bit heuristic, but at least avoids problems
		 * resulting from a possible misuse.
		 */
		delay (zz_curr->Timer, state);
		return;
	}
	inctimer (zz_curr);
}

/* ============== */
/* System trigger */
/* ============== */
int zz_strigger (int etype, word event) {

	int j, c;
	pcb_t *i;

	c = 0;
	for_all_tasks (i) {
		if (nevents (i) == 0)
			continue;
		if (i->code == NULL)
			continue;
		for (j = 0; j < nevents (i); j++) {
			if (i->Events [j] . Event == event &&
				getetype (i->Events [j]) == etype) {
					/* Wake up */
					wakeupev (i, j);
					c++;
					break;
			}
		}
	}

	return c;
}

/* ============ */
/* User trigger */
/* ============ */
int trigger (word event) {

	int j, c;
	pcb_t *i;

	c = 0;
	for_all_tasks (i) {
		if (nevents (i) == 0)
			continue;
		if (i->code == NULL)
			continue;
		for (j = 0; j < nevents (i); j++) {
			if (i->Events [j] . Event == event &&
				getetype (i->Events [j]) == ETYPE_USER) {
					/* Wake up */
					wakeupev (i, j);
					c++;
					break;
			}
		}
	}

	return c;
}

static void killev (int pid, word wfun) {
	word etp;
	int j;
	pcb_t *i;

	for_all_tasks (i) {
		if (nevents (i) == 0 || i->code == NULL)
			continue;
		for (j = 0; j < nevents (i); j++) {
			etp = getetype (i->Events [j]);
			if ((etp == ETYPE_TERM &&
				i->Events [j] . Event == pid) ||
			    (etp == ETYPE_TERMANY &&
				i->Events [j] . Event == wfun) ) {
					wakeupev (i, j);
					break;
			}
		}
	}
}

int kill (int pid) {
/* =========================== */
/* Terminate process execution */
/* =========================== */

	pcb_t *i;

	if (pid == 0 || pid == -1 || pid == (int) zz_curr) {
		/* Self */
		killev ((int)zz_curr, (word)(zz_curr->code));
		zz_curr->Status = 0;
		zz_curr->Timer = 0;
		if (pid == -1) {
			/* This makes you a zombie ... */
			swait (ETYPE_TERM, 0, 0);
		} else {
			/* ... and this makes you dead */
			zz_curr->code = NULL;
		}
		release;
	}

	ver_pid (i, pid);

	if (i->code != NULL) {
		killev ((int)i, (word)(i->code));
		i->Status = 0;
		i->code = NULL;
		return pid;
	}
	return 0;
}

int killall (code_t fun) {

	pcb_t *i;
	Boolean rel;
	int nk;

	rel = NO;
	nk = 0;
	for_all_tasks (i) {
		if (i->code == fun) {
			if (i == zz_curr)
				rel = YES;
			killev ((int)i, (word)(i->code));
			i->Status = 0;
			i->code = NULL;
			nk++;
		}
	}
	if (rel)
		release;
	return nk;
}

#if SCHED_PRIO

int prioritizeall (code_t fun, int pr) {

	pcb_t *i;
	int np = 0;

	sysassert (((pr >= 0) && (pr < MAX_PRIO)), "priority");

	for_all_tasks (i){
		if (i->code == fun) {
			i -> prio = pr;
			np++;
		}
	}

	return np;
}

int prioritize (int pid, int pr) {

	pcb_t *i;

	if (pid == 0)
	        i = zz_curr;
	else
	        ver_pid (i, pid);

	if ((pr >= 0) && (pr < MAX_PRIO))
	        i -> prio = pr;

	return i -> prio ;
}

#endif

int status (int pid) {

	pcb_t *i;
	int res;

	if (pid == 0)
		i = zz_curr;
	else
		ver_pid (i, pid);

	res = nevents (i);

	if (res == 1 && getetype (i->Events [0]) == ETYPE_TERM &&
		i->Events [0] . Event == 0)
			/* Zombie */
			return -1;

	if (twaiting (i))
		res++;

	return res;
}

code_t getcode (int pid) {

	pcb_t *i;

	if (pid == 0)
		i = zz_curr;
	else
		ver_pid (i, pid);

	return i->code;
}

int join (int pid, word state) {
	pcb_t *i;

	/* Check if pid is legit */

	ver_pid (i, pid);

	if (i->code == NULL)
		return 0;

	/* Do not wait for anything if the process is a zombie already */
	if (nevents (i) != 1 || getetype (i->Events [0]) != ETYPE_TERM ||
		i->Events [0] . Event != 0)
			swait (ETYPE_TERM, pid, state);

	return (int)i;
}

void joinall (code_t fun, word state) {

	swait (ETYPE_TERMANY, (word)fun, state);
}

int running (code_t fun) {

	pcb_t *i;

	if (fun == NULL)
		return (int) zz_curr;

	for_all_tasks (i)
		if (i->code == fun)
			return (int) i;

	return 0;
}

int zzz_find (code_t fun, address dat) {

	pcb_t *i;

	for_all_tasks (i)
		if (i->code == fun && i->data == dat)
			return (int) i;

	return 0;
}

int zombie (code_t fun) {

	pcb_t *i;

	for_all_tasks (i)
		if (i->code == fun && nevents (i) == 1 &&
			getetype (i->Events [0]) == ETYPE_TERM &&
				i->Events [0] . Event == 0)
					return (int) i;
	return 0;
}

/* ========================================================================= */
/* Simple operations on strings. We prefer to have everything under control. */
/* ========================================================================= */
int zzz_strlen (const char *s) {

	int i;

	for (i = 0; *(s+i) != '\0'; i++);

	return i;
}

void zzz_strcpy (char *d, const char *s) {

	while ((Boolean)(*d++ = *s++));
}

void zzz_strncpy (char *d, const char *s, int n) {

	while (n-- && (*s != '\0'))
		*d++ = *s++;
	*d = '\0';
}

void zzz_strcat (char *d, const char *s) {

	while (*d != '\0') d++;
	strcpy (d, s);
}

void zzz_strncat (char *d, const char *s, int n) {

	strcpy (d+n, s);
}

void zzz_memcpy (char *dest, const char *src, int n) {

	while (n--)
		*dest++ = *src++;
}

void zzz_memset (char *dest, char c, int n) {

	while (n--)
		*dest++ = c;
}

#if MAX_DEVICES
/* ============================================= */
/* Configures a driver-specific service function */
/* ============================================= */
void adddevfunc (devreqfun_t rf, int loc) {

	if (loc < 0 || loc >= MAX_DEVICES)
		syserror (EREQPAR, "addevfunc");

	if (ioreq [loc] != NULL)
		syserror (ERESOURCE, "addevfunc");

	ioreq [loc] = rf;
}
#endif	/* MAX_DEVICES */

#if MAX_DEVICES
/* ==================================== */
/* User-visible i/o request distributor */
/* ==================================== */
int io (int retry, int dev, int operation, char *buf, int len) {

	int ret;

	if (dev < 0 || dev >= MAX_DEVICES || ioreq [dev] == NULL)
		syserror (ENODEVICE, "io");

	if (len == 0)
		/* This means that the call is void */
		return 0;

	ret = (ioreq [dev]) (operation, buf, len);

	if (ret >= 0)
		return ret;

	if (ret == -1) {
		/*
		 * This means busy with the ready event to be triggered by
		 * a process, in which case iowait requires no locking.
		 */
		if (retry == NONE)
			return 0;
		iowait (dev, operation, retry);
		release;
	}

	if (ret == -2) {
		/* This means busy with the ready event to be triggered by an
		 * interrupt handler. This option provides a shortcut for those
		 * drivers that do not require separate driver processes, yet
		 * want to perceive interrupts. In such a case, we have to call
		 * the ioreq function again to clean up (unmask interrupts)
	 	*/
		if (retry != NONE) {
			iowait (dev, operation, retry);
			(ioreq [dev]) (NONE, buf, len);
			release;
		}
		(ioreq [dev]) (NONE, buf, len);
		return 0;
	}

	/* ret < -2. This means a timer retry after -ret -2 milliseconds */
	if (retry != NONE) {
		return 0;
	}

	delay (-ret - 2, retry);
	release;

#ifdef	__ECOG1__
	/* Against stupidity ... */
	return 0;
#endif
}

#endif	/* MAX_DEVICES */

/* --------------------------------------------------------------------- */
/* ========================= MEMORY ALLOCATORS ========================= */
/* --------------------------------------------------------------------- */

#if	MALLOC_SINGLEPOOL == 0
extern	word zzz_heap [];
#endif

static 	address *mpools;
static	address mevent;

#define	MEV_NWAIT(np)	(*((byte*)(&(mevent [np])) + 0 ))
#define	MEV_NFAIL(np)	(*((byte*)(&(mevent [np])) + 1 ))

#if	MALLOC_STATS
static 	address mnfree, mcfree;
#endif

#define	m_nextp(c)	((address)(*(c)))
#define m_setnextp(c,p)	(*(c) = (word)(p))
#define m_size(c)	(*((c)-1))
#define	m_magic(c) 	(*((c)+1))
#define	m_hdrlen	1

#if 0

void dump_malloc (const char *s) {

#if	MALLOC_SINGLEPOOL
#define	MPMESS 		"N %u, F %u", nc, tot
#define	NPOOLS		1
#else
#define	MPMESS		"POOL %d, N %u, F %u", i, nc, tot
#define	NPOOLS		2
#endif

	word tot, nc;
	address ch;
	int i;

	diag (s);

	for (i = 0; i < NPOOLS; i++) {
		tot = 0;
		for (nc = 0, ch = mpools [i]; ch != NULL; ch = m_nextp (ch)) {
			tot += m_size (ch);
			nc++;
		}
		diag (MPMESS);
	}

#undef	MPMESS
#undef	NPOOLS

}

#endif

void zz_malloc_init () {
/* =================================== */
/* Initializes memory allocation pools */
/* =================================== */

#if	MALLOC_SINGLEPOOL
#define	MA_NP		1
#else
#define	MA_NP		np
	word	mtot, np, perc, chunk, fac, ceil;
	address freelist, morg;

	for (perc = np = 0; perc < 100; np++)
		/* Calculate the number of pools */
		perc += zzz_heap [np];
#endif	/* MALLOC_SINGLEPOOL */

	if (MALLOC_LENGTH < 256 + MA_NP)
		/* Make sure we do have some memory available */
		syserror (ERESOURCE, "malloc_init (1)");

	/* Set aside the free list table for the pools */
	mpools = (address*) MALLOC_START;
	mevent = MALLOC_START + MA_NP;

#if	MALLOC_SINGLEPOOL

#if	MALLOC_STATS
	mnfree = mevent + 1;
	mcfree = mnfree + 1;
#define	morg 		(mcfree + 1)
#define	mtot		(MALLOC_LENGTH - 4)
#else	/* MALLOC_STATS */
#define	morg 		(mevent + 1)
#define	mtot 		(MALLOC_LENGTH - 2)
#endif	/* MALLOC_STATS */

	mpools [0] = morg + m_hdrlen;

#if	MALLOC_ALIGN4
	if (((word)(mpools [0]) & 0x2)) {
		// Make sure the chunk origin is doubleword aligned
		mpools [0]++;
		m_size (mpools [0]) = mtot - m_hdrlen - 1;
	} else {
		m_size (mpools [0]) = mtot - m_hdrlen;
	}
#else
	m_size (mpools [0]) = mtot - m_hdrlen;

#endif	/* MALLOC_ALIGN4 */

	MEV_NWAIT (0) = 0;

#if	MALLOC_STATS
	mnfree [0] = mcfree [0] = m_size (mpools [0]);
#endif
	m_setnextp (mpools [0], NULL);
#if	MALLOC_SAFE
	m_magic (mpools [0]) = 0xdeaf;
#endif

#undef	morg
#undef	mtot

#else	/* MALLOC_SINGLEPOOL */

#if	MALLOC_STATS
	mnfree = mevent + np;
	mcfree = mnfree + np;
	morg = mcfree + np;
	mtot = MALLOC_LENGTH - 4 * np;
#else	/* MALLOC_STATS */
	morg = mevent + np;
	mtot = MALLOC_LENGTH - 2 * np;
#endif	/* MALLOC_STATS */

#if	MALLOC_ALIGN4
	if (((word)morg & 0x2) == 0) {
		// Make sure the chunk origin is doubleword aligned
		morg++;
		mtot--;
	}
#endif
	perc = mtot;

	while (np) {
		np--;
		mpools [np] = freelist = morg + m_hdrlen;
		if (np) {
			/* Do it avoiding long division */
			ceil = MAX_UINT / zzz_heap [np];
			for (chunk = mtot, fac = 0; chunk > ceil;
				chunk >>= 1, fac++);
			chunk = (chunk * zzz_heap [np]) / 100;
			chunk <<= fac;
			// This required __div/__mul from the library:
			// chunk = (word)((((long) zzz_heap [np]) * mtot) /100);
		} else {
			chunk = perc;
		}

#if	MALLOC_ALIGN4
		if ((chunk & 01))
			// Make sure it is even, so the allocatable chunk size
			// is odd
			chunk--;
#endif	/* MALOC_ALIGN4 */

		if (chunk < 128)
			syserror (ERESOURCE, "malloc_init (2)");

		m_size (freelist) =
#if	MALLOC_STATS
			mnfree [np] = mcfree [np] =
#endif
				chunk - m_hdrlen;
		MEV_NWAIT (np) = 0;
		m_setnextp (freelist, NULL);
#if	MALLOC_SAFE
		m_magic (freelist) = 0xdeaf;
#endif
		morg += chunk;
		/* Whatever remains for grabs */
		perc -= chunk;
	}
#endif	/* MALLOC_SINGLEPOOL */
#undef	MA_NP
}

#if	MALLOC_SINGLEPOOL
static void qfree (address ch) {
#define	MA_NP	0
#else
static void qfree (int np, address ch) {
#define	MA_NP	np
#endif

	address chunk, cc;

	cc = (address)(mpools + MA_NP);
	for (chunk = mpools [MA_NP]; chunk != NULL; chunk = m_nextp (chunk)) {
		if (chunk + m_size (chunk) + m_hdrlen == ch) {
			/* Merge at the front */
			m_setnextp (cc, m_nextp (chunk));
			m_size (chunk) += m_hdrlen + m_size (ch);
			ch = chunk;
		} else if (ch + m_size (ch) + m_hdrlen == chunk) {
			/* Merge at the back */
			m_setnextp (cc, m_nextp (chunk));
			m_size (ch) += m_hdrlen + m_size (chunk);
		} else {
			/* Skip */
			cc = chunk;
		}
	}

	/* Insert */
	cc = (address)(mpools + MA_NP);
	for (chunk = mpools [MA_NP]; chunk != NULL; cc = chunk,
		chunk = m_nextp (chunk))
			if (m_size (chunk) >= m_size (ch))
				break;

	m_setnextp (ch, chunk);
	m_setnextp (cc, ch);

#if	MALLOC_SAFE
	m_magic (ch) = 0xdeaf;
#endif

#undef	MA_NP
}

#if	MALLOC_SINGLEPOOL
void zzz_free (address ch) {
#define	MA_NP	0
#define	QFREE	qfree (ch)
#else
void zzz_free (int np, address ch) {
#define	MA_NP	np
#define	QFREE	qfree (np, ch)
#endif
/*
 * User-visible free
 */
	if (ch == NULL)
		/* free (NULL) is legal */
		return;

#if	MALLOC_SAFE
	if ((m_size (ch) & 0x8000) == 0)
		syserror (EMALLOC, "freeing garbage");

	m_size (ch) &= 0x7fff;
#endif

#if	MALLOC_STATS
	mcfree [MA_NP] += m_size (ch);
#endif
	QFREE;

	if (MEV_NWAIT (MA_NP)) {
		trigger ((word)(&(mevent [MA_NP])));
		MEV_NWAIT (MA_NP) --;
	}

#undef	QFREE
#undef	MA_NP

}

#if	MALLOC_SINGLEPOOL
address zzz_malloc (word size) {
#define	MA_NP	0
#define	QFREE	qfree (cc);
#else
address zzz_malloc (int np, word size) {
#define	QFREE	qfree (np, cc);
#define	MA_NP	np
#endif
	address chunk, cc;
	word waste;

	/* Put a limit on how many bytes you can formally request */
	if (size > 0x8000)
		syserror (EREQPAR, "malloc");

#if	MALLOC_ALIGN4
	if (size < 6) {
		size = 3;
	} else {
		size = (size + 1) >> 1;
		if ((size & 1) == 0)
			// Make this an odd number of words, such that the next
			// chunk will fall on an odd-word boundary
			size++;
	}
#else
	if (size < 4)
		/* Never allocate less than two words */
		size = 2;
	else
		/* Convert size to words */
		size = (size + 1) >> 1;
	/* Right shift of unsigned doesn't propagate the sign bit */
#endif	/* MALLOC_ALIGN4 */

	cc = (address)(mpools + MA_NP);
	for (chunk = mpools [MA_NP]; chunk != NULL; cc = chunk,
		chunk = m_nextp (chunk)) {
#if	MALLOC_SAFE
			if (m_magic (chunk) != 0xdeaf)
				syserror (EMALLOC, "inconsistency");
#endif
			if (m_size (chunk) >= size)
				break;
	}
	if (chunk) {
		/* We've got a chunk - remove it from the list */
		m_setnextp (cc, m_nextp (chunk));
		/* Is the waste acceptable ? */
		if ((waste = m_size (chunk) - size) > MAX_MALLOC_WASTE) {
			/* Split the chunk */
			m_size (chunk) = size;
			cc = chunk + size + m_hdrlen;
			m_size (cc) = waste - m_hdrlen;
			/* Insert the chunk */
			QFREE;
		}
		// Erase the failure counter
		MEV_NFAIL (MA_NP) = 0;
#if	MALLOC_STATS
		mcfree [MA_NP] -= m_size (chunk);
		if (mnfree [MA_NP] > mcfree [MA_NP])
			/* Update the minimum */
			mnfree [MA_NP] = mcfree [MA_NP];
#endif
#if	MALLOC_SAFE
		m_size (chunk) |= 0x8000;
#endif
	} else {
		// Failure
		if (MEV_NFAIL (MA_NP) != 255)
			MEV_NFAIL (MA_NP) ++;
#if	RESET_ON_MALLOC
		else {
#if	RESET_ON_SYSERR
			syserror (EWATCH, "malloc watch");
#else
			diag ("MALLOC STALL, RESETTING");
			reset ();
#endif	/* RESET_ON_SYSERR */
		}
#endif	/* RESET_ON_MALLOC */
		
#if	MALLOC_STATS
		mnfree [MA_NP] = 0;
#endif
	}

	return chunk;

#undef	QFREE
#undef	MA_NP

}

#if	MALLOC_SINGLEPOOL
void zzz_waitmem (word state) {
#define	MA_NP	0
#else
void zzz_waitmem (int np, word state) {
#define	MA_NP	np
#endif
/*
 * Wait for pool memory release
 */
	if (MEV_NWAIT (MA_NP) < MAX_TASKS)
		MEV_NWAIT (MA_NP) ++;

	wait ((word)(&(mevent [MA_NP])), state);
#undef	MA_NP
}

#if	MALLOC_STATS

#if	MALLOC_SINGLEPOOL
word	zzz_memfree (address min) {
#define	MA_NP	0
#else
word	zzz_memfree (int np, address min) {
#define	MA_NP	np
#endif
/*
 * Returns memory statistics
 */
	if (min != NULL)
		*min = mnfree [MA_NP];
	return mcfree [MA_NP];
#undef	MA_NP
}

#if	MALLOC_SINGLEPOOL
word	zzz_maxfree (address nc) {
#define	MA_NP	0
#else
word	zzz_maxfree (int np, address nc) {
#define	MA_NP	np
#endif
/*
 * Returns the maximum available chunk size (in words)
 */
	address chunk;
	word max, nchk;

	for (max = nchk = 0, chunk = mpools [MA_NP]; chunk != NULL;
	    chunk = m_nextp (chunk), nchk++)
		if (m_size (chunk) > max)
			max = m_size (chunk);
	if (nc != NULL)
		*nc = nchk;

	return max;
#undef	MA_NP
}

#endif /* MALLOC_STATS */

#if	RESET_ON_MALLOC

int zz_malloc_high_mark () {
/*
 * Checks if any of the pools has reached the failure limit
 */

#if	MALLOC_SINGLEPOOL

	return (MEV_NFAIL (0) == 255);
#else
	word i, j;

	for (i = j = 0; i < 100; j++) {
		if (MEV_NFAIL (j))
			return 1;
		i += zzz_heap [j];
	}
	return 0;

#endif	/* MALLOC_SINGLEPOOL */
}

#endif	/* RESET_ON_MALLOC */
	
#if	SDRAM_PRESENT
void zz_sdram_test (void) {

	unsigned int loc, err;

#if ! ECOG_SIM

	for (loc = 0; loc != ((word)1 << SDRAM_SIZE); loc++)
		*((word*)SDRAM_ADDR + loc) = 1 - loc;
	err = 0;
	for (loc = 0; loc != ((word)1 << SDRAM_SIZE); loc++)
		if (*((word*)SDRAM_ADDR + loc) != 1 - loc)
			err++;
#else
	err = 0;
	loc = ((word)1 << SDRAM_SIZE);
#endif

	if (err)
		diag ("SDRAM failure, 0x%x words, %u errors", loc, err);
	else
		diag ("SDRAM block OK, 0x%x words", loc);
}
#endif

#if	dbg_level != 0

void zz_dbg (const word lvl, word code) {

	byte is;

#if	dbg_binary

	diag_disable_int (a, is);

	diag_wait (a); 	diag_wchar (0         , a);
	diag_wait (a); 	diag_wchar (4         , a);
	diag_wait (a); 	diag_wchar (0xFD      , a);
	diag_wait (a); 	diag_wchar (lvl       , a);
	diag_wait (a); 	diag_wchar (code >> 8 , a);
	diag_wait (a); 	diag_wchar (code      , a);
	diag_wait (a); 	diag_wchar (4         , a);
#else
	int i; word v;

	diag_disable_int (a, is);
	diag_wait (a); 	diag_wchar ('+'       , a);
	diag_wait (a); 	diag_wchar ('+'       , a);
	diag_wait (a);
	if (lvl < 10) {
			diag_wchar ('0' + lvl , a);
	} else {
			diag_wchar ('a' + lvl - 10, a);
	}
	diag_wait (a); 	diag_wchar ('.'       , a);

	for (i = 0; i < 16; i += 4) {
		v = (code >> 12 - i) & 0xf;
		if (v > 9)
			v = (word)'a' + v - 10;
		else
			v = (word)'0' + v;
		diag_wait (a);
		diag_wchar (v, a);
	}
	
	diag_wait (a); diag_wchar ('\r'       , a);
	diag_wait (a); diag_wchar ('\n'       , a);
#endif
	diag_wait (a);
	diag_enable_int (a, is);
}

#endif	/* dbg_level */

#if	DIAG_MESSAGES

void diag (const char *mess, ...) {
/* ================================ */
/* Writes a direct message to UART0 */
/* ================================ */

	va_list	ap;
	word i, val, v;
	char *s;
	byte is;

	va_start (ap, mess);
	diag_disable_int (a, is);

	while  (*mess != '\0') {
		if (*mess == '%') {
			mess++;
			switch (*mess) {
			  case 'x' :
				val = va_arg (ap, word);
				for (i = 0; i < 16; i += 4) {
					v = (word) zz_hex_enc_table [
							(val >> (12 - i)) & 0xf
								    ];
					diag_wait (a);
					diag_wchar (v, a);
				}
				break;
			  case 'd' :
				val = va_arg (ap, word);
				if (val & 0x8000) {
					diag_wait (a);
					diag_wchar ('-', a);
					val = (~val) + 1;
				}
			    DI_SIG:
				i = 10000;
				while (1) {
					v = val / i;
					if (v || i == 1) break;
					i /= 10;
				}
				while (1) {
					diag_wait (a);
					diag_wchar (v + '0', a);
					val = val - (v * i);
					i /= 10;
					if (i == 0) break;
					v = val / i;
				}
				break;
			  case 'u' :
				val = va_arg (ap, word);
				goto DI_SIG;
			  case 's' :
				s = va_arg (ap, char*);
				while (*s != '\0') {
					diag_wait (a);
					diag_wchar (*s, a);
					s++;
				}
				break;
			  default:
				diag_wait (a);
				diag_wchar ('%', a);
				diag_wait (a);
				diag_wchar (*mess, a);
			}
			mess++;
		} else {
			diag_wait (a);
			diag_wchar (*mess++, a);
		}
	}

	diag_wait (a);
	diag_wchar ('\r', a);
	diag_wait (a);
	diag_wchar ('\n', a);
	diag_wait (a);

	diag_enable_int (a, is);
}

#else

void diag (const char *mess, ...) { }

#endif	/* DIAG_MESSAGES */

#ifdef	DEBUG_BUFFER

word	debug_buffer_pointer = 0;
word	debug_buffer [DEBUG_BUFFER];

void dbb (word d) {

	if (debug_buffer_pointer == DEBUG_BUFFER)
		debug_buffer_pointer = 0;

	debug_buffer [debug_buffer_pointer++] = d;
	debug_buffer [debug_buffer_pointer == DEBUG_BUFFER ? 0 :
		debug_buffer_pointer] = 0xdead;
}

#endif

#ifdef	DUMP_PCB
void dpcb (pcb_t *p) {

	diag ("PR %x: S%x T%x E (%x %x) (%x %x) (%x %x)", (word) p,
		p->Status, p->Timer,
		p->Events [0] . Status,
		p->Events [0] . Event,
		p->Events [1] . Status,
		p->Events [1] . Event,
		p->Events [2] . Status,
		p->Events [2] . Event);
}
#else
#define dpcb(a)
#endif

#if	DUMP_MEMORY

void dmp_mem () {

	byte *addr, is;
	int i;

	diag ("FULL DUMP OF RAM:");

	diag_disable_int (a, is);

	addr = (byte*) RAM_START;

	while (addr != (byte*) RAM_END) {
		if ((((word)addr) & 0xF) == 0) {
			// New line
			diag_wait (a); diag_wchar ('\r', a);
			diag_wait (a); diag_wchar ('\n', a);
			for (i = 12; i >= 0; i -= 4) {
				diag_wait (a);
				diag_wchar (zz_hex_enc_table [(((word)addr) >>
					i) & 0xf], a);
			}
			diag_wait (a); diag_wchar (':', a);
		}
		diag_wait (a); diag_wchar (' ', a);
		diag_wait (a); diag_wchar (' ', a);
		diag_wait (a); diag_wchar (
			zz_hex_enc_table [((*addr) >> 4) & 0xf], a);
		diag_wait (a); diag_wchar (
			zz_hex_enc_table [((*addr)     ) & 0xf], a);
		addr++;
	}
	diag_wait (a); diag_wchar ('\r', a);
	diag_wait (a); diag_wchar ('\n', a);

	diag_enable_int (a, is);

	diag ("\r\nEND OF DUMP");
}

#endif

/* --------------------------------------------------------- */
/* ====== end of MEMORY ALLOCATORS ========================= */
/* --------------------------------------------------------- */
