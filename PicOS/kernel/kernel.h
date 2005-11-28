#ifndef __pg_kernel_h
#define __pg_kernel_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* ========================================================================== */
/*                     PicOS                                                  */
/*                                                                            */
/* Main kernel include file                                                   */
/*                                                                            */

#include "sysio.h"
#include "uart.h"

void	zzz_set_release (void);
void	zzz_tservice (void);

typedef struct	{
/* =================================== */
/* A single event awaited by a process */
/* =================================== */
	word	Status;
	/* =========================================================== */
	/* Status = State << 4 | EType, where EType != 0 for an active */
	/* event wait entry                                            */
	/* =========================================================== */
	word	Event;
} event_t;

/* =========== */
/* Event types */
/* =========== */
#define	ETYPE_USER	1	/* User signal */
#define	ETYPE_SYSTEM	2	/* System signal */
#define	ETYPE_IO	3	/* I/O */
#define	ETYPE_TERM	4	/* Process termination */
#define	ETYPE_TERMANY	5	/* Termination of any process (joinall) */
#define	ETYPE_LDELAY	6	/* Minute delay */

#define	efree(e)		((e).Status == 0)
#define	setestatus(e,t,s)	((e).Status = ((s) << 4) | (t))
#define	getetype(e)		((e).Status & 0xf)

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
	word	Timer;		/* Timer delay in ticks */
	code_t	code;		/* Code function pointer */
	address	data;		/* Data pointer */
	event_t	Events [MAX_EVENTS_PER_TASK];

#if SCHED_PRIO
        int prio;
#endif

} pcb_t;

extern	pcb_t	__PCB [];

extern	void tcv_init (void);

#define FIRST_PCB		(&(__PCB [0]))
#define	LAST_PCB		(FIRST_PCB + MAX_TASKS)
#define for_all_tasks(i)	for (i = FIRST_PCB; i != LAST_PCB; i++)
#define tasknum(p)		((p) - FIRST_PCB)

#define	incwait(p)	((p)->Status++)
#define	inctimer(p)	((p)->Status |= 0x8)
#define	waiting(p)	((p)->Status & 0xf)
#define	twaiting(p)	((p)->Status & 0x8)
#define	nevents(p)	((p)->Status & 0x7)
#define	tstate(p)	((p)->Status >> 4)
#define	settstate(p,t)	((p)->Status = ((p)->Status & 0x7) | ((t) << 4))
#define	prcdstate(p,t)	((p)->Status = (t) << 4)

#define wakeupev(p,j)	((p)->Status = (p)->Events [j].Status & 0xfff0)
#define wakeuptm(p)	do { (p)->Status &= 0xfff0; (p)->Timer = 0; } while (0)

typedef	void (*devinitfun_t)(int param);
typedef int (*devreqfun_t)(int, char*, int);

typedef struct{
/* ===================================== */
/* This describes a single device driver */
/* ===================================== */
	devinitfun_t	init;
	int		param;
} devinit_t;

void zz_swait (word, word, word);
int zz_strigger (int, word);
void adddevfunc (devreqfun_t, int);

/* Encoding device events */
#define	devevent(dev,ope) ((dev) << 4 | (ope))

#define	swait(a,b,c)		zz_swait (a, b, c)
#define	strigger(a,b)		zz_strigger (a, b)
#define	iowait(dev,eve,sta)	swait (ETYPE_IO, devevent (dev,eve), sta)
#define	iotrigger(dev,eve)	strigger (ETYPE_IO, devevent (dev, eve))
#define	ldtrigger(del)		strigger (ETYPE_LDELAY, del)

/* Special i/o 'operations': - attention events (interrupt + request) */
#define	ATTENTION 	0xf
#define	REQUEST		0xe

/* Check whether PID is legitimate */
#define ver_pid(i,pid)	do { \
				for_all_tasks (i) \
					if ((int)(i) == pid) \
						break; \
				if ((i) == LAST_PCB) \
					syserror (EREQPAR, "pid"); \
			} while (0)

/* =============================================== */
/* Event trigger code for interrupt mode functions */
/* =============================================== */
#define	i_trigger(etype,evnt)	do {\
	int j; pcb_t *i;\
	for_all_tasks (i) {\
		if (nevents (i) == 0)\
			continue;\
		if (i->code == NULL)\
			continue;\
		for (j = 0; j < nevents (i); j++) {\
			if (i->Events [j] . Event == evnt &&\
				getetype (i->Events [j]) == etype) {\
					zz_systat.evntpn = 1;\
					wakeupev (i, j);\
					break;\
			}\
		}\
	}\
	} while (0)

/* ==================================== */
/* A shortcut when the process is known */
/* ==================================== */
#define	p_trigger(pcs,etype,evnt)	do {\
	if (((pcb_t*)(pcs)) -> code != NULL) { \
		int j; \
		for (j = 0; j < nevents ((pcb_t*)(pcs)); j++) { \
			if (((pcb_t*)(pcs))->Events [j] . Event == evnt && \
				getetype (((pcb_t*)(pcs))->Events [j]) == \
				    etype) {\
					zz_systat.evntpn = 1;\
					wakeupev ((pcb_t*)(pcs), j);\
					break;\
			} \
		} \
	} \
	} while (0)

#endif
