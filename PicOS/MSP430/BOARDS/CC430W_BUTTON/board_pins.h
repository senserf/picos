/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// ============================================================================
// P1.0 - button, GND when pressed, needs pullup
// P1.1 - CONN J3-4
// P1.2 - NC
// P1.3 - NC
// P1.4 - CONN J3-5
// P1.5 - UART RX
// P1.6 - UART TX
// P1.7 - CMA3000 INT
#define	PIN_DEFAULT_P1DIR	0x5E
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1REN	0x81
#define	PIN_DEFAULT_P1OUT	0x01
// ============================================================================
// P2.0 - CONN J3-8
// P2.1 - button, GND when pressed (pulled up)
// P2.2 - CONN J4-1
// P2.3 - CONN J4-2
// P2.4 - CONN J4-3
// P2.5 - caps for Vref
// P2.6 - CONN J4-4
// P2.7 - CONN J4-5
#define	PIN_DEFAULT_P2DIR	0xFD
// ============================================================================
// P3.0 - CONN J2-8
// P3.1 - CONN J2-9
// P3.2 - CONN J2-10
// P3.3 - CONN J2-11
// P3.4 - CONN J2-12
// P3.5 - CONN J2-13
// P3.6 - CONN J2-14
// P3.7 - CONN J3-2
#define	PIN_DEFAULT_P3DIR	0xFF
// ============================================================================
// P4.0 - CONN J2-2
// P4.1 - CONN J2-2
// P4.2 - CONN J2-2
// P4.3 - CONN J2-2
// P4.4 - CMA3000 PWR
// P4.5 - CONN J2-2
// P4.6 - LED ON-GND
// P4.7 - CONN J2-2
#define	PIN_DEFAULT_P4DIR	0xFF
#define	PIN_DEFAULT_P4OUT	0x40
// ============================================================================
// P5.0 - Crystal
// P5.1 - Crystal
// P5.2 - CMA3000 CSB
// P5.3 - NC
// P5.4 - NC
// P5.5 - CMA3000 MISO
// P5.6 - CMA3000 SCK
// P5.7 - CMA3000 MOSI
// ============================================================================
#define	PIN_DEFAULT_P4SEL	0x03
#define	PIN_DEFAULT_P5DIR	0xFC
#define	PIN_DEFAULT_P5OUT	0x00

#define	SOFT_RESET_BUTTON_PRESSED	((P2IN & 0x02) == 0)

// ============================================================================

#include "board_sensors.h"
#include "board_rtc.h"

#define	EXTRA_INITIALIZERS	do { \
					__pi_rtc_init (); \
				} while (0)

// P2 pins are analog-capable
#define	PIN_LIST	{	\
	PIN_DEF	(P2, 0),	\
	PIN_DEF	(P2, 2),	\
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 4),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P2, 7),	\
	PIN_DEF	(P1, 1),	\
	PIN_DEF	(P1, 4),	\
	PIN_DEF (P3, 0),	\
	PIN_DEF (P3, 1),	\
	PIN_DEF (P3, 2),	\
	PIN_DEF (P3, 3),	\
	PIN_DEF (P3, 4),	\
	PIN_DEF (P3, 5),	\
	PIN_DEF (P3, 6),	\
	PIN_DEF (P3, 7),	\
	PIN_DEF (P4, 0),	\
	PIN_DEF (P4, 1),	\
	PIN_DEF (P4, 2),	\
	PIN_DEF (P4, 3),	\
	PIN_DEF (P4, 5),	\
	PIN_DEF (P4, 7),	\
}

#define	PIN_MAX		22
#define	PIN_MAX_ANALOG	6
#define	PIN_DAC_PINS	0x00
