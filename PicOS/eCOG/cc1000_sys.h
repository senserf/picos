#ifndef	__pg_cc1000_sys_h
#define	__pg_cc1000_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "gpioirq.c"

#include "board_rf.h"
#include "pins.h"

#define	clear_cc1000_int	CNOP
#define	hard_lock		clr_xcv_int


#define	disable_xcv_timer	CNOP

#if 0
#define	lbt_ok(v)	((int)RSSI_MAX - (int)(v) < \
			(word)(((long)LBT_THRESHOLD * (RSSI_MAX-RSSI_MIN))/100))
#endif	/* DISABLED */

#endif
