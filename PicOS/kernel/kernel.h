#ifndef __pg_kernel_h
#define __pg_kernel_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* ========================================================================== */
/*                     PicOS                                                  */
/*                                                                            */
/* Main kernel include file                                                   */
/*                                                                            */

#include "sysio.h"
#include "uart.h"
#include "pins.h"

void	__pi_set_release (void);
void	update_n_wake (word);

#ifdef SENSOR_LIST
void	__pi_init_sensors (void);
#endif

#ifdef LCDG_PRESENT
void	__pi_lcdg_init (void);
#endif

typedef struct	{
/* =================================== */
/* A single event awaited by a process */
/* =================================== */
	word	State;
	word	Event;
} event_t;

typedef struct	{
	/* ============================================================== */
	/* This is the PCB. Status consists of two parts. The three least */
	/* significant bits store the number of awaited events except for */
	/* the Timer delay, and the fourth bit is set if a Timer event is */
	/* being awaited.  The remaining (upper 12) bits encode the state */
	/* to be assumed when the Timer goes off. Also, if the process is */
	/* ready to go, those bits encode the process's current state.    */
	/* ============================================================== */
	word	Status;
	word	Timer;		/* Timer wakeup tick */
	code_t	code;		/* Code function pointer */
	address	data;		/* Data pointer */
	event_t	Events [MAX_EVENTS_PER_TASK];
} pcb_t;

extern	pcb_t	__PCB [];

extern 			word  		__pi_mintk;
extern 	volatile 	word 		__pi_old, __pi_new;

extern	void tcv_init (void);

#define FIRST_PCB		(&(__PCB [0]))
#define	LAST_PCB		(FIRST_PCB + MAX_TASKS)
#define for_all_tasks(i)	for (i = FIRST_PCB; i != LAST_PCB; i++)

#define	setestate(e,s,v)	do { \
					(e).State = (s) << 4; \
					(e).Event = (v); \
				} while (0)

#define wakeupev(p,j)	((p)->Status = (p)->Events [j].State)

#define	incwait(p)	((p)->Status++)
#define	inctimer(p)	((p)->Status |= 0x8)
#define	waiting(p)	((p)->Status & 0xf)
#define	twaiting(p)	((p)->Status & 0x8)
#define	nevents(p)	((p)->Status & 0x7)
#define	tstate(p)	((p)->Status >> 4)
#define	settstate(p,t)	((p)->Status = ((p)->Status & 0x7) | ((t) << 4))
#define	prcdstate(p,t)	((p)->Status = (t) << 4)

#define wakeuptm(p)	(p)->Status &= 0xfff0
#define cltmwait(p)	(p)->Status &= 0xfff7

// The best compilation of the timer wakeup condition, which says that m
// (describing the target timer value for the first-to-be-awakened delay)
// is between o(ld) and n(ew), effectively qualifying for a wakeup. This
// accounts for a wraparound, i.e., if n >= o, the first && must hold (m
// must be between the two (inclusively). Otherwise, we have a wraparound
// and the second or must hold.
#define	twakecnd(o,n,m)	( ( (m) <= (n) && (m) >= (o) ) || \
			  ( ((n) < (o)) && ( ((m) <= (n)) || ((m) >= (o)) ) ) )

typedef	void (*devinitfun_t)(int param);
typedef int (*devreqfun_t)(int, char*, int);

typedef struct {
/* ===================================== */
/* This describes a single device driver */
/* ===================================== */
	devinitfun_t	init;
	int		param;
} devinit_t;

void adddevfunc (devreqfun_t, int);

// Encoding device events: an address-derived value being different for
// different devices/operations; note: this is obsolete and only used by
// the old 'io' mechanism
#define devevent(dev,ope) (((word)&io) + ((dev) << 3) + (ope))

#define	iowait(dev,eve,sta)	wait (devevent (dev,eve), sta)
#define	iotrigger(dev,eve)	trigger (devevent (dev, eve))

/* Special i/o 'operations': - attention events (interrupt + request) */
#define	REQUEST		3
#define	ATTENTION 	4

/* =============================================== */
/* Event trigger code for interrupt mode functions */
/* =============================================== */
#define	i_trigger(evnt)	do {\
		int j; pcb_t *i;\
		for_all_tasks (i) {\
			if (i->code == NULL)\
				continue;\
			for (j = 0; j < nevents (i); j++) {\
				if (i->Events [j] . Event == evnt) {\
					wakeupev (i, j);\
					break;\
				}\
			}\
		}\
	} while (0)

/* ==================================== */
/* A shortcut when the process is known */
/* ==================================== */
#define	p_trigger(pcs,evnt) do {\
		int j; \
		for (j = 0; j < nevents ((pcb_t*)(pcs)); j++) { \
			if (((pcb_t*)(pcs))->Events [j] . Event == evnt) {\
				wakeupev ((pcb_t*)(pcs), j);\
				break;\
			} \
		} \
	} while (0)

#endif
