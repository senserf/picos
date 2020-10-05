/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_pwm_driver_h
#define	__pg_pwm_driver_h	1

//+++ "pwm_driver.c"

void pwm_driver_start (), pwm_driver_stop ();
void pwm_driver_setwidth (word), pwm_driver_write (word, const byte*, address);

#endif
