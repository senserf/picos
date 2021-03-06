/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_kernel_h
#define __pg_kernel_h		1

/* ========================================================================== */
/*                     PicOS                                                  */
/*                                                                            */
/* Main kernel include file                                                   */
/*                                                                            */

#include "sysio.h"
#include "uart.h"
#include "pins.h"

void	__pi_set_release (void);
void	update_n_wake (word, Boolean);

#ifdef SENSOR_LIST
void	__pi_init_sensors (void);
#endif

#ifdef LCDG_PRESENT
void	__pi_lcdg_init (void);
#endif

extern 			word  		__pi_mintk;
extern 	volatile 	word 		__pi_old, __pi_new;

extern	void tcv_init (void);

extern	__pi_pcb_t	*__PCB;

#define	for_all_tasks(i)	for (i = __PCB; i != NULL; i = i->Next)
#define	pcb_not_found(i)	((i) == NULL)

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
// the old 'io' mechanism.
// Note: the event used to look like this:
// 	#define devevent(dev,ope) (((aword)&io) + ((dev) << 3) + (ope))
// which posed too much demands on the loader. So I am guessing a random
// "address" that won't be confused with any other (normal) event. No big
// harm if it is, I guess.
#define devevent(dev,ope) ((MAX_AWORD - 257) + ((dev) << 3) + (ope))

#define	iowait(dev,eve,sta)	wait (devevent (dev,eve), sta)
#define	iotrigger(dev,eve)	trigger (devevent (dev, eve))

/* Special i/o 'operations': - attention events (interrupt + request) */
#define	REQUEST		3
#define	ATTENTION 	4

/* =============================================== */
/* Event trigger code for interrupt mode functions */
/* =============================================== */
#define	i_trigger(evnt)	do {\
		int j; __pi_pcb_t *i;\
		for_all_tasks (i) {\
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
		for (j = 0; j < nevents ((__pi_pcb_t*)(pcs)); j++) { \
			if (((__pi_pcb_t*)(pcs))->Events [j] . Event == evnt) {\
				wakeupev ((__pi_pcb_t*)(pcs), j);\
				break;\
			} \
		} \
	} while (0)

#endif
