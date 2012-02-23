#ifndef	__pg_actuators_h
#define	__pg_actuators_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
//+++ "actuators.c"

#define	ACTUATOR_DEF(inf,vlf)	{ vlf, inf }

typedef	struct {

	void (*fun_val) (word, address);
	void (*fun_ini) (void);

} actudesc_t;

void write_actuator (word, sint, address);

#endif
