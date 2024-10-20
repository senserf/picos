/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_as3932_sys_h
#define	__pg_as3932_sys_h

#include "pins.h"

// Manchester rate (the default)
#define	AS3932_BITRATE		2730

// This yields almost exactly 12 for the standard 32768 Hz clock (probably
// intended this way)
#define	AS3932_TICKS_PER_BIT	(CRYSTAL_RATE / AS3932_BITRATE)

// Make it half way between 1 and 2
#define	AS3932_SHORT_BIT	(AS3932_TICKS_PER_BIT + \
					(AS3932_TICKS_PER_BIT / 2))
// This is somewhat bigger than 2
#define	AS3932_TIMEOUT		(AS3932_TICKS_PER_BIT * 3)

#if	__TCI_CONFIG__ == 1
// Use timer A on MSP430
#define	AS3932_TCI_CTL		TACTL
#define	AS3932_TCI_CCTL		TACCTL0
#define	AS3932_TIMER		TACCR0
#define	AS3932_CLOCK		TAR
#define	AS3932_TCI_VECTOR	TIMERA0_VECTOR

#endif

#if	__TCI_CONFIG__ == 2
// Use the second A timer on CC430
#define	AS3932_TCI_CTL		TA1CTL
#define	AS3932_TCI_CCTL		TA1CCTL0
#define	AS3932_TIMER		TA1CCR0
#define	AS3932_CLOCK		TA1R
#define	AS3932_TCI_VECTOR	TIMER1_A0_VECTOR

#endif

#ifndef	AS3932_TCI_CTL
#error	"S: undefined TCI_CONFIG for AS3932 driver"
#endif

#define	as3932_clock		AS3932_CLOCK
#define	as3932_init_timer	AS3932_TIMER = AS3932_TIMEOUT
#define	as3932_start_timer	AS3932_TCI_CTL = TASSEL0 | TACLR | MC0
#define	as3932_stop_timer	_BIC (AS3932_TCI_CTL, MC0 + MC1)

#define	as3932_tim_sti		_BIS (AS3932_TCI_CCTL, CCIE)
#define	as3932_tim_cli		_BIC (AS3932_TCI_CCTL, CCIE)

#endif
