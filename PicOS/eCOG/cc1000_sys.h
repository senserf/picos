#ifndef	__pg_cc1000_sys_h
#define	__pg_cc1000_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "gpioirq.c"

#include "board_rf.h"

#define	clear_cc1000_int	CNOP
#define	hard_lock		clr_xcv_int

#define	adc_stop		do { \
					fd.ssm.cfg.adc_en = 0; \
					fd.adc.ctrl.int_clr = 1; \
				} while (0)

#define	adc_wait		do { while (fd.adc.sts.rdy == 0); } while (0)

#define	disable_xcv_timer	CNOP

// See mach.h
// #define	adc_value	fd.adc.sts.data

#define	adc_start	(fd.ssm.cfg.adc_en = 1)
#define	adc_disable	CNOP

#if 0
#define	lbt_ok(v)	((int)RSSI_MAX - (int)(v) < \
			(word)(((long)LBT_THRESHOLD * (RSSI_MAX-RSSI_MIN))/100))
#endif	/* DISABLED */

#endif
