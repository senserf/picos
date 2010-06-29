#ifndef	__pg_rf24g_h
#define	__pg_rf24g_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "phys_rf24g.h"
#include "rf24g_sys.h"
#include "rfleds.h"

#define	FLG_RCVA	0x80 	/* Receiver active */
#define	FLG_RCVI	0x40	/* Receiver interrupt enabled */
#define	FLG_XMTE	0x20	/* Transmitter enabled */

#define	stat_get(flg)	((zzx_power & flg) != 0)
#define	stat_set(flg)	zzx_power |= (flg)
#define	stat_clr(flg)	zzx_power &= ~(flg)

#define	gbackoff	(zzx_backoff = MIN_BACKOFF + (rnd () & MSK_BACKOFF))

extern word	*zzr_buffer, zzv_qevent, zzv_physid, zzv_statid, zzx_backoff;

extern byte	zzv_paylen, zzx_power, zzv_group, zzv_channel, zzv_rxoff,
		zzv_txoff;

#define	rxevent	((word)&zzr_buffer)

#endif
