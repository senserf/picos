/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2017                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "tcv.h"

//
// Put all lword variables here to avoid losing memory on alignment
//

#if	ENTROPY_COLLECTION
lword	entropy;
#endif

#if	RANDOM_NUMBER_GENERATOR > 1
lword	__pi_seed = 327672838L;
#else
word	__pi_seed = 30011;
#endif

#include "pins.h"

/* Task table */
#if MAX_TASKS <= 0
__pi_pcb_t *__PCB = NULL;
#else
__pi_pcb_t __PCB [MAX_TASKS];
#endif

#if MAX_DEVICES
/* Device table */
static devreqfun_t ioreq [MAX_DEVICES];
#endif

/* System status word */
volatile systat_t __pi_systat;

/* =============== */
/* Current process */
/* =============== */
__pi_pcb_t	*__pi_curr;

/* ========= */
/* The clock */
/* ========= */
#if TRIPLE_CLOCK == 0
static		word	millisec;
#endif

word		__pi_mintk;
volatile word	__pi_old, __pi_new;

/* ================================ */
/* User-defineable countdown timers */
/* ================================ */
address		__pi_utims [MAX_UTIMERS];

#if TCV_PRESENT && TCV_TIMERS
/* ============================= */
/* Queue for fired packet timers */
/* ============================= */
titem_t		*__pi_tcv_ftimers;
#endif

const char	__pi_hex_enc_table [] = {
				'0', '1', '2', '3', '4', '5', '6', '7',
				'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
			      };

const lword	host_id = 0xBACADEAD;

// Memory events

typedef	struct {
	byte	nwait;	// Number of processes waiting for the pool
	byte	nfail;	// Number of consecutive allocation failures
} mevent_t;

static	mevent_t mevent [MALLOC_NPOOLS];

// PID verifier ===============================================================

#ifndef	PID_VER_TYPE
#define	PID_VER_TYPE	1
#endif

// None =======================================================================

#if PID_VER_TYPE == 0
#define	ver_pid(i,pid)	((i) = (__pi_pcb_t*)(pid))
#endif

// Function ===================================================================

#if PID_VER_TYPE == 1
static __pi_pcb_t *pidver (aword pid) {

	register __pi_pcb_t *i;

	for_all_tasks (i)
		if ((aword)(i) == pid)
			return i;
	syserror (EREQPAR, "pid");
}

#define	ver_pid(i,pid)	((i) = pidver (pid))
#endif

// Macro ======================================================================

#if PID_VER_TYPE == 2
#define ver_pid(i,pid)	do { \
				for_all_tasks (i) \
					if ((int)(i) == pid) \
						break; \
				if (pcb_not_found (i)) \
					syserror (EREQPAR, "pid"); \
			} while (0)
#endif

// ============================================================================
// ============================================================================

void __pi_badstate (void) {
	syserror (ESTATE, "state");
}

void utimer_add (address ut) {
//
// Add a utimer
//
	int i;

	for (i = 0; i < MAX_UTIMERS; i++)
		if (__pi_utims [i] == NULL)
			break;

	if (i == MAX_UTIMERS)
		syserror (ERESOURCE, "ut a");

	__pi_utims [i] = ut;
}

void utimer_delete (address ut) {
//
// Remove a utimer
//
	int i;

	for (i = 0; i < MAX_UTIMERS; i++) {
		if (__pi_utims [i] == 0)
			syserror (EREQPAR, "ut d");
		if (__pi_utims [i] == ut)
			break;
	}
	cli_utims;
	while (i < MAX_UTIMERS-1) {
		__pi_utims [i] = __pi_utims [i+1];
		if (__pi_utims [i] == NULL)
			break;
		i++;
	}
	sti_utims;
}

#if TRIPLE_CLOCK

void __pi_utimer_set (address t, word v) {

	*t = v;
	TCI_RUN_AUXILIARY_TIMER;
}

#endif

void update_n_wake (word min, Boolean force) {
//
// The idea is this. There are two circular clock pointers wrapping around at
// 64K (PicOS) milliseconds, i.e., at 64 seconds. One of them is called new
// (or __pi_new), the other is old (__pi_old). The new pointer is updated by
// the timer (interrupt) by the number of milliseconds elapsed. Depending on
// the clock mode, this update can happen at regular intervals (typically by
// one every millisecond), or at longer intervals (in TRIPLE_CLOCK mode). The
// old pointer is only advanced by this function. The idea is that old
// represents the moment we last looked at the timer. The difference
// (accounting for the wraparound) between the two pointers is the lag that we
// are supposed to cover in the current step. Any delay requests falling into
// the lag interval are fired.
//
	__pi_pcb_t *i;
	word d;

// ============================================================================
#if TRIPLE_CLOCK == 0
	// Delay timer interrupts are asynchronous, i.e., they are not blocked
	// while we are doing this, unlike in the TRIPLE_CLOCK mode. We copy
	// the current new (to make sure it stays put why we are handling it),
	// but there is no need to block interrupts. The interrupt advances
	// new (we only read it), we advance old (the interrupt only reads it).
	word znew;
	znew = __pi_new;
#else

#define	znew __pi_new
	// This will collect any ticks accumulated since the timer was started
	// (i.e., bring new up to date) and make sure the timer is stopped now
	if (TCI_UPDATE_DELAY_TICKS (force))
		// Timer is running and we are not doing a new delay request;
		// so just let it continue
		return;
#endif
// ============================================================================

	// See kernel.h for the wake condition; it basically says that mintk
	// falls within the lag interval
	if (twakecnd (__pi_old, znew, __pi_mintk)) {

		// As now all __pi_mintk values are legit (e.g., 0 cannot be
		// special), the condition will occasionally force us to do
		// an idle scan through the list of processes. This will do no
		// harm, or at least less harm than having an extra flag.

		for_all_tasks (i) {

			if (!twaiting (i))
				continue;

			if (twakecnd (__pi_old, znew, i->Timer)) {
				// Wake up
				wakeuptm (i);
			} else {
				// Waiting on the timer but not for wakeup yet,
				// calculate new minimum
				d = i->Timer - znew;
				if (d < min)
					min = d;
			}
		}
#if TCV_PRESENT && TCV_TIMERS
		// Identify the packet timers that has gone off. This function
		// picks them up and puts into a special queue, pointed to by
		// __pi_tcv_ftimers (in kernel.c). Then, __pi_tcv_execqueue
		// (see below) will actually fire them. We cannot fire them
		// right away (as we pick them), because the firing involves
		// calling plugin functions, which are not unlikely to issue
		// tcvp_settimer requests, which in turn will call us (i.e.,
		// update_n_wake) recursively (which would be bad at this
		// stage). So we have to do it in two steps. Note that nothing
		// really happens between these two steps, except that want to
		// separate them by the code that separates this point from the
		// call to __pi_tcv_execqueue below.
		__pi_tcv_runqueue (znew, &min);
#endif
	} else {
		// Nobody is eligible for wakeup, the old minimum holds, unless
		// the requested one is less
		if (__pi_mintk - znew < min) 
			goto MOK;
	}

	__pi_mintk = znew + min;

MOK:

#if TRIPLE_CLOCK == 0
	// Handle the seconds clock; note that with TRIPLE_CLOCK == 1, the
	// seconds clock is handled by an interrupt
	millisec += (znew - __pi_old);
	while (millisec >= JIFFIES) {
		millisec -= JIFFIES;
		__pi_nseconds++;
		check_stack_overflow;
#include "second.h"
	}
#endif
	__pi_old = znew;

	// This is void with TRIPLE_CLOCK == 0 (the hardware timer never stops
	// running); otherwise, it starts the hardware timer
	TCI_RUN_DELAY_TIMER;

#if TCV_PRESENT && TCV_TIMERS
	// Fire the expired packet timers. We can (must) do it as the last
	// statement in update_n_wake, so a possible recursive call from a
	// plugin will effectively amount to a separate (safe) call.
	__pi_tcv_execqueue ();
#endif

#if TRIPLE_CLOCK == 0
#undef	znew
#endif

}

/* ==================== */
/* Launch a new process */
/* ==================== */
aword __pi_fork (fsmcode func, aword data) {

	__pi_pcb_t *i;

#if MAX_TASKS <= 0
//
// Linked PCBT, the PCB is malloc'ed
//
	if ((i = (__pi_pcb_t*) umalloc (sizeof (__pi_pcb_t))) == NULL)
		return 0;

	i->code = func;
	i->data = data;
	i->Status = 0;

#if MAX_TASKS == 0
//
// The PCB is appended at the end
//
	i->Next = NULL;
	{
		__pi_pcb_t *j;
		if ((j = __PCB) == NULL)
			__PCB = i;
		else {
			for (; j->Next != NULL; j = j->Next);
			j->Next = i;
		}
	}
#else
//
// The PCB is appended at the front
//
	i->Next = __PCB;
	// Note: no race with interrupt trigger/ptrigger
	__PCB = i;
#endif
	return (aword) i;
#else
//
// Static PCBT
//
	for_all_tasks (i)
		if (i->code == NULL) {
			i -> code = func;
			i -> data = data;
			i -> Status = 0;
			return (aword) i;
		}
	return 0;
#endif
}

/* ======================== */
/* Proceed to another state */
/* ======================== */
void proceed (word state) {

	prcdstate (__pi_curr, state);
	release;
}

void __pi_wait (aword event, word state) {
//
// The unified wait operation
//
	sint j = nevents (__pi_curr);

	if (j >= MAX_EVENTS_PER_TASK)
		syserror (ENEVENTS, "sw");

	setestate (__pi_curr->Events [j], state, event);
	incwait (__pi_curr);
}

void __pi_trigger (aword event) {
//
// The unified trigger operation
//
	int j;
	__pi_pcb_t *i;

	for_all_tasks (i) {
#if MAX_TASKS > 0
//
// With linked PCBT, this is never NULL
//
		if (i->code == NULL)
			continue;
#endif
		for (j = 0; j < nevents (i); j++) {
			if (i->Events [j] . Event == event) {
				/* Wake up */
				wakeupev (i, j);
				break;
			}
		}
	}
}

void __pi_ptrigger (aword pid, aword event) {
//
// Trigger for a single process
//
	__pi_pcb_t	*i;
	int 	j;

	ver_pid (i, pid);

#if MAX_TASKS > 0
	if (i->code == NULL)
		return;
#endif
	for (j = 0; j < nevents (i); j++) {
		if (i->Events [j] . Event == event) {
			wakeupev (i, j);
		}
	} 
}

void __pi_fork_join_release (fsmcode func, aword data, word st) {

	aword pid;

	if ((pid = __pi_fork (func, data)) == 0)
		return;

	__pi_wait (pid, st);
	release;
}

/* ========== */
/* Timer wait */
/* ========== */
void delay (word d, word state) {

	// Note that this also removes the timer wait bit, if set, so
	// update_n_wake won't bother us
	settstate (__pi_curr, state);

	// Catch up with time
	update_n_wake (d, YES);

	__pi_curr->Timer = __pi_old + d;

	inctimer (__pi_curr);
}

void unwait (void) {
/* =========================================== */
/* Cancels all wait requests including a delay */
/* =========================================== */
	wakeuptm (__pi_curr);
}

/* ======================================================================== */
/* Return the number of milliseconds that the indicated process is going to */
/* sleep for                                                                */
/* ======================================================================== */
word dleft (aword pid) {

	__pi_pcb_t *i;

	update_n_wake (MAX_WORD, YES);

	if (pid == 0)
		i = __pi_curr;
	else {
		ver_pid (i, pid);
		if (i->code == NULL || !twaiting (i))
			return MAX_WORD;
	}

	return i->Timer - __pi_old;
}

static void killev (__pi_pcb_t *pid) {
//
// Deliver events after killing a process
//
	aword wfun;
	int j;
	__pi_pcb_t *i;

	wfun = (aword)(pid->code);
	for_all_tasks (i) {
#if MAX_TASKS > 0
		if (i->code == NULL)
			continue;
#endif
		for (j = 0; j < nevents (i); j++) {
			if (i->Events [j] . Event == (aword)pid
			    || i->Events [j] . Event == wfun
#if MAX_TASKS > 0
// Also take care of those waiting for a free slot, but only when MAX_TASKS is
// in effect, as otherwise ufree will trigger the memory event when the PCB is
// freed
			    || i->Events [j] . Event == (aword)(&(mevent [0]))
#endif
			    ) {
				wakeupev (i, j);
				break;
			}
		}
	}
}

void kill (aword pid) {
//
// Terminate the process
//
	__pi_pcb_t *i;

#if MAX_TASKS <= 0
//
// Linked PCBT
//
	__pi_pcb_t *j;

	if (pid == 0)
		pid = (aword) __pi_curr;

	j = NULL;
	for_all_tasks (i) {
		if ((aword)i == pid) {
			// Found it
			if (j == NULL)
				__PCB = i->Next;
			else
				j->Next = i->Next;
			killev (i);
			ufree (i);
			if (i == __pi_curr)
				release;
			return;
		}
		j = i;
	}
	syserror (EREQPAR, "kpi");

#else
//
// Static PCBT
//
	if (pid == 0)
		i = __pi_curr;
	else
		ver_pid (i, pid);

	if (i->code != NULL) {
		killev (i);
		i->Status = 0;
		i->code = NULL;
	}

	if (i == __pi_curr)
		release;
#endif
}

void killall (fsmcode fun) {
//
// Kill all processes running the given code
//
	Boolean rel;
	__pi_pcb_t *i;

#if MAX_TASKS <= 0
//
// Linked PCBT
//
	__pi_pcb_t *j, *k;

	rel = NO;
	j = NULL;
	for (i = __PCB; i != NULL; ) {
		if (i->code == fun) {
			k = i->Next;
			if (j == NULL)
				__PCB = k;
			else
				j->Next = k;
			if (i == __pi_curr)
				rel = YES;
			killev (i);
			ufree (i);
			i = k;
		} else {
			i = (j = i)->Next;
		}
	}
#else
//
// Static PCBT
//
	rel = NO;
	for_all_tasks (i) {
		if (i->code == fun) {
			if (i == __pi_curr)
				rel = YES;
			killev (i);
			i->Status = 0;
			i->code = NULL;
		}
	}
#endif
	if (rel)
		release;
}

fsmcode getcode (aword pid) {

	__pi_pcb_t *i;

	if (pid == 0)
		i = __pi_curr;
	else
		ver_pid (i, pid);

	return i->code;
}

aword running (fsmcode fun) {

	__pi_pcb_t *i;

	if (fun == NULL)
		return (int) __pi_curr;

	for_all_tasks (i)
		if (i->code == fun)
			return (int) i;

	return 0;
}

word crunning (fsmcode fun) {
//
	__pi_pcb_t *i;
	word c;

	c = 0;
	for_all_tasks (i)
		if (i->code == fun)
			c++;
	return c;
}

int __pi_strlen (const char *s) {

	int i;

	for (i = 0; *(s+i) != '\0'; i++);

	return i;
}

void __pi_strcpy (char *d, const char *s) {

	while ((Boolean)(*d++ = *s++));
}

void __pi_strncpy (char *d, const char *s, int n) {

	while (n-- && (*s != '\0'))
		*d++ = *s++;
	*d = '\0';
}

void __pi_strcat (char *d, const char *s) {

	while (*d != '\0') d++;
	strcpy (d, s);
}

void __pi_strncat (char *d, const char *s, int n) {

	strcpy (d+n, s);
}

void __pi_memcpy (char *dest, const char *src, int n) {

	while (n--)
		*dest++ = *src++;
}

void __pi_memset (char *dest, char c, int n) {

	while (n--)
		*dest++ = c;
}

#if MAX_DEVICES
/* ============================================= */
/* Configures a driver-specific service function */
/* ============================================= */
void adddevfunc (devreqfun_t rf, int loc) {

	if (loc < 0 || loc >= MAX_DEVICES)
		syserror (EREQPAR, "addv");

	if (ioreq [loc] != NULL)
		syserror (ERESOURCE, "addv");

	ioreq [loc] = rf;
}
#endif	/* MAX_DEVICES */

#if MAX_DEVICES
/* ==================================== */
/* User-visible i/o request distributor */
/* ==================================== */
sint io (word retry, word dev, word operation, char *buf, word len) {

	sint ret;

	if (dev >= MAX_DEVICES || ioreq [dev] == NULL)
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
		delay (-ret - 2, retry);
		release;
	}

	return 0;
}

#endif	/* MAX_DEVICES */

/* --------------------------------------------------------------------- */
/* ========================= MEMORY ALLOCATORS ========================= */
/* --------------------------------------------------------------------- */

// The application can request less than 64K for a single chunk, even though
// the total amount of allocatable memory can be more than that

#if	MALLOC_NPOOLS > 1
// Pool size percentages
extern	byte __pi_heap [];
#endif

// These are pool headers, i.e., pointers to awords
static	aword	*mpools [MALLOC_NPOOLS];

#if	MALLOC_STATS
// Pointers to awords storing the statistics; the statistics are kept exact,
// but the returned values are maxed at word size
static	aword	mnfree [MALLOC_NPOOLS], mcfree [MALLOC_NPOOLS];
#endif

#define	m_nextp(c)	((aword*)(*(c)))
#define m_setnextp(c,p)	(*(c) = (aword)(p))
#define m_size(c)	(*((c)-1))
#define	m_magic(c) 	(*((c)+1))
#define	m_hdrlen	1			// in awords

#if	SIZE_OF_AWORD > 2
// Corruption guard (for MALLOC_SAFE)
#define	MALLOC_MAGIC		0xDEAFDEAF
#define	MALLOC_UMASK		0x80000000
#define	MALLOC_PCEIL		0xffffffff	// MAX_AWORD
#define	MAX_MALLOC_WASTE	8
#define	topword(w)		((w) > MAX_WORD ? MAX_WORD : (word)(w))
#else
#define	MALLOC_MAGIC		0xDEAF
#define	MALLOC_UMASK		0x8000
#define	MALLOC_PCEIL		0xffff
#define	MAX_MALLOC_WASTE	12
#define	topword(w)		(w)
#endif

#if 0

void dump_malloc (const char *s) {

	aword tot, nc;
	aword *ch;
	sint i;

	diag (s);

	for (i = 0; i < MALLOC_NPOOLS; i++) {
		tot = 0;
		for (nc = 0, ch = mpools [i]; ch != NULL; ch = m_nextp (ch)) {
			tot += m_size (ch);
			nc++;
		}
		diag ("POOL %d, N " __ufaw ", F " __ufaw, i, nc, tot);
	}

#undef	NPOOLS

}

#endif	/* 0 */

void __pi_malloc_init () {
/* =================================== */
/* Initializes memory allocation pools */
/* =================================== */

	aword	mlen;

#if MALLOC_NPOOLS > 1

	aword	*morg, *freelist, ceil, chunk;
	sint	perc, np, fac;

	for (perc = np = 0; np < MALLOC_NPOOLS; np++) {
		// This assumes that the percentage adds to 100, otherwise
		// we will hit an error
		if (__pi_heap [np] == 0)
			syserror (ESYSPAR, "mal1");
		perc += __pi_heap [np];
	}

	if (perc != 100)
		// Make sure the percentages add up to 100
		syserror (ESYSPAR, "mal2");
#endif
	if ((mlen = (aword)MALLOC_LENGTH) < 256)
		// Make sure we do have some memory available
		syserror (ERESOURCE, "mal1");

#if MALLOC_NPOOLS == 1

	// ====================================================================
	// The single-pool case
	// ====================================================================

	mpools [0] = ((aword*)MALLOC_START) + m_hdrlen;
	// The size is in awords
	m_size (mpools [0]) = mlen - m_hdrlen;
	m_setnextp (mpools [0], NULL);
#if	MALLOC_SAFE
	m_magic (mpools [0]) = MALLOC_MAGIC;
#endif
#if	MALLOC_STATS
	mnfree [0] = mcfree [0] = m_size (mpools [0]);
#endif

#else	// MALLOC_NPOOLS > 1

	// ====================================================================
	// Multiple pools
	// ====================================================================

	morg = (aword*)MALLOC_START;
	perc = mlen;

	while (np) {
		np--;
		mpools [np] = freelist = morg + m_hdrlen;
		if (np) {
			// Do it in a way avoiding tricky division
			ceil = MALLOC_PCEIL / (aword)(__pi_heap [np]);
			for (chunk = mlen, fac = 0; chunk > ceil;
				chunk >>= 1, fac++);
			chunk = (chunk * (aword)__pi_heap [np]) / 100;
			chunk <<= fac;
		} else {
			chunk = perc;
		}

		if (chunk < 64)
			syserror (ERESOURCE, "mal2");

		m_size (freelist) =
#if	MALLOC_STATS
			mnfree [np] = mcfree [np] =
#endif
				chunk - m_hdrlen;
		m_setnextp (freelist, NULL);
#if	MALLOC_SAFE
		m_magic (freelist) = MALLOC_MAGIC;
#endif
		morg += chunk;
		/* Whatever remains for grabs */
		perc -= chunk;
	}
#endif	// MALLOC_NPOOLS

}

#if	MALLOC_NPOOLS == 1
static void qfree (aword *ch) {
#define	MA_NP	0
#else
static void qfree (int np, aword *ch) {
#define	MA_NP	np
#endif

#if	0
	// Trying to circumvent a bug in TI MSPGCC
	// Not needed: -fno-strict-aliasing does take care of it, and is
	// safer for other potential cases of "unsafe optimization"
	// volatile
#endif
	aword *chunk, *cc;

	cc = (aword*)(mpools + MA_NP);
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
	cc = (aword*)(mpools + MA_NP);
	for (chunk = mpools [MA_NP]; chunk != NULL; cc = chunk,
		chunk = m_nextp (chunk))
			if (m_size (chunk) >= m_size (ch))
				break;

	m_setnextp (ch, chunk);
	m_setnextp (cc, ch);

#if	MALLOC_SAFE
	m_magic (ch) = MALLOC_MAGIC;
#endif

#undef	MA_NP
}

#if	MALLOC_NPOOLS == 1
void __pi_free (aword *ch) {
#define	MA_NP	0
#define	QFREE	qfree (ch)
#else
void __pi_free (int np, address ch) {
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
	if ((m_size (ch) & MALLOC_UMASK) == 0)
		syserror (EMALLOC, "malg");
	else
		m_size (ch) ^= MALLOC_UMASK;
#endif

#if	MALLOC_STATS
	mcfree [MA_NP] += m_size (ch);
#endif
	QFREE;

	if (mevent [MA_NP] . nwait) {
		trigger ((aword)(&(mevent [MA_NP])));
		mevent [MA_NP] . nwait --;
	}

#undef	QFREE
#undef	MA_NP

}

#if	MALLOC_NPOOLS == 1
aword *__pi_malloc (word size) {
#define	MA_NP	0
#define	QFREE	qfree (cc);
#else
aword *__pi_malloc (int np, word size) {
#define	QFREE	qfree (np, cc);
#define	MA_NP	np
#endif
	aword	*chunk, *cc, waste;

#if	SIZE_OF_AWORD > 2

	aword	_size;

	if (size < 8)
		// At least two awords
		_size = 2;
	else
		// Convert to awords
		_size = (((aword)size) + 3) >> 2;
#else
	/* Put a limit on how many bytes you can formally request */
	if (size > 0x8000)
		syserror (EREQPAR, "mal");

	if (size < 4)
		// At least two words
		size = 2;
	else
		// Convert to words
		size = (size + 1) >> 1;
#define	_size	size
#endif

	cc = (aword*)(mpools + MA_NP);
	for (chunk = mpools [MA_NP]; chunk != NULL; cc = chunk,
		chunk = m_nextp (chunk)) {
#if	MALLOC_SAFE
			if (m_magic (chunk) != MALLOC_MAGIC)
				syserror (EMALLOC, "malc");
#endif
			if (m_size (chunk) >= _size)
				break;
	}
	if (chunk) {
		/* We've got a chunk - remove it from the list */
		m_setnextp (cc, m_nextp (chunk));
		/* Is the waste acceptable ? */
		if ((waste = m_size (chunk) - _size) > MAX_MALLOC_WASTE) {
			/* Split the chunk */
			m_size (chunk) = _size;
			cc = chunk + _size + m_hdrlen;
			m_size (cc) = waste - m_hdrlen;
			/* Insert the chunk */
			QFREE;
		}
		// Erase the failure counter
		mevent [MA_NP] . nfail = 0;
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
		if (mevent [MA_NP] . nfail != 255)
			mevent [MA_NP] . nfail ++;
#if	RESET_ON_MALLOC
		else {
#if	RESET_ON_SYSERR
			syserror (EWATCH, "malw");
#else
			diag ("MALW");
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
void __pi_waitmem (word state) {
#define	MA_NP	0
#else
void __pi_waitmem (int np, word state) {
#define	MA_NP	np
#endif
/*
 * Wait for pool memory release
 */
	if (mevent [MA_NP] . nwait != 255)
		mevent [MA_NP] . nwait ++;

	wait ((aword)(&(mevent [MA_NP])), state);
#undef	MA_NP
}

#if	MALLOC_STATS

#if	MALLOC_SINGLEPOOL
word	__pi_memfree (address min) {
#define	MA_NP	0
#else
word	__pi_memfree (int np, address min) {
#define	MA_NP	np
#endif
/*
 * Returns memory statistics
 */
	if (min != NULL)
		*min = topword (mnfree [MA_NP]);
	return topword (mcfree [MA_NP]);
#undef	MA_NP
}

#if	MALLOC_SINGLEPOOL
word	__pi_maxfree (address nc) {
#define	MA_NP	0
#else
word	__pi_maxfree (int np, address nc) {
#define	MA_NP	np
#endif
/*
 * Returns the maximum available chunk size (in awords)
 */
	aword *chunk;
	aword max, nchk;

	for (max = nchk = 0, chunk = mpools [MA_NP]; chunk != NULL;
	    chunk = m_nextp (chunk), nchk++)
		if (m_size (chunk) > max)
			max = m_size (chunk);
	if (nc != NULL)
		*nc = topword (nchk);

	return topword (max);
#undef	MA_NP
}

#endif /* MALLOC_STATS */

/* --------------------------------------------------------------------- */
/* ======================= END MEMORY ALLOCATORS ======================= */
/* --------------------------------------------------------------------- */

#if	dbg_level != 0 || DIAG_MESSAGES

static void dgout (word c) {

	diag_wait (a);
	diag_wchar (c, a);
}

#endif

#if	dbg_level != 0

void __pi_dbg (const word lvl, word code) {

	byte is;

#if	dbg_binary

	diag_disable_int (a, is);

	dgout (0         );
	dgout (4         );
	dgout (0xFD      );
	dgout (lvl       );
	dgout (code >> 8 );
	dgout (code      );
	dgout (4         );
#else
	int i; word v;

	diag_disable_int (a, is);
	dgout ('+'       );
	dgout ('+'       );
	if (lvl < 10) {
			dgout ('0' + lvl);
	} else {
			dgout ('a' + lvl - 10);
	}
	dgout ('.');

	for (i = 0; i < 16; i += 4) {
		v = (code >> 12 - i) & 0xf;
		if (v > 9)
			v = (word)'a' + v - 10;
		else
			v = (word)'0' + v;

		dgout (v);
	}
	
	dgout ('\r');
	dgout ('\n');
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

	lword val;
	va_list	ap;
	sint bc;
	word v;
	byte is;

	va_start (ap, mess);
	diag_disable_int (a, is);

	while  (*mess != '\0') {
		if (*mess == '%') {
			mess++;
			if (*mess == 'l') {
				bc = 32;
				val = va_arg (ap, lword);
				mess++;
			} else {
				bc = 16;
				// On a 32-bit system, this must be a 32-bit
				// item regardless of the requested size
				val = va_arg (ap, aword);
			}
			switch (*mess) {
			  case 'x' : {
				sint i = 0;
				while (i < bc) {
					i += 4;
					v = __pi_hex_enc_table [
					(val >> (bc - i)) & 0xf];
					dgout (v);
				}
				break;
			  }
			  case 'd' : {
				lword d;
				if ((val >> (bc - 1)) & 1) {
					dgout ('-');
					val = ~val + 1;
				}
DI_SIG:
				d = (bc == 16) ? 10000 : 1000000000;
				while (1) {
					// Strip initial zeros
					v = (word) (val / d);
					if (v || d == 1) break;
					d /= 10;
				}
				while (1) {
					dgout (v + '0');
					val = val - (v * d);
					d /= 10;
					if (d == 0) break;
					v = (word) (val / d);
				}
				break;
			  }
			  case 'u' :
				goto DI_SIG;
			  case 's' : {
				char *s = (char*)(aword)val;
				while (*s != '\0') {
					dgout (*s);
					s++;
				}
				break;
			  }
			  default:
				dgout ('%');
				dgout (*mess);
			}
			mess++;
		} else {
			dgout (*mess++);
		}
	}

	dgout ('\r');
	dgout ('\n');
	diag_wait (a);
	diag_enable_int (a, is);
}

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
void dpcb (__pi_pcb_t *p) {

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
				diag_wchar (__pi_hex_enc_table [(((word)addr) >>
					i) & 0xf], a);
			}
			diag_wait (a); diag_wchar (':', a);
		}
		diag_wait (a); diag_wchar (' ', a);
		diag_wait (a); diag_wchar (' ', a);
		diag_wait (a); diag_wchar (
			__pi_hex_enc_table [((*addr) >> 4) & 0xf], a);
		diag_wait (a); diag_wchar (
			__pi_hex_enc_table [((*addr)     ) & 0xf], a);
		addr++;
	}
	diag_wait (a); diag_wchar ('\r', a);
	diag_wait (a); diag_wchar ('\n', a);

	diag_enable_int (a, is);

	diag ("\r\nEND OF DUMP");
}

#endif

// ============================================================================
// High-quality RNG (oh, well, as high as we can afford) ======================
// ============================================================================
#if RANDOM_NUMBER_GENERATOR > 1
lword lrnd () {
	__pi_seed = __pi_seed * 1103515245 + 12345;
	return __pi_seed
#if ENTROPY_COLLECTION
	^ entropy
#endif
	;
}
#endif

// ============================================================================
// Low-quality RNG ============================================================
// ============================================================================
#if RANDOM_NUMBER_GENERATOR == 1
word rnd () {
	__pi_seed = __pi_seed * 17981 + 12345;
	return __pi_seed
#if ENTROPY_COLLECTION
		^ (word)entropy
#endif
	;
}
#endif
