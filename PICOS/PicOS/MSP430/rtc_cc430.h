/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_rtc_cc430_h
#define	__pg_rtc_cc430_h

//+++ "rtc_cc430.c"

#include "sysio.h"
#include "pins.h"
#include "board_rtc.h"

typedef	struct {

	byte	year, month, day, dow, hour, minute, second;

} rtc_time_t;

void	rtc_set (const rtc_time_t*);
void	rtc_get (rtc_time_t*);
void	rtc_calibrate (int);

#endif
