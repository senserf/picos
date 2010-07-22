#ifndef	__tcv_h
#define __tcv_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

/* ========================================================================== */
/*                     PicOS                                                  */
/*                                                                            */
/* Transceiver driver include file                                            */
/*                                                                            */

#if	TCV_PRESENT

#ifndef	TCV_HOOKS
#define	TCV_HOOKS	0
#endif

#ifndef	TCV_TIMERS
#define	TCV_TIMERS	0
#endif

struct qitem_struct {
	struct qitem_struct	*next,
				*prev;
};

typedef	struct qitem_struct	qitem_t;
typedef	struct qitem_struct	qhead_t;

#define	TCV_QHEAD_LENGTH	4

struct titem_struct {
// Timer queue item
	struct titem_struct	*next,
				*prev;
	word			value;
};

typedef	struct titem_struct	titem_t;

typedef	struct {
	titem_t			*next,
				*prev;
} thead_t;

typedef	union {
	word	value;
	struct	{
		word 	queued:1,
			outgoing:1,
			urgent:1,
			session:7,
			plugin:3,
			phys:3;
	} b;
} battr_t;

/* =================== */
/* Buffer header block */
/* =================== */
struct hblock_struct {
	/*
	 * These ones must be at the very beginning
	 */
    union {
	qitem_t			bqueue;		/* Buffer queue links */
	tcvadp_t		pointers;	/* Application data pointers */
	/*
	 * Note: the application data pointers are only valid for a packet that
	 * has been either removed from a queue, or not yet put into a queue;
	 * thus, we recycle the links for this purpose.
	 */
    } u;			// 4 bytes PicOS, 8 bytes simulator

#if	TCV_HOOKS
	address *hptr;		// 2 bytes PicOS, 4 bytes simulator
#define	TCV_HBLOCK_HOOKS_LENGTH		2
#else
#define	TCV_HBLOCK_HOOKS_LENGTH		0
#endif
	/*
	 * Packet length in bytes.
	 */
	word	length;		// 2 bytes
	/*
	 * Flags (e.g., whether the packet is queued or not) + plugin ID +
	 * phys ID
	 */
	battr_t	attributes;	// 2 bytes

#if	TCV_TIMERS
	/*
	 * Timer queue links (must be the last item, see
	 * t_tqoffset below
	 */
	titem_t	tqueue;		// 6 bytes PicOS, 12 bytes simulator
#define	TCV_HBLOCK_TIMERS_LENGTH	6
#else
#define	TCV_HBLOCK_TIMERS_LENGTH	0
#endif
};

#define	TCV_HBLOCK_LENGTH   (8+TCV_HBLOCK_HOOKS_LENGTH+TCV_HBLOCK_TIMERS_LENGTH)

typedef	struct hblock_struct	hblock_t;

#define	hblenb		(sizeof (hblock_t))
#define hblen		(hblenb/sizeof(word))

#define	payload(p)	(byteaddr ((p) + 1))
#define	header(p)	((hblock_t*)((p) - hblen))

#define	q_first(q)	((hblock_t*)((q)->next))
#define	q_end(p,q)	(((qitem_t*)(p)) == (q))
#define	q_next(p)	((hblock_t*)(((p)->u.bqueue).next))
#define	q_empty(q)	((q)->next == (qitem_t*)(q))
#define	q_init(q)	((q)->next = (q)->prev = (qitem_t*)(q))

#define	t_first		(tcv_q_tim . next)
#define	t_end(p)	((p) == (titem_t*)(&tcv_q_tim))
#define	t_next(p)	((p)->next)

// This one doesn't work any more:
//#define	t_tqoffset	(((int*)(((hblock_t*)0)->tqueue))-(int*)((hblock_t*)0))
// ... and this one is a bit less reliable:
#define	t_tqoffset	((sizeof(hblock_t) - sizeof(titem_t))/sizeof (word))
#define t_buffer(p)	((hblock_t*)((sint*)(p) - t_tqoffset))
#define	t_empty		(tcv_q_tim . next == (titem_t*)(&tcv_q_tim))

#define	deqt(t)		do { \
				if ((t) -> next) { \
					(t)->prev->next = (t)->next; \
					(t)->next->prev = (t)->prev; \
					(t)->next = NULL; \
				} \
			} while (0)

typedef	struct {
/*
 * Session descriptor
 */
	qhead_t		rqueue;		/* Reception queue */
	/*
	 * This is the attribute pattern word for a new outgoing packet
	 */
	battr_t		attpattern;
	/*
	 * Note: we no longer use the notion of the currently read/written
	 * packet because the packet itself knows where it belongs.
	 */

	/*
	 * This one is used while the session is being open and the requesting
	 * process must go to sleep. Using it, we can identify the descriptor
	 * when the open operation is resumed. Kind of clumsy, especially that
	 * I can think of no other use for this attribute.
	 */
	int		pid;
} sesdesc_t;

#define	TCV_SESDESC_LENGTH	(4+2+2)

// Here is some mess required by the timers
#ifdef	__SMURPH__

process TCVTimerService;

#else

#include "kernel.h"
void __pi_tcv_runqueue (word, word*);
#endif	/* __SMURPH__ */

#endif

#endif
