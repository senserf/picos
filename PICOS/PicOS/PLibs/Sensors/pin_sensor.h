/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pin_sensor_h
#define	__pin_sensor_h

#include "pins_sys.h"

#if defined(INPUT_PIN_LIST) || defined(OUTPUT_PIN_LIST)
//+++ pin_sensor.c
#endif

#ifdef INPUT_PIN_LIST
void pin_sensor_init ();
void pin_sensor_read (word, const byte*, address);
#endif

#ifdef OUTPUT_PIN_LIST
void pin_actuator_write (word, const byte*, address);
#endif

#endif
