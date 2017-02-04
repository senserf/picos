#ifndef	__tcv_h
#define __tcv_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2017                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

#if	TCV_PRESENT

#define	qitem_t			__tcv_qitem_t
#define	qhead_t			__tcv_qhead_t
#define	titem_t			__tcv_titem_t
#define	thead_t			__tcv_thead_t
#define	battr_t			__tcv_battr_t

#define	hblock_t		__tcv_hblock_t
#define	sesdesc_t		__tcv_sesdesc_t

#define	hblenb			__tcv_hblenb
#define hblen			__tcv_hblen

#define	header(p)		__tcv_header (p)

// ============================================================================

#define	payload(p)	(byteaddr ((p) + 1))

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

// Here is some mess required by the timers
#ifdef	__SMURPH__

process TCVTimerService;

#else

#include "kernel.h"

#if TCV_TIMERS
void __pi_tcv_runqueue (word, word*);
void __pi_tcv_execqueue ();
#endif

#endif	/* __SMURPH__ */

#endif

#endif
