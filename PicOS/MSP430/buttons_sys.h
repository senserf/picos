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

#define	BUTTON_PORT(b)			(((b) >> 12) & 0xf)
#define	BUTTON_REPEAT(b)		(((b) >> 8 ) & 0xf)
#define	BUTTON_PIN(b)			((byte) (b))

#define	BUTTON_DEF(port,pin,repeat)	( ((port) << 12) | \
					  ((repeat) << 8) | \
						(pin))

#ifndef	BUTTON_PRESSED_LOW
#define	BUTTON_PRESSED_LOW	0
#endif

#define	button_pin_status(b)	(((BUTTON_PORT (b) == 2) ? P2IN : P1IN) & \
					BUTTON_PIN (b))

#define	button_int_status(b)	(((BUTTON_PORT (b) == 2) ? P2IFG : P1IFG) & \
					BUTTON_PIN (b))

// ============================================================================

#if	BUTTON_PRESSED_LOW

#define	buttons_iedge_p1	_BIS (P1IES, BUTTON_PIN_P1_IRQ)
#define	buttons_iedge_p2	_BIS (P2IES, BUTTON_PIN_P2_IRQ)
#define	button_still_pressed(b) (button_pin_status(b) == 0)

#else

#define	buttons_iedge_p1	_BIC (P1IES, BUTTON_PIN_P1_IRQ)
#define	buttons_iedge_p2	_BIC (P2IES, BUTTON_PIN_P2_IRQ)
#define	button_still_pressed(b) button_pin_status(b)

#endif	/* BUTTON_PRESSED_LOW */

// ============================================================================

#ifdef	BUTTON_PIN_P1_IRQ

#define	buttons_enable_p1	do { \
				    _BIC (P1IFG, BUTTON_PIN_P1_IRQ); \
				    _BIS (P1IE, BUTTON_PIN_P1_IRQ); \
				} while (0)

#define	buttons_disable_p1	_BIC (P1IE, BUTTON_PIN_P1_IRQ)

#define	buttons_init_p1		do { \
					buttons_iedge_p1; \
					_BIC (P1IFG, BUTTON_PIN_P1_IRQ); \
				} while (0)

REQUEST_EXTERNAL (p1irq);
//+++ "p1irq.c"
#else

#define buttons_enable_p1	CNOP
#define	buttons_disable_p1	CNOP
#define	buttons_init_p1		CNOP

#endif	/* P1 */


#ifdef	BUTTON_PIN_P2_IRQ

#define	buttons_enable_p2	do { \
				    _BIC (P2IFG, BUTTON_PIN_P2_IRQ); \
				    _BIS (P2IE, BUTTON_PIN_P2_IRQ); \
				} while (0)

#define	buttons_disable_p2	_BIC (P2IE, BUTTON_PIN_P2_IRQ)

#define	buttons_init_p2		do { \
				    buttons_iedge_p2; \
				    _BIC (P2IFG, BUTTON_PIN_P2_IRQ); \
				} while (0)
REQUEST_EXTERNAL (p2irq);
//+++ "p2irq.c"
#else

#define buttons_enable_p2	CNOP
#define	buttons_disable_p2	CNOP
#define	buttons_init_p2		CNOP

#endif	/* P1 */

#define	buttons_enable()	do { \
					buttons_enable_p1; buttons_enable_p2;\
				} while (0)

#define	buttons_disable()	do { \
					buttons_disable_p1; buttons_disable_p2;\
				} while (0)

#define	buttons_init()		do { \
					buttons_init_p1; buttons_init_p2;\
				} while (0)

#define	button_pressed(b)	button_int_status (b)


#endif
