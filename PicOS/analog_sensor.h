#ifndef	__pg_analog_sensor_h
#define	__pg_analog_sensor_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "analog_sensor_sys.h"
//+++ "analog_sensor.c"

// Shifts for param bits (second argument of analog_sensor_read)
#define	ASNS_PNO_SH	0
#define	ASNS_SHT_SH	4
#define	ASNS_ISI_SH	8
#define	ASNS_NSA_SH	10
#define	ASNS_REF_SH	14

void analog_sensor_read (word, word, address);

#endif
