/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef	__tcv_cc__
#define	__tcv_cc__

#include "tcv.h"
#include "tcvphys.h"
#include "tcvplug.h"

/* #define	DUMPQUEUES	1 */

/* ========================================================================== */
/*                     PicOS                                                  */
/*                                                                            */
/* The open-ended and open-sided part of the transceiver driver               */


#if	TCV_PRESENT

#ifdef	__SMURPH__

#include "board.h"

// The simulator

#define	TMALLOC(a,b)	(TheNode->memAlloc (a, b))
#define	TFREE(a)	(TheNode->memFree (a))
#define	UMALLOC(a,b)	TMALLOC (a, b)
#define	UFREE(a)	TFREE (a)

#else	/* PicOS */
// PicOS
#define	TMALLOC(a,b)	tmalloc (a)
#define	UMALLOC(a,b)	umalloc (a)

#endif	/* SMURPH or PicOS */

// Memory allocators (good for all occasions)
#define	b_malloc(s)	TMALLOC (s, (s)-(sizeof (hblock_t)-TCV_HBLOCK_LENGTH))
#define	s_malloc(s)	UMALLOC (s, (s)-(sizeof (sesdesc_t)-TCV_SESDESC_LENGTH))
#define	q_malloc(s)	UMALLOC (s, (s)-(sizeof (qhead_t)-TCV_QHEAD_LENGTH))

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

#ifndef __SMURPH__

#include "tcv_node_data.h"

static void rlp (hblock_t*);

#endif

#if DUMP_MEMORY
__PRIVF (PicOSNode, void, dmpq) (qhead_t *q) {

	hblock_t *pp;
	diag ("START Q DUMP %x", (word)q);
	for (pp = q_first (q); !q_end (pp, q); pp = q_next (pp))
		diag ("%d %x [%x %x %x]", pp->length, pp->attributes,
			((word*)(payload (pp))) [0],
			((word*)(payload (pp))) [1],
			((word*)(payload (pp))) [2]
		);
	diag ("END Q DUMP %x", (word)q);
}

__PUBLF (PicOSNode, void, tcv_dumpqueues) (void) {

	int	i;
	for (i = 0; i < TCV_MAX_DESC; i++) {
		if (descriptors [i] != NULL) {
			diag ("TCV QUEUE RCV [%d]", i);
			dmpq (&(descriptors [i]->rqueue));
		}
	}
	for (i = 0; i < TCV_MAX_PHYS; i++) {
		if (oqueues [i] != NULL) {
			diag ("TCV QUEUE XMT [%d]", i);
			dmpq (oqueues [i]);
		}
	}
}
#endif

__PRIVF (PicOSNode, void, deq) (hblock_t *p) {
/*
 * Removes a buffer from its queue
 */
	if (p->attributes.b.queued) {
		p->u.bqueue.next -> prev = p->u.bqueue.prev;
		p->u.bqueue.prev -> next = p->u.bqueue.next;
		p->attributes.b.queued = 0;
	}
}

// ============================================================================

#if TCV_TIMERS
__PRIVF (PicOSNode, void, deqtm) (hblock_t *p) {
/*
 * Removes a buffer from the timer queue
 */
	titem_t *t = &(p->tqueue);
	deqt (t);
}
#else
#define	deqtm(a)	CNOP
#endif

#if TCV_HOOKS
__PRIVF (PicOSNode, void, deqhk) (hblock_t *p) {
/*
 * Clears the buffer's hook
 */
	if (p->hptr != NULL) {
		*(p->hptr) = NULL;
		p->hptr = NULL;
	}
}
#else
#define	deqhk(a)	CNOP
#endif

// ============================================================================

__PRIVF (PicOSNode, void, enq) (qhead_t *q, hblock_t *p) {
/*
 * Inserts a buffer into a queue
 */
	sysassert (p->attributes.b.queued == 0, "tcv01");
	if (p->attributes.b.urgent) {
		/* At the front. This always triggers a queue event. */
		trigger (q);
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
			trigger (q);
		p->u.bqueue.next = q;
		p->u.bqueue.prev = q->prev;
		q->prev->next = (qitem_t*) p;
		q->prev = (qitem_t*) p;
	}
	p->attributes.b.queued = 1;
}

#if	TCV_LIMIT_RCV || TCV_LIMIT_XMT
__PRIVF (PicOSNode, Boolean, qmore) (qhead_t *q, word lim) {

	hblock_t *p;

	for (p = q_first (q); !q_end (p, q); p = q_next (p))
		if (--lim == 0)
			return YES;
	return NO;
}
#endif

__PRIVF (PicOSNode, int, empty) (qhead_t *oq) {
/*
 * Empties the indicated queue
 */
	int nq;
	hblock_t *b;

	for (nq = 0; ; nq++) {
		b = q_first (oq);
		if (q_end (b, oq))
			return nq;
		// deq (b);
		rlp (b);
	}
	return nq;
}

__PRIVF (PicOSNode, void, dispose) (hblock_t *p, int dv) {
/*
 * Note that plugin functions can still return simple values, as in the previous
 * version, because the targets can be decoded from the buffer attributes.
 */
	/*
	 * Dispose always dequeues first, so if it ends up doing nothing,
	 * the packet will be left detached.
	 */
	deq (p);	// Soft dequeue, timer and hook left intact

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
			 * Do nothing. The plugin knows what it is doing.
			 */
			break;
	}
}

__PRIVF (PicOSNode, void, rlp) (hblock_t *p) {
/*
 * Releases the packet buffer, frees its memory
 */
	deq (p);	// Remove from queue
	deqtm (p);	// Clear the timer ...
	deqhk (p);	// ... and the hook
	/* Release memory */
	tfree ((address)p);
}

/*
 * Forced implicit packet dropping removed. Plugins will have to drop
 * packets explicitly (if they really want to).
 */

__PRIVF (PicOSNode, hblock_t*, apb) (word size) {
/* ========================================= */
/* Allocates a packet buffer size bytes long */
/* ========================================= */

	hblock_t *p;
	word cs = size + hblenb;

	if ((p = (hblock_t*)b_malloc (cs)) == NULL)
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

__PUBLF (PicOSNode, void, tcv_endp) (address p) {
/*
 * Closes an outgoing or incoming packet. Note that this one cannot possibly
 * block because the packet is there already, and it either has to be queued
 * somewhere or deallocated.
 */
	hblock_t *b;
	sesdesc_t *d;

	b = header (p);
	verify_ses (b, "tcv02");
	d = descriptors [b->attributes.b.session];
	if (b->attributes.b.outgoing) {
		verify_plg (b, tcv_out, "tcv03");
		dispose (b, plugins [b->attributes.b.plugin] -> tcv_out (p));
	} else
		/* This is a received packet - just drop it */
		rlp (b);
}

__PUBLF (PicOSNode, int, tcv_open) (word state, int phy, int plid, ... ) {
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

#ifdef	__SMURPH__
#define	va_par(s)	ap
	va_list		ap;
	va_start (ap, plid);
#endif
	/* Check if we have the plugin and the phys */
	if (phy < 0 || phy >= TCV_MAX_PHYS || oqueues [phy] == NULL ||
		plid < 0 || plid >= TCV_MAX_PLUGS || plugins [plid] == NULL)
			syserror (ENODEVICE, "tcv04");

	pid = getcpid ();
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
		s = (sesdesc_t*) s_malloc (sizeof (sesdesc_t));
		if (s == NULL)
			syserror (EMALLOC, "tcv05");
		descriptors [fd] = s;
		q_init (&(s->rqueue));
		s->attpattern = attp;
		s->attpattern.b.session = fd;
		s->pid = pid;
	}
	sysassert (plugins [plid] -> tcv_ope != NULL, "tcv06");
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
	if (state != WNONE) {
		when (eid, state);
		release;
	}

	return (int)BLOCKED;
}

__PUBLF (PicOSNode, int, tcv_close) (word state, int fd) {
/*
 * This one closes a session descriptor
 */
	sesdesc_t *s;
	word eid;
	hblock_t *b;

	verify_fds (fd, "tcv07");
	if ((s = descriptors [fd]) == NULL || s->attpattern.b.queued)
		/* Also if the session hasn't opened yet */
		syserror (EREQPAR, "tcv08");

	verify_pld (s, tcv_clo, "tcv09");
	eid = (word) (plugins [s->attpattern.b.plugin]->
					tcv_clo (s->attpattern.b.phys, fd));

	if (eid == (word) ERROR)
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
	s->pid = getcpid ();	/* We may use it for something later */
	if (state != WNONE) {
		when (eid, state);
		release;
	}

	return (int)BLOCKED;
}

__PUBLF (PicOSNode, int, tcv_plug) (int ord, const tcvplug_t *pl) {
/*
 * This is one way now. Later we may implement switching plugs on the fly.
 */
	if (ord < 0 || ord >= TCV_MAX_PLUGS ||
	    (plugins [ord] != NULL && plugins [ord] != pl))
		return ERROR;

	plugins [ord] = pl;
	return 0;
}

__PUBLF (PicOSNode, address, tcv_rnp) (word state, int fd) {
/*
 * Read next packet. Makes the next packet available for reading. Returns
 * the packet handle. Note: we do not worry about closing the previous read
 * packet anymore because it is now legal to have multiple packets being
 * read at the same time.
 */
	address p;
	hblock_t *b;
	qhead_t *rq;

	verify_fds (fd, "tcv10");

	rq = &(descriptors [fd] -> rqueue);
	b = q_first (rq);
	if (q_end (b, rq)) {
		/* The queue is empty */
		if (state != WNONE) {
			when (rq, state);
			release;
		}
		return NULL;
	}

	deq (b);	// Dequeue the packet ...
	deqtm (b);	// ... and clear its timer

	/* Packet pointer */
	p = ((address)(b + 1));
	/* Set the pointers to application data */
	verify_plg (b, tcv_frm, "tcv11");
	plugins [b->attributes.b.plugin]->tcv_frm (p, b->attributes.b.phys,
		&(b->u.pointers));
	/* Adjust the second pointer to look like the length */
	b->u.pointers.tail =
		b->length - b->u.pointers.head - b->u.pointers.tail;
	/* OK, it seems that we are set */
	return p;
}

__PUBLF (PicOSNode, int, tcv_qsize) (int fd, int disp) {
/*
 * Return the queue size
 */
	sesdesc_t *s;
	int nq;
	qhead_t *rq;
	hblock_t *b;

	verify_fds (fd, "tcv12");

	s = descriptors [fd];

	if (disp == TCV_DSP_RCV || disp == TCV_DSP_RCVU) {
		rq = &(s->rqueue);
	} else if (disp == TCV_DSP_XMT || disp == TCV_DSP_XMTU) {
		rq = oqueues [s->attpattern.b.phys];
	} else {
		syserror (EREQPAR, "tcv13");
	}

	nq = 0;
	if (disp == TCV_DSP_XMTU || disp == TCV_DSP_RCVU) {
		// Urgent only
		for (b = q_first (rq); !q_end (b, rq); b = q_next (b)) {
			if (b->attributes.b.urgent)
				nq++;
			else
				// They are all in front
				break;
		}
	} else {
		for (b = q_first (rq); !q_end (b, rq); b = q_next (b))
			nq++;
	}

	return nq;
}

__PUBLF (PicOSNode, int, tcv_erase) (int fd, int disp) {
/*
 * Erases the indicated queue: TCV_DSP_RCV, TCV_DSP_XMT
 */
	sesdesc_t *s;
	int nq;
	qhead_t *rq;
	hblock_t *b;

	verify_fds (fd, "tcv14");

	s = descriptors [fd];

	if (disp == TCV_DSP_RCV || disp == TCV_DSP_RCVU) {
		rq = &(s->rqueue);
	} else if (disp == TCV_DSP_XMT || disp == TCV_DSP_XMTU) {
		rq = oqueues [s->attpattern.b.phys];
	} else {
		syserror (EREQPAR, "tcv15");
	}

	if (disp == TCV_DSP_RCVU || disp == TCV_DSP_XMTU) {
		// All
		return empty (rq);
	}

	// Non-urgent
	nq = 0;
Er_rt:
	for (b = q_first (rq); !q_end (b, rq); b = q_next (b)) {
		if (b->attributes.b.urgent == 0) {
			// deq (b);
			rlp (b);
			nq++;
			goto Er_rt;
		}
	}

	return nq;
}

#if	TCV_LIMIT_XMT

__PUBLF (PicOSNode, address, tcv_wnpu) (word state, int fd, int length) {
/*
 * Urgent variant of wnp. Bumps by 1 the queue size limit and marks the packet
 * as urgent.
 */
	hblock_t *b;
	tcvadp_t ptrs;
	sesdesc_t *s;

	verify_fds (fd, "tcv16");

	s = descriptors [fd];

	/* Obtain framing parameters */
	verify_pld (s, tcv_frm, "tcv17");

	plugins [s->attpattern.b.plugin]->tcv_frm (NULL, s->attpattern.b.phys,
		&ptrs);

	sysassert (s->attpattern.b.queued == 0, "tcv18");

	if (qmore (oqueues [s->attpattern.b.phys], TCV_LIMIT_XMT+1)) {
		if (state != WNONE) {
			tmwait (state);
			release;
		}
		return NULL;
	}

	if ((b = apb (length + ptrs . head + ptrs . tail)) == NULL) {
		/* No memory */
		if (state != WNONE) {
			tmwait (state);
			release;
		}
		return NULL;
	}

	b->attributes = s->attpattern;
	b->attributes.b.urgent = 1;
	b->u.pointers.head = ptrs.head;
	b->u.pointers.tail = length;

	return (address) (b + 1);
}

#else	/* TCV_LIMIT_XMT */

__PUBLF (PicOSNode, address, tcv_wnpu) (word state, int fd, int length) {

	address p = tcv_wnp (state, fd, length);

	if (p != NULL)
		tcv_urgent (p);

	return p;
}

#endif	/* TCV_LIMIT_XMT */

__PUBLF (PicOSNode, address, tcv_wnp) (word state, int fd, int length) {
/*
 * Creates a new outgoing packet and makes it available for writing. Returns
 * the packet handle. There may be several such packets started up per
 * session, so there's no notion of 'current' outgoing packet.
 */
	hblock_t *b;
	tcvadp_t ptrs;
	sesdesc_t *s;

	verify_fds (fd, "tcv19");

	s = descriptors [fd];

	/* Obtain framing parameters */
	verify_pld (s, tcv_frm, "tcv20");
	plugins [s->attpattern.b.plugin]->tcv_frm (NULL, s->attpattern.b.phys,
		&ptrs);

	sysassert (s->attpattern.b.queued == 0, "tcv21");

	/* Total length of the packet */

#if	TCV_LIMIT_XMT
	if (qmore (oqueues [s->attpattern.b.phys], TCV_LIMIT_XMT)) {
		if (state != WNONE) {
			tmwait (state);
			release;
		}
		return NULL;
	}
#endif
	if ((b = apb (length + ptrs . head + ptrs . tail)) == NULL) {
		/* No memory */
		if (state != WNONE) {
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

__PUBLF (PicOSNode, int, tcv_read) (address p, char *buf, int len) {
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

__PUBLF (PicOSNode, int, tcv_write) (address p, const char *buf, int len) {
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

__PUBLF (PicOSNode, void, tcv_drop) (address p) {
/*
 * Drop the packet unconditionally
 */
	if (p != NULL)
		rlp (header (p));
}

__PUBLF (PicOSNode, int, tcv_left) (address p) {
/*
 * Tells how much packet space is left
 */
	return header (p) -> u.pointers.tail;
}

__PUBLF (PicOSNode, void, tcv_urgent) (address p) {
/*
 * Mark the packet as urgent
 */
	header (p) -> attributes.b.urgent = 1;
}

__PUBLF (PicOSNode, Boolean, tcv_isurgent) (address p) {

	return header (p) -> attributes.b.urgent;
}

__PUBLF (PicOSNode, int, tcv_control) (int fd, int opt, address arg) {
/*
 * This generic function covers phys control operations available to the
 * application.
 */
	if (opt < 0) {
		if (fd < 0)
			return 0;
		if (opt == PHYSOPT_PLUGINFO) {
			const tcvplug_t *p;
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
	verify_fds (fd, "tcv22");
	return tcvp_control (descriptors [fd] -> attpattern.b.phys, opt, arg);
}

/* ---------------------------------------------------------------------- */
	           /* ================================ */
	           /* Functions callable by the plugin */
	           /* ================================ */
/* ---------------------------------------------------------------------- */

__PUBLF (PicOSNode, int, tcvp_control) (int phy, int opt, address arg) {
/*
 * Plugin version of interface control
 */
	verify_fph (phy, "tcv23");
	return (physical [phy]) (opt, arg);
}

__PUBLF (PicOSNode, void, tcvp_assign) (address p, int ses) {
/*
 * Assigns the packet buffer to a specific session.
 */
	verify_fds (ses, "tcv24");
	header (p) -> attributes.b.session = ses;
	header (p) -> attributes.b.phys = descriptors [ses]->attpattern.b.phys;
}

__PUBLF (PicOSNode, void, tcvp_attach) (address p, int phy) {
/*
 * Attaches the packet to a specific physical interface.
 */
	verify_fph (phy, "tcv25");
	header (p) -> attributes.b.phys = phy;
}

__PUBLF (PicOSNode, address, tcvp_clone) (address p, int disp) {
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

__PUBLF (PicOSNode, void, tcvp_dispose) (address p, int dsp) {
/*
 * Plugin-visible dispose
 */
	dispose (header (p), dsp);
}

__PUBLF (PicOSNode, address, tcvp_new) (int size, int dsp, int ses) {
/*
 * Create a new packet with attributes inherited from the session
 */
	hblock_t *p;

	if (dsp != TCV_DSP_PASS) {

		/* Session must be defined for that */
		if (ses == NONE)
			syserror (EREQPAR, "tcv26");
		verify_fds (ses, "tcv27");

#if	TCV_LIMIT_RCV

		if ((dsp == TCV_DSP_RCV || dsp == TCV_DSP_RCVU) &&
	    	    qmore (&(descriptors [ses]->rqueue), (dsp == TCV_DSP_RCVU) ?
			TCV_LIMIT_RCV + 1 : TCV_LIMIT_RCV)) {
		    	    // Drop
		    	    return NULL;
		}
#endif

#if	TCV_LIMIT_XMT

		if ((dsp == TCV_DSP_XMT || dsp == TCV_DSP_XMTU) &&
	    	    qmore (oqueues [descriptors [ses]->attpattern.b.phys],
			(dsp == TCV_DSP_XMTU) ? TCV_LIMIT_XMT + 1 :
			    TCV_LIMIT_XMT)) {
		    	   	 // Drop
		    	   	 return NULL;
		}
#endif
		if ((p = apb (size)) != NULL) {
			p->attributes = descriptors [ses] -> attpattern;
			/* If you accidentally call tcv_endp on it */
			p->attributes.b.outgoing = 0;
			dispose (p, dsp);
			return (address)(p + 1);
		}
		return NULL;
	}

	if ((p = apb (size)) != NULL)
		return (address)(p + 1);
	else
		return NULL;
}

__PUBLF (PicOSNode, Boolean, tcvp_isqueued) (address p) {

	return header (p) -> attributes.b.queued;
}

#if	TCV_HOOKS
__PUBLF (PicOSNode, void, tcvp_hook) (address p, address *h) {

	header (p) -> hptr = h;
	*h = p;
}

__PUBLF (PicOSNode, address*, tcvp_gethook) (address p) {

	return header (p) -> hptr;
}

__PUBLF (PicOSNode, void, tcvp_unhook) (address p) {

	if (p != NULL)
		deqhk (header (p));
}
/* TCV_HOOKS */
#endif

#if	TCV_TIMERS

//=============================================================================
// VUEE mess needed to implement the timers right
//=============================================================================

#ifdef	__SMURPH__

process TCVTimerService : _PP_ (PicOSNode) {

	TIME started;
	word current;

	void wake ();
	void newitem (word);

	states { Start };
	perform;
};

void TCVTimerService::wake () {

	titem_t *t, *f;
	hblock_t *p;
	word min;

#define plugins S->plugins

	min = MAX_WORD;

	for (t = S->tcv_q_tim.next; t != (titem_t*)&(S->tcv_q_tim); t = f) {
		f = t->next;
		if (t->value <= current) {
			deqt (t);
			p = t_buffer (t);
			verify_plg (p, tcv_tmt, "runtq");
			S->dispose (p, plugins [p->attributes.b.plugin] ->
				tcv_tmt ((address)(p + 1)));
		} else {
			t->value -= current;
			if (t->value < min)
				min = t->value;
		}
	}
	current = min;

#undef plugins
}

void TCVTimerService::newitem (word del) {

	word res;
	double sof;

	trigger (&(S->tcv_q_tim));

	if (S->tcv_q_tim.next == (titem_t*)&(S->tcv_q_tim)) {
		// Empty queue
		current = 0;
		return;
	}

	// Progress so far
	sof = ituToEtu (Time - started) * MSCINSECOND;
	if (sof < current)
		current = (word) sof;

	// Update all times to reflect the partial progress
	wake ();
	current = 0;
}

TCVTimerService::perform {

	state Start:

		wake ();
		when (&(S->tcv_q_tim), Start);

		if (S->tcv_q_tim.next == (titem_t*)&(S->tcv_q_tim)) {
			// Empty queue
			release;
		}

		started = Time;
		delay (current, Start);
}

#else

void __pi_tcv_runqueue (word new, word *min) {
//
// Invoked in one place (see kernel.c)
//
	titem_t *t, *f;
	hblock_t *p;
	word d;

	for (t = t_first; !t_end (t); t = f) {
		f = t->next;
		if (twakecnd (__pi_old, new, t->value)) {
			// Trigger this one
			deqt (t);
			p = t_buffer (t);
			verify_plg (p, tcv_tmt, "runtq");
			dispose (p, plugins [p->attributes.b.plugin] ->
				tcv_tmt ((address)(p + 1)));
		} else {
			if ((d = t->value - new) < *min)
				*min = d;
		}
	}
}

#endif	/* __SMURPH__ */

__PUBLF (PicOSNode, void, tcvp_settimer) (address p, word del) {
/*
 * Put the packet on the timer queue
 */
	titem_t *t;

	t = &(header(p)->tqueue);

	// Remove from the queue (if present already) to avoid confusing
	// update_n_wake
	deqt (t);

#ifdef __SMURPH__
	TheNode->tcv_tservice->newitem (del);
	t -> value = del;
#else
	update_n_wake (del);
	t -> value = __pi_old + del;
#endif

	// Queue back
	t->next = tcv_q_tim . next;
	t->prev = (titem_t*)(&tcv_q_tim);
	tcv_q_tim.next->prev = t;
	tcv_q_tim.next = t;
}

__PUBLF (PicOSNode, void, tcvp_cleartimer) (address p) {
/*
 * Plugin-callable function to remove a packet from the timer queue
 */
	deqtm (header (p));
}

__PUBLF (PicOSNode, Boolean, tcvp_issettimer) (address p) {

	return (header (p) -> tqueue) . next != NULL;

}

#endif	/* TIMERS */

__PUBLF (PicOSNode, int, tcvp_length) (address p) {
	return header (p) -> length;
}

/* ---------------------------------------------------------------------- */
		    /* ============================== */
		    /* Functions callable by the phys */
		    /* ============================== */
/* ---------------------------------------------------------------------- */

__PUBLF (PicOSNode, int, tcvphy_reg) (int phy, ctrlfun_t ps, int info) {
/*
 * This is called to register a physical interface. The second argument
 * points to a function that controls (i.e., changes the options of) the
 * interface.
 */
	qhead_t *q;

	if (phy < 0 || phy >= TCV_MAX_PHYS || physical [phy] != NULL)
		syserror (EREQPAR, "tcv28");

	physical [phy] = ps;
	physinfo [phy] = info;

	oqueues [phy] = q = (qhead_t*) q_malloc (sizeof (qhead_t));
	if (q == NULL)
		syserror (EMALLOC, "tcv29");
	q_init (q);

	/*
	 * Queue event identifier (which happens to be the queue pointer
	 * in disguise).
	 */
	return ptrtoint (q);
}

__PUBLF (PicOSNode, int, tcvphy_rcv) (int phy, address p, int len) {
/*
 * Called when a packet is received by a phy. Each phy has its private
 * (possibly static) reception buffer that it maintains by itself.
 */
	int plg, dsp, ses;
	tcvadp_t ap;
	address c;

	verify_fph (phy, "tcv30");

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
		sysassert (plugins [plg] -> tcv_rcv != NULL, "tcv31");
		if ((dsp = plugins [plg] -> tcv_rcv (phy, p, len, &ses, &ap)) !=
			TCV_DSP_PASS)
				/* This plugin claims it */
				break;
	}

	if (dsp == NONE || dsp == TCV_DSP_DROP) {
		/*
		 * Either no one is claiming the packet or the claimant says
		 * we should drop it.
		 */
		return 0;
	}

	len -= (ap.head + ap.tail);

	/* Acquire a buffer */
	if ((c = tcvp_new (len, dsp, ses)) == NULL)
		/* No room - drop it */
		return 0;

	memcpy ((char*)c, ((char*)p) + ap.head, len);

	return 1;
}

__PUBLF (PicOSNode, address, tcvphy_get) (int phy, int *len) {
/*
 * Acquires the next outgoing packet to be sent on the specified interface.
 * Returns the packet pointer and its length.
 */
	qhead_t	*oq;
	hblock_t *b;

	verify_fph (phy, "tcv32");

	oq = oqueues [phy];
	b = q_first (oq);
	if (q_end (b, oq)) {
		/* The queue is empty */
		return NULL;
	}

	*len = b->length;
	deq (b);	// Dequeue the packet ...
	deqtm (b);	// ... and clear its timer
	return (address) (b + 1);
}

__PUBLF (PicOSNode, address, tcvphy_top) (int phy) {
/*
 * Returns the pointer to the first outgoing packet.
 */
	qhead_t *oq;
	hblock_t *b;

	verify_fph (phy, "tcv33");

	oq = oqueues [phy];
	b = q_first (oq);
	if (q_end (b, oq))
		return NULL;

	return (address)(b + 1);
}

__PUBLF (PicOSNode, void, tcvphy_end) (address pkt) {
/*
 * Marks the end of packet transmission
 */
	hblock_t *b = header (pkt);

	verify_plg (b, tcv_xmt, "tcv34");
	dispose (b, plugins [b->attributes.b.plugin]->tcv_xmt (pkt));
}

__PUBLF (PicOSNode, int, tcvphy_erase) (int phy) {
/*
 * Erases the output queue
 */
	verify_fph (phy, "tcv35");
	return empty (oqueues [phy]);
}

__PUBLF (PicOSNode, void, tcv_init) () {

#ifdef __SMURPH__
	// Initialize the otherwise statically initialized data
	int i;

	for (i = 0; i < TCV_MAX_DESC; i++)
		descriptors [i] = NULL;
	for (i = 0; i < TCV_MAX_PHYS; i++) {
		physical [i] = NULL;
		oqueues [i] = NULL;
		physinfo [i] = 0;
	}
	for (i = 0; i < TCV_MAX_PLUGS; i++)
		plugins [i] = NULL;
#endif

#if	TCV_TIMERS
	// These ones are always initialized dynamically
	tcv_q_tim . next = tcv_q_tim . prev = (titem_t*) &tcv_q_tim;
#ifdef __SMURPH__
	tcv_tservice = create TCVTimerService;
	tcv_tservice -> _pp_apid_ ();
#endif	/* __SMURPH__ */

#endif	/* TCV_TIMERS */
}

#ifdef	deqtm
#undef	deqtm
#endif

#ifdef	deqhk
#undef	deqhk
#endif

/* TCV_PRESENT */
#endif

#endif
