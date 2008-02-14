#ifndef	__pg_sensors_h
#define	__pg_sensors_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
//+++ "sensors.c"

#define	SENSOR_DEF(inf,vlf,par)	{ vlf, inf, par }
//
// Init function (or NULL), action function, parameter (to be interpreted
// by the action function)
//
typedef	struct {

	void (*fun_val) (word, word, address);
	void (*fun_ini) (void);
	word param;

} sensdesc_t;

void read_sensor (word, word, address);

#endif
