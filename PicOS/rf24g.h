#ifndef	__pg_rf24g_h
#define	__pg_rf24g_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "phys_rf24g.h"
#include "rf24g_sys.h"
#include "rfleds.h"

#define	FLG_RCVA	0x80 	/* Receiver active */
#define	FLG_RCVI	0x40	/* Receiver interrupt enabled */
#define	FLG_XMTE	0x20	/* Transmitter enabled */

#define	stat_get(flg)	((__pi_x_power & flg) != 0)
#define	stat_set(flg)	__pi_x_power |= (flg)
#define	stat_clr(flg)	__pi_x_power &= ~(flg)

#define	gbackoff	(__pi_x_backoff = MIN_BACKOFF + (rnd () & MSK_BACKOFF))

extern word	*__pi_r_buffer, __pi_v_qevent, __pi_v_physid, __pi_v_statid,
		__pi_x_backoff;

extern byte	__pi_v_paylen, __pi_x_power, __pi_v_group, __pi_v_channel,
		__pi_v_rxoff, __pi_v_txoff;

#define	rxevent	((word)&__pi_r_buffer)

#endif
