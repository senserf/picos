#include "tcv.h"
#include "tcvphys.h"
#include "tcvplug.h"

/* #define	DUMPQUEUES	1 */

/* ============================================================================ */
/*                       PicOS                                                  */
/*                                                                              */
/* The open-ended and open-sided part of the transceiver driver                 */
/*                                                                              */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002--2005                 */
/*                                                                              */
/* Permission is hereby granted, free of charge, to any person obtaining a copy */
/* of this software and associated documentation files (the "Software"), to     */
/* deal in the Software without restriction, including without limitation the   */
/* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  */
/* sell copies of the Software, and to permit persons to whom the Software is   */
/* furnished to do so, subject to the following conditions:                     */
/*                                                                              */
/* The above copyright notice and this permission notice shall be included in   */
/* all copies or substantial portions of the Software.                          */
/*                                                                              */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS */
/* IN THE SOFTWARE.                                                             */
/*                                                                              */
/* ============================================================================ */


#if	TCV_PRESENT

/*
 * Session descriptor pool
 */
static sesdesc_t	*descriptors [TCV_MAX_DESC];

/*
 * Phys interfaces. An interface registers by calling phys_reg (see below) and
 * providing a pointer to the options function, which is stored in this array.
 */
static ctrlfun_t	physical [TCV_MAX_PHYS];

/*
 * Phys output queues, each registered interface gets one dedicated output
 * queue.
 */
static qhead_t		*oqueues [TCV_MAX_PHYS];

/*
 * Physinfo declared when the interface is registered
 */
static int		physinfo [TCV_MAX_PHYS];

/*
 * Plugins. Again, plugins register into this table and are identified by
 * their indexes. This table is normally very small.
 */
tcvplug_t	*zzz_plugins [TCV_MAX_PLUGS];
#define	plugins	zzz_plugins

#if	TCV_TIMERS
/*
 * This is the timer queue, and the time when the timer was last set.
 */
static thead_t	tcv_q_tim = { (titem_t*) &tcv_q_tim, (titem_t*) &tcv_q_tim };
static unsigned long tcv_tim_set = 0;
#endif

static void rlp (hblock_t*);

#ifdef	DUMPQUEUES

void dmpq (qhead_t *q) {
	hblock_t *pp;
	diag ("START Q DUMP %x", (word)q);
	for (pp = q_first (q); !q_end (pp, q); pp = q_next (pp))
		diag ("%d %d %x", pp->length, pp->tqueue.value, pp->attributes);
	diag ("END Q DUMP %x", (word)q);
}

#else

#define	dmpq(a)

#endif

#if DIAG_MESSAGES > 1
/* FIXME: check if everything works with DIAG_MESSAGES == 1 and 0 */
#define	verify_ses(p,m)	do { \
			if ((p)->attributes.b.session >= TCV_MAX_DESC || \
			descriptors [(p)->attributes.b.session] == NULL) \
			syserror (EASSERT, m); \
			} while (0)
#define	verify_fds(d,m)	do { \
			if ((d) < 0 || (d) > TCV_MAX_DESC || \
			descriptors [d] == NULL) \
			syserror (EASSERT, m); \
			} while (0)
#define	verify_fph(d,m)	do { \
			if ((d) < 0 || (d) > TCV_MAX_PHYS || \
			physical [d] == NULL) \
			syserror (EASSERT, m); \
			} while (0)
#define	verify_phy(p,m)	do { \
			if ((p)->attributes.b.phys >= TCV_MAX_PHYS || \
			physical [(p)->attributes.b.phys] == NULL) \
			syserror (EASSERT, m); \
			} while (0)
#define	verify_plg(p,f,m) do { \
			if ((p)->attributes.b.plugin >= TCV_MAX_PLUGS || \
			plugins [(p)->attributes.b.plugin] == NULL || \
			plugins [(p)->attributes.b.plugin] -> f == NULL) \
			syserror (EASSERT, m); \
			} while (0)
#define	verify_pld(p,f,m) do { \
			if ((p)->attpattern.b.plugin >= TCV_MAX_PLUGS || \
			plugins [(p)->attpattern.b.plugin] == NULL || \
			plugins [(p)->attpattern.b.plugin] -> f == NULL) \
			syserror (EASSERT, m); \
			} while (0)
#else
#define	verify_ses(p,m)
#define	verify_fds(d,m)
#define	verify_fph(d,m)
#define	verify_phy(p,m)
#define	verify_plg(p,f,m)
#define	verify_pld(p,f,m)
#endif

static void deq (hblock_t *p) {
/*
 * Removes a buffer from its queue
 */
	if (p->attributes.b.queued) {
		p->u.bqueue.next -> prev = p->u.bqueue.prev;
		p->u.bqueue.prev -> next = p->u.bqueue.next;
		p->attributes.b.queued = 0;
	}
}

static void enq (qhead_t *q, hblock_t *p) {
/*
 * Inserts a buffer into a queue
 */
	sysassert (p->attributes.b.queued == 0, "tcv enq item already queued");
	if (p->attributes.b.urgent) {
		/* At the front. This always triggers a queue event. */
		trigger ((word)q);
		p->u.bqueue.next = q->next;
		p->u.bqueue.prev = q;
		q->next->prev = (qitem_t*) p;
		q->next = (qitem_t*) p;
	} else {
		/*
		 * At the end. This triggers a queue event if the queue was
		 * empty.
		 */
		if (q_empty (q))
			trigger ((word)q);
		p->u.bqueue.next = q;
		p->u.bqueue.prev = q->prev;
		q->prev->next = (qitem_t*) p;
		q->prev = (qitem_t*) p;
	}
	p->attributes.b.queued = 1;
}

static void dispose (hblock_t *p, int dv) {
/*
 * Note that plugin functions can still return simple values, as in the previous
 * version, because the targets can be decoded from the buffer attributes.
 */
	/*
	 * Dispose always dequeues first, so if it ends up doing nothing,
	 * the packet will be left detached.
	 */
	deq (p);

	switch (dv) {
		case TCV_DSP_RCVU:
			p->attributes.b.urgent = 1;
		case TCV_DSP_RCV:
			verify_ses (p, "dispose ses");
			enq (&(descriptors [p->attributes.b.session]->rqueue),
				p);
			break;
		case TCV_DSP_XMTU:
			p->attributes.b.urgent = 1;
		case TCV_DSP_XMT:
			verify_phy (p, "dispose phy");
			enq (oqueues [p->attributes.b.phys], p);
			break;
		case TCV_DSP_DROP:
			rlp (p);
			break;
		default:
			/*
			 * Do nothing. We assume that the plugin knows what it
			 * is doing.
			 */
			break;
	}
}

#if 	TCV_TIMERS

static word runtq () {
/*
 * This function is called to run the timer queue. It returns the delay after
 * which it should be called again.
 */
	unsigned long cs, ts;
	titem_t *t;
	hblock_t *p;

	t = t_first;
	if (t_end (t))
		/* This means that the timer queue is empty */
		return MAX_UINT;

	if ((ts = (cs = seconds ()) - tcv_tim_set) >= t->value) {
		/* We go off */
		do {
			deqt (t);
			/* Get hold of the actual buffer */
			p = t_buffer (t);
			/* Locate the plugin function to call */
			verify_plg (p, tcv_tmt, "runtq");
			dispose (p, plugins [p->attributes.b.plugin] ->
				tcv_tmt ((address)(p + 1)));
			t = t_first;
		} while (!t_end (t) && t->value <= ts);

		if (!t_end (t)) {
			/* Reset the timer */
			tcv_tim_set = cs;
			do {
				t->value -= (word) ts;
				t = t_next (t);
			} while (!t_end (t));
			ts = 0;
		} else {
			return MAX_UINT;
		}
	}

	ts = (((unsigned long) (t_first->value)) - ts) * JIFFIES;

	return (ts < MAX_UINT) ? (word) ts : MAX_UINT;
}

/* TCV_TIMERS */
#endif

static void rlp (hblock_t *p) {
/*
 * Releases the packet buffer, frees its memory
 */

#if	TCV_TIMERS
	titem_t *t = &(p->tqueue);
	deqt (t);
#endif
	deq (p);

#if	TCV_HOOKS
	if (p->hptr)
		/* Zero out the hook */
		*(p->hptr) = NULL;
#endif
	/* Release memory */
	tfree ((address)p);
}

/*
 * Forced implicit packet dropping removed. Plugins will have to drop
 * packets explicitly (if they really want to).
 */

static hblock_t *apb (word size) {
/* ========================================= */
/* Allocates a packet buffer size bytes long */
/* ========================================= */

	hblock_t *p;
	word cs = size + hblenb;

	if ((p = (hblock_t*)tmalloc (cs)) == NULL)
		return NULL;

#if	TCV_HOOKS
	p -> hptr = NULL;
#endif
	p -> length = size;
	p -> attributes . value = 0;
	/*
	 * Queue pointers don't have to be initialized: the 'queued' flag
	 * is authoritative
	 */
#if	TCV_TIMERS
	/*
	 * But here we have to initialize something - just a little bit
	 */
	p -> tqueue . next = NULL;
	/* This means the packet is not on the timer queue */
#endif
	return p;
	/*
	 * Note that this doesn't initialize the plugin/phys/session stuff.
	 */
}

/* ---------------------------------------------------------------------- */
		/* ===================================== */
		/* Functions callable by the application */
		/* ===================================== */
/* ---------------------------------------------------------------------- */

void tcv_endp (address p) {
/*
 * Closes an outgoing or incoming packet. Note that this one cannot possibly
 * block because the packet is there already, and it either has to be queued
 * somewhere or deallocated.
 */
	hblock_t *b;
	sesdesc_t *d;

	b = header (p);
	verify_ses (b, "tcv_endp ses");
	d = descriptors [b->attributes.b.session];
	if (b->attributes.b.outgoing) {
		verify_plg (b, tcv_out, "tcv_endp plg");
		dispose (b, plugins [b->attributes.b.plugin] -> tcv_out (p));
	} else
		/* This is a received packet - just drop it */
		rlp (b);
}

int tcv_open (word state, int phy, int plid, ... ) {
/*
 * This function creates a new session descriptor and returns its number.
 *   phy  - physical interface number
 *   plid - plugin number
 * phy, along with the arguments following plid, is passed to the plugin's
 * tcv_open function
 */
	int eid, pid, fd;
	battr_t attp;
	sesdesc_t *s;

	/* Check if we have the plugin and the phys */
	if (phy < 0 || phy >= TCV_MAX_PHYS || oqueues [phy] == NULL ||
		plid < 0 || plid >= TCV_MAX_PLUGS || plugins [plid] == NULL)
			syserror (ENODEV, "tcv_open");

	pid = getpid ();
	/* Prepare the attribute pattern word */
	attp.value = 0;
	attp.b.plugin = plid;
	attp.b.phys = phy;
	/*
	 * This is set because the attribute pattern will be (mostly) used
	 * for initializing the attributes of outgoing packets.
	 */
	attp.b.outgoing = 1;
	/*
	 * This is a flag meaning 'open in progress' (if open was blocked in a
	 * previous attempt)
	 */
	attp.b.queued = 1;

	for (fd = -1, s = NULL, eid = 0; eid < TCV_MAX_DESC; eid++) {
		if ((s = descriptors [eid]) == NULL) {
			if (fd < 0)
				fd = eid;
		} else {
			/*
			 * We check for an open in progress, i.e., a resumed
			 * open that was blocked before
			 */
			attp.b.session = eid;
			if (s->attpattern.value == attp.value &&
			    s->pid == pid) {
				/* This is a pending open */
				fd = eid;
				break;
			}
			/*
			 * FIXME?: A possible problem here is that a process
			 * that blocks on open and then ignores the operation
			 * (e.g., dies or gets killed) will create a wasted
			 * descriptor. Oh, well, we can live with this, I
			 * guess.
			 */
		}
	}
	if (fd < 0)
		/* We fail gracefully */
		return ERROR;

	if (eid == TCV_MAX_DESC) {
		/* We have to create the session */
		s = (sesdesc_t*) umalloc (sizeof (sesdesc_t));
		if (s == NULL)
			syserror (EMALLOC, "tcv_open");
		descriptors [fd] = s;
		q_init (&(s->rqueue));
		s->attpattern = attp;
		s->attpattern.b.session = fd;
		s->pid = pid;
	}
	sysassert (plugins [plid] -> tcv_ope != NULL, "tcv_open/tcv_ope");
	eid = (word) (plugins [plid] -> tcv_ope (phy, fd, va_par (plid)));

	/*
	 * If eid is nonzero, tcv_ope has failed. -1 means a final failure,
	 * while anything else is interpreted as the event to be triggered
	 * when the operation should be restarted.
	 */
	if (eid == ERROR) {
		ufree (s);
		/* Undo the session */
		descriptors [fd] = NULL;
		return (int)ERROR;
	}
	if (eid == 0) {
		/* Success */
		s->attpattern.b.queued = 0;
		// s->pid = 0;
		return fd;
	}

	/* We block */
	if (state != NONE) {
		wait (eid, state);
		release;
	}

	return (int)BLOCKED;
}

int tcv_close (word state, int fd) {
/*
 * This one closes a session descriptor
 */
	sesdesc_t *s;
	word eid;
	hblock_t *b;

	verify_fds (fd, "tcv_close fd");
	if ((s = descriptors [fd]) == NULL || s->attpattern.b.queued)
		/* Also if the session hasn't opened yet */
		syserror (EREQPAR, "tcv_close");

	verify_pld (s, tcv_clo, "tcv_close/tcv_clo");
	eid = (word) (plugins [s->attpattern.b.plugin]->
					tcv_clo (s->attpattern.b.phys, fd));

	if (eid == ERROR)
		return (int) ERROR;

	if (eid == 0) {
		/* Success */
		while (!q_empty (&(s->rqueue))) {
			/* Empty the queue */
			b = q_first (&(s->rqueue));
			rlp (b);
		}
		ufree (s);
		descriptors [fd] = NULL;
		return 0;
	}

	/* We block */
	s->pid = getpid ();	/* We may use it for something later */
	if (state != NONE) {
		wait (eid, state);
		release;
	}

	return (int)BLOCKED;
}

void tcv_plug (int ord, tcvplug_t *pl) {
/*
 * This is one way now. Later we may implement switching plugs on the fly.
 */
	if (ord < 0 || ord >= TCV_MAX_PLUGS || plugins [ord] != NULL)
		syserror (EREQPAR, "tcv_plug");

	plugins [ord] = pl;
}

address tcv_rnp (word state, int fd) {
/*
 * Read next packet. Makes the next packet available for reading. Returns
 * the packet handle. Note: we do not worry about closing the previous read
 * packet anymore because it is now legal to have multiple packets being
 * read at the same time.
 */
	address p;
	hblock_t *b;
	qhead_t *rq;

	verify_fds (fd, "tcv_rnp fd");

	rq = &(descriptors [fd] -> rqueue);
	b = q_first (rq);
	if (q_end (b, rq)) {
		/* The queue is empty */
		if (state != NONE) {
			wait ((word)rq, state);
			release;
		}
		return NULL;
	}
	/* We have a packet */
	deq (b);
	/* Packet pointer */
	p = ((address)(b + 1));
	/* Set the pointers to application data */
	verify_plg (b, tcv_frm, "tcv_rnp/tcv_frm");
	plugins [b->attributes.b.plugin]->tcv_frm (p, b->attributes.b.phys,
		&(b->u.pointers));
	/* Adjust the second pointer to look like the length */
	b->u.pointers.tail =
		b->length - b->u.pointers.head - b->u.pointers.tail;
	/* OK, it seems that we are set */
	return p;
}

address tcv_wnp (word state, int fd, int length) {
/*
 * Creates a new outgoing packet and makes it available for writing. Returns
 * the packet handle. There may be several such packets started up per
 * session, so there's no notion of 'current' outgoing packet.
 */
	hblock_t *b;
	tcvadp_t ptrs;
	sesdesc_t *s;

	verify_fds (fd, "tcv_wnp fd");

	s = descriptors [fd];

	/* Obtain framing parameters */
	verify_pld (s, tcv_frm, "tcv_wnp/tcv_frm");
	plugins [s->attpattern.b.plugin]->tcv_frm (NULL, s->attpattern.b.phys,
		&ptrs);

	sysassert (s->attpattern.b.queued == 0, "tcv_wnp partially opened");

	/* Total length of the packet */

	if ((b = apb (length + ptrs . head + ptrs . tail)) == NULL) {
		/* No memory */
		if (state != NONE) {
			tmwait (state);
			release;
		}
		return NULL;
	}

	b->attributes = s->attpattern;
	b->u.pointers.head = ptrs.head;
	b->u.pointers.tail = length;

	return (address) (b + 1);
}

int tcv_read (address p, char *buf, int len) {
/*
 * Extracts (up to) len bytes from the packet
 */
	hblock_t *b = header (p);

	if (len >= b->u.pointers.tail)
		len = b->u.pointers.tail;

	if (len > 0) {
		memcpy (buf, ((char*)p) + b->u.pointers.head, len);
		b->u.pointers.tail -= len;
		b->u.pointers.head += len;
	}

	return len;
}

int tcv_write (address p, const char *buf, int len) {
/*
 * Writes (up to) len bytes to the packet
 */
	hblock_t *b = header (p);

	if (len >= b->u.pointers.tail)
		len = b->u.pointers.tail;

	if (len > 0) {
		memcpy (((char*)p) + b->u.pointers.head, buf, len);
		b->u.pointers.tail -= len;
		b->u.pointers.head += len;
	}
	return len;
}

int tcv_left (address p) {
/*
 * Tells how much packet space is left
 */
	return header (p) -> u.pointers.tail;
}

void tcv_urgent (address p) {
/*
 * Mark the packet as urgent
 */
	header (p) -> attributes.b.urgent = 1;
}

int tcv_control (int fd, int opt, address arg) {
/*
 * This generic function covers phys control operations available to the
 * application.
 */
	if (opt < 0) {
		if (fd < 0)
			return 0;
		if (opt == PHYSOPT_PLUGINFO) {
			tcvplug_t *p;
			if (fd > TCV_MAX_PLUGS)
				return 0;
			if ((p = plugins [fd]) == NULL)
				return 0;
			return p->tcv_info;
		}
		/* PHYSINFO */
		if (fd > TCV_MAX_PHYS)
			return 0;
		return physinfo [fd];
	}
	verify_fds (fd, "tcv_control fd");
	return tcvp_control (descriptors [fd] -> attpattern.b.phys, opt, arg);
}

/* ---------------------------------------------------------------------- */
	           /* ================================ */
	           /* Functions callable by the plugin */
	           /* ================================ */
/* ---------------------------------------------------------------------- */

int tcvp_control (int phy, int opt, address arg) {
/*
 * Plugin version of interface control
 */
	verify_fph (phy, "tcvp_control phy");
	return (physical [phy]) (opt, arg);
}

void tcvp_assign (address p, int ses) {
/*
 * Assigns the packet buffer to a specific session.
 */
	verify_fds (ses, "tcvp_assign fd");
	header (p) -> attributes.b.session = ses;
	header (p) -> attributes.b.phys = descriptors [ses]->attpattern.b.phys;
}

void tcvp_attach (address p, int phy) {
/*
 * Attaches the packet to a specific physical interface.
 */
	verify_fph (phy, "tcvp_attach phy");
	header (p) -> attributes.b.phys = phy;
}

address tcvp_clone (address p, int disp) {
/*
 * Duplicate the packet
 */
	hblock_t *pc, *pp = header (p);

	if ((pc = apb (pp->length)) == NULL) {
		/* No memory - it is legal to fail here */
		return NULL;
	}

	memcpy (payload (pc), payload (pp), pp->length);

	pc->attributes = pp->attributes;
	pc->attributes.b.queued = 0;
	pc->attributes.b.urgent = 0;
	dispose (pc, disp);
	return (address)(pc + 1);
}

void tcvp_dispose (address p, int dsp) {
/*
 * Plugin-visible dispose
 */
	dispose (header (p), dsp);
}

address tcvp_new (int size, int dsp, int ses) {
/*
 * Create a new packet with attributes inherited from the session
 */
	hblock_t *p;

	p = apb (size);
	if (p) {
		if (dsp != TCV_DSP_PASS) {
			/* Session must be defined for that */
			if (ses == NONE)
				syserror (EREQPAR, "tcvp_new");
			verify_fds (ses, "tcvp_new fd");
			p->attributes = descriptors [ses] -> attpattern;
			/* This will not be closed by tcv_endp */
			p->attributes.b.outgoing = 0;
			dispose (p, dsp);
		}
		return (address)(p + 1);
	}
	return NULL;
}

#if	TCV_HOOKS
void tcvp_hook (address p, address *h) {

	header (p) -> hptr = h;
}

void tcvp_unhook (address p) {

	header (p) -> hptr = NULL;
}
/* TCV_HOOKS */
#endif

#if	TCV_TIMERS
void tcvp_settimer (address p, word del) {
/*
 * Put the packet on the timer queue
 */
	titem_t *t;

	t = &(header(p)->tqueue);
	/* Remove from current location */
	deqt (t);

	if (t_empty) {
		/* First entry */
		t->value = del;
		tcv_q_tim . next = tcv_q_tim . prev = t;
		t -> next = t -> prev = (titem_t*)(&tcv_q_tim);
		tcv_tim_set = seconds ();
		trigger ((word)(&tcv_q_tim));
	} else {
		titem_t *tt;
		/* Adjust the delay */
		del += (word) (seconds () - tcv_tim_set);
		t -> value = del;
		tt = t_first;
		if (tt->value >= del) {
			/* Our delay is the smallest */
			trigger ((word)(&tcv_q_tim));
		} else {
			for (tt = t_next (tt); !t_end (tt); tt = t_next (tt))
				if (tt->value >= del)
					break;
		}
		t->next = tt;
		t->prev = tt->prev;
		tt->prev->next = t;
		tt->prev = t;
	}
}

void tcvp_cleartimer (address p) {
/*
 * Plugin-callable function to remove a packet from the timer queue
 */
	titem_t *t = &(header(p)->tqueue);
	deqt (t);
}
#endif

int tcvp_length (address p) {
	return header (p) -> length;
}

/* ---------------------------------------------------------------------- */
		    /* ============================== */
		    /* Functions callable by the phys */
		    /* ============================== */
/* ---------------------------------------------------------------------- */

word tcvphy_reg (int phy, ctrlfun_t ps, int info) {
/*
 * This is called to register a physical interface. The second argument
 * points to a function that controls (i.e., changes the options of) the
 * interface.
 */
	qhead_t *q;

	if (phy < 0 || phy >= TCV_MAX_PHYS || physical [phy] != NULL)
		syserror (EREQPAR, "tcvphy_reg");

	physical [phy] = ps;
	physinfo [phy] = info;

	oqueues [phy] = q = (qhead_t*) umalloc (sizeof (qhead_t));
	if (q == NULL)
		syserror (EMALLOC, "tcvphy_reg");
	q_init (q);

	/*
	 * Queue event identifier (which happens to be the queue pointer
	 * in disguise).
	 */
	return (word)q;
}

void tcvphy_rcv (int phy, address p, int len) {
/*
 * Called when a packet is received by a phy. Each phy has its private
 * (possibly static) reception buffer that it maintains by itself.
 */
	int plg, dsp, ses;
	tcvadp_t ap;
	address c;

	verify_fph (phy, "tcvphy_rcv phy");

	dsp = NONE;
	for (plg = TCV_MAX_PLUGS-1; plg >= 0; plg--) {
		/*
		 * We start from the high end giving the plugins with higher
		 * numbers preference in claiming the packet. The idea is that
		 * plugins with lower numbers are likely to be "default"
		 * (or fall back) plugins to be used when none of the
		 * "specific" plugins claims the packet.
		 */
		if (plugins [plg] == NULL)
			continue;
		sysassert (plugins [plg] -> tcv_rcv != NULL,
			"tcvphy_rcv/tcv_rcv");
		if ((dsp = plugins [plg] -> tcv_rcv (phy, p, len, &ses, &ap)) !=
			TCV_DSP_PASS)
				/* This plugin claims it */
				break;
	}

	if (dsp == NONE || dsp == TCV_DSP_DROP)
		/*
		 * Either no one is claiming the packet or the claimant says
		 * we should drop it.
		 */
		return;

	len -= (ap.head + ap.tail);

	/* Acquire a buffer */
	if ((c = tcvp_new (len, dsp, ses)) == NULL)
		/* No room - drop it */
		return;

	memcpy ((char*)c, ((char*)p) + ap.head, len);
}

address tcvphy_get (int phy, int *len) {
/*
 * Acquires the next outgoing packet to be sent on the specified interface.
 * Returns the packet pointer and its length.
 */
	qhead_t	*oq;
	hblock_t *b;

	verify_fph (phy, "tcvphy_get phy");

	oq = oqueues [phy];
	b = q_first (oq);
	if (q_end (b, oq)) {
		/* The queue is empty */
		return NULL;
	}

	*len = b->length;
	return (address) (b + 1);
}

int tcvphy_top (int phy) {
/*
 * Returns 0 if there's no outgoing packet, 1 if there's a non-urgent
 * outgoing packet, and 2 if the packet is urgent.
 */
	qhead_t *oq;
	hblock_t *b;

	verify_fph (phy, "tcvphy_top phy");

	oq = oqueues [phy];
	b = q_first (oq);
	if (q_end (b, oq))
		return 0;

	return (b->attributes.b.urgent) ? 2 : 1;
}

void tcvphy_end (address pkt) {
/*
 * Marks the end of packet transmission
 */
	hblock_t *b = header (pkt);

	verify_plg (b, tcv_xmt, "tcvphy_end/tcv_xmt");
	dispose (b, plugins [b->attributes.b.plugin]->tcv_xmt (pkt));
}

int tcvphy_erase (int phy) {
/*
 * Erases the output queue
 */
	int nq;
	qhead_t	*oq;
	hblock_t *b;

	verify_fph (phy, "tcvphy_erase phy");

	for (nq = 0; ; nq++) {
		oq = oqueues [phy];
		b = q_first (oq);
		if (q_end (b, oq))
			return nq;
		dispose (b,
		    plugins [b->attributes.b.plugin]->tcv_xmt ((address)(b+1)));
	}
	return nq;
}

#if	TCV_TIMERS
static process (timersrv, void)
/*
 * This simple process is needed to service the timer queue
 */
    entry (0)

	delay (runtq (), 0);
	wait ((word)(&tcv_q_tim), 0);
	release;
	
	nodata;

endprocess (1)
#endif

void tcv_init () {
/*
 * This one is really simple in this version. There isn't much to initialize,
 * and whatever little there is, it can be done statically.
 */
#if	TCV_TIMERS
	fork (timersrv, NULL);
#endif
}

/* TCV_PRESENT */
#endif
