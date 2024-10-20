/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_tcvplug_h
#define	__pg_tcvplug_h		1

#include "sysio.h"

#if	TCV_PRESENT

#ifndef	__SMURPH__

#if	TCV_TIMERS
void	tcvp_settimer (address, word);
void	tcvp_cleartimer (address);
#endif

int	tcvp_control (int, int, address);
void	tcvp_assign (address, int);
void	tcvp_attach (address, int);
void	tcvp_dispose (address, int);
address	tcvp_clone (address, int);
address	tcvp_new (int, int, int);

#if	TCV_HOOKS
void	tcvp_hook (address, address*);
void	tcvp_unhook (address);
#endif

#endif	/* __SMURPH __ */

#define	tcvp_isqueued(p)	(__tcv_header (p) -> attributes.b.queued)
#define	tcvp_isoutgoing(p)	(__tcv_header (p) -> attributes.b.outgoing)
#define	tcvp_isurgent(p)	tcv_isurgent (p)
#define	tcvp_session(p)		(__tcv_header (p) -> attributes.b.session)
#define	tcvp_plugin(p)		(__tcv_header (p) -> attributes.b.plugin)
#define	tcvp_phys(p)		(__tcv_header (p) -> attributes.b.phys)
#define	tcvp_length(p)		(__tcv_header (p) -> length)

#if	TCV_HOOKS
#define	tcvp_gethook(p)		(__tcv_header (p) -> hptr)
#endif

#if	TCV_TIMERS
#define	tcvp_issettimer(p)	((__tcv_header (p) -> tqueue) . next != NULL)
#define	tcvp_isdetached(p)	(!tcvp_isqueued(p) && !tcvp_issettimer(p))
#else
#define	tcvp_isdetached(p)	(!tcvp_isqueued(p))
#endif

/* Disposition codes */
#define	TCV_DSP_PASS	0
#define	TCV_DSP_DROP	1
#define	TCV_DSP_RCV	2
#define	TCV_DSP_RCVU	3
#define	TCV_DSP_XMT	4
#define	TCV_DSP_XMTU	5

#endif	/* TCV_PRESENT */

#endif
