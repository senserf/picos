/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_analog_sensor_h
#define	__pg_analog_sensor_h

#include "sensors.h"

//+++ "analog_sensor.c"

void analog_sensor_read (word, const a_sensdesc_t*, address);

#endif
