#ifndef	__pg_sensors_h
#define	__pg_sensors_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
//+++ "sensors.c"

#define	SENSOR_DEF(inf,vlf)	{ vlf, inf }

typedef	struct {

	void (*fun_val) (word, address);
	void (*fun_ini) (void);

} sensdesc_t;

void read_sensor (word, word, address);

#endif
