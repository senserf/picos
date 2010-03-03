#ifndef __pg_irq_irmtn_h
#define __pg_irq_irmtn_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

if (irmtn_signal) {
	irmtn_clear;
	if (irmtn_icounter != WNONE)
		irmtn_icounter++;
}

#endif
