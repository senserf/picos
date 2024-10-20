/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_sensors_h
#define	__pg_sensors_h

#include "sysio.h"
//+++ "sensors.c"

#include "sensors_sys.h"

void read_sensor (word, sint, address);

#ifdef	SENSOR_EVENTS

void wait_sensor (sint, word);

#endif

#endif
