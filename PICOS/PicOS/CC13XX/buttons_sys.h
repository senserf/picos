/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_buttons_sys_h
#define	__pg_buttons_sys_h

#include "pins.h"

extern const word __button_list [];

#define	BUTTON_GPIO(b)			(((b) >> 8) & 0xff)
#define	BUTTON_REPEAT(b)		(((b)     ) & 0xff)

#define	BUTTON_DEF(gpio,repeat)		( ((gpio) << 8) | (repeat) )

// We inherit (from MSP430) the rigid concept of the same polarity for all
// buttons; here we are in fact more flexible (or rather it's easier to be
// flexible), so this can easily change later, if needed
#ifndef	BUTTON_PRESSED_LOW
#define	BUTTON_PRESSED_LOW	0
#endif

#define	button_pin_status(b)	GPIO_readDio (BUTTON_GPIO (b))
#define	button_int_status(b)	GPIO_getEventDio (BUTTON_GPIO (b))

#if	BUTTON_PRESSED_LOW
#define	button_still_pressed(b) (button_pin_status(b) == 0)
#else
#define	button_still_pressed(b) button_pin_status(b)
#endif

void __buttons_setirq (int);

#define	buttons_enable()	__buttons_setirq (1)
#define	buttons_disable()	__buttons_setirq (0)
#define	buttons_init()		buttons_disable ()
#define	button_pressed(b)	button_still_pressed (b)

#define	buttons_int		(HWREG (GPIO_BASE + GPIO_O_EVFLAGS31_0) & \
					BUTTON_GPIOS)	
#endif
