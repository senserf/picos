#ifndef	__pin_sensor_h
#define	__pin_sensor_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ pin_sensor.c

void pin_sensor_init ();
void pin_sensor_read (word, const byte*, address);
void pin_sensor_interrupt ();

#endif
