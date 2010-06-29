#ifndef	__pg_leds_sys_h
#define	__pg_leds_sys_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	ZZ_LEDS03(b)	(rg.io.gp0_3_out |= (b))
#define	ZZ_LEDS47(b)	(rg.io.gp4_7_out |= (b))

#endif
