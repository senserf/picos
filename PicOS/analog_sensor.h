#ifndef	__pg_analog_sensor_h
#define	__pg_analog_sensor_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sensors.h"

#include "analog_sensor_sys.h"
//+++ "analog_sensor.c"

void analog_sensor_read (word, const a_sensdesc_t*, address);

#endif
