#ifndef	__pg_tcvplug_h
#define	__pg_tcvplug_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

#if	TCV_PRESENT

#ifndef	__SMURPH__

#if	TCV_TIMERS
void	tcvp_settimer (address, word);
void	tcvp_cleartimer (address);
Boolean	tcvp_issettimer (address);
#endif

int	tcvp_length (address);
int	tcvp_control (int, int, address);
void	tcvp_assign (address, int);
void	tcvp_attach (address, int);
void	tcvp_dispose (address, int);
address	tcvp_clone (address, int);
address	tcvp_new (int, int, int);
Boolean	tcvp_isqueued (address);

#if	TCV_HOOKS
void	tcvp_hook (address, address*);
void	tcvp_unhook (address);
address	*tcvp_gethook (address);
#endif

#endif	/* __SMURPH __ */

/* Disposition codes */
#define	TCV_DSP_PASS	0
#define	TCV_DSP_DROP	1
#define	TCV_DSP_RCV	2
#define	TCV_DSP_RCVU	3
#define	TCV_DSP_XMT	4
#define	TCV_DSP_XMTU	5

#endif	/* TCV_PRESENT */

#endif
