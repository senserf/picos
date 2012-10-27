#ifndef	__pg_pwm_driver_h
#define	__pg_pwm_driver_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "pwm_driver.c"

void pwm_driver_start (), pwm_driver_stop ();
void pwm_driver_setwidth (word), pwm_driver_write (word, address);

#endif
