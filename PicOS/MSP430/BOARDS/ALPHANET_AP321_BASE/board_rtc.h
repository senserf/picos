/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#define	__pi_rtc_init()	RTCCTL01 = RTCSSEL__ACLK + RTCMODE
//+++ "rtc_cc430.c"

