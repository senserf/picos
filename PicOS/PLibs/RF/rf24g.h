/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_rf24g_h
#define	__pg_rf24g_h	1

#include "phys_rf24g.h"
#include "rf24g_sys.h"
#include "rfleds.h"

#ifndef	RADIO_LBT_MIN_BACKOFF
#define	RADIO_LBT_MIN_BACKOFF	8
#endif

#ifndef	RADIO_LBT_BACKOFF_EXP
#define	RADIO_LBT_BACKOFF_EXP	8
#endif

#define	FLG_RCVA	0x80 	/* Receiver active */
#define	FLG_RCVI	0x40	/* Receiver interrupt enabled */
#define	FLG_XMTE	0x20	/* Transmitter enabled */

#define	stat_get(flg)	((__pi_x_power & flg) != 0)
#define	stat_set(flg)	__pi_x_power |= (flg)
#define	stat_clr(flg)	__pi_x_power &= ~(flg)

// Note: e is a constant, so the condition will be optimized out
#define	gbackoff(e) 	do { if (e) __pi_x_backoff = RADIO_LBT_MIN_BACKOFF + \
				(rnd () & ((1 << (e)) - 1)); } while (0)

extern word	*__pi_r_buffer, __pi_v_qevent, __pi_v_physid, __pi_v_statid,
		__pi_x_backoff;

extern byte	__pi_v_paylen, __pi_x_power, __pi_v_group, __pi_v_channel,
		__pi_v_rxoff, __pi_v_txoff;

#define	rxevent	((word)&__pi_r_buffer)

#endif
