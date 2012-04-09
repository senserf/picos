#ifndef	__pg_sensors_h
#define	__pg_sensors_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
//+++ "sensors.c"

#include "sensors_sys.h"

void read_sensor (word, sint, address);

#ifdef	SENSOR_EVENTS

void wait_sensor (sint, word);

#endif

#endif
