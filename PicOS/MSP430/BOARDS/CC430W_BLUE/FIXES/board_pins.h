/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// These are the "correct" definitions for the fixed version of the board
// 111012


// ============================================================================
// Module connected to the primary UART through the on-board switch
// The remaining pins:
//
//
// LinkMatik:
//	ESC	- P5.4
//	ATN	- P2.4
//	RESET	- P5.3
//
// BTM-182:
//	PIO 4	- P5.4
//	PIO 6	- P2.4
//	RESET	- P5.3
//
// BOLUTEK:
//	PIO 3	- P5.4
//	PIO 2	- P2.4
//		  P5.3 is unused
// ============================================================================

#ifdef	BT_MODULE_LINKMATIK
#define	__P5DIR_SUPPL	0x18
#endif

#ifdef	BT_MODULE_BTM182
#define	__P5DIR_SUPPL	0x18
#endif

#ifdef	BT_MODULE_BOLUTEK
// No RESET pin
#define	__P5DIR_SUPPL	0x10
#endif

#ifndef	__P5DIR_SUPPL
#error "You have to define exactly one of: BT_MODULE_LINKMATIK, BT_MODULE_BTM182, BT_MODULE_BOLUTEK!!!"
#endif


// P1:
//	0 - CONN	J8.2	IR PIR
//	1 - CONN	J8.4	IR TAMP
//	2 - EEPROM	SO	[UCB0SOMI]
//	3 - EEPROM	SI	[UCB0SIMO]
//	4 - EEPROM	SCK	[UCB0CLK]
//	5 - UARTRX		[UCA0RX]	CONN	J7.13
//	6 - UARTTX		[UCA0TX]	CONN	J7.12
//	7 - INT		CMA3000
#define	PIN_DEFAULT_P1DIR	0x58
#define	PIN_DEFAULT_P1SEL	0x60

// P2:
//	0 - CONN	J6.14
//	1 - BUTTON + SHUNT (default high)
//	2 - CONN	J6.12
//	3 - CONN	J6.11
//	4 - CONN	J7.11	[VREF-]		!!! LM ATN
//	5 - CONN	J7.10	[VEREF+]
//	6 - CONN	J7.4
//	7 - EXT VOL SEN		[ADC A7]
#define	PIN_DEFAULT_P2DIR	0x00
#define	PIN_DEFAULT_P2SEL	0x80

// P3:
//	0 - CONN	J6.10
//	1 - CONN	J6.9
//	2 - CONN	J6.8
//	3 - CONN	J6.7
//	4 - CONN	J6.6
//	5 - CONN	J6.5
//	6 - CONN	J6.4
//	7 - CONN	J6.3

#define	PIN_DEFAULT_P3DIR	0x00

// P4:
//	0 - EEPROM	CS
//	1 - ASWITCH	EEPROM
//	2 - ASWITCH	XX Vcc	J7.3
//	3 - VOL SEN ON	OFF == HIGH
//	4 - CMA3000 VDD
//	5 - LED Y	OFF == HIGH
//	6 - LED G	OFF == HIGH
//	7 - LED R	OFF == HIGH
#define	PIN_DEFAULT_P4DIR	0xF7
#define	PIN_DEFAULT_P4OUT	0xE0

// P5:
//	0 - 32K crystal
//	1 - 32K crystal
//	2 - CMA3000	CSB
//	3 - CONN	J7.6			!!! LM RESET
//	4 - CONN	J7.5			!!! LM ESC
//	5 - CMA3000	SCL
//	6 - CMA3000	MOSI SDA ->sensor
//	7 - CMA3000	MISO	 <-sensor
#define	PIN_DEFAULT_P5SEL	0x03
// Note: bits 0 and 1 on DIR MUST be zero (DIR must be IN) for the crystal to
// work!!!!
#define	PIN_DEFAULT_P5DIR	(0xE4 + __P5DIR_SUPPL)
#define	PIN_DEFAULT_P5OUT	0xE4

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
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P1, 0),	\
	PIN_DEF	(P1, 1),	\
	PIN_DEF (P3, 0),	\
	PIN_DEF (P3, 1),	\
	PIN_DEF (P3, 2),	\
	PIN_DEF (P3, 3),	\
	PIN_DEF (P3, 4),	\
	PIN_DEF (P3, 5),	\
	PIN_DEF (P3, 6),	\
	PIN_DEF (P3, 7),	\
	PIN_DEF (P5, 3),	\
	PIN_DEF (P5, 4)		\
}

#define	PIN_MAX		17
#define	PIN_MAX_ANALOG	5
#define	PIN_DAC_PINS	0x00

#define	CSWITCH_EE	2
#define	CSWITCH_EXT	4

#define	cswitch_on(p)	_BIS (P4OUT, p)
#define	cswitch_off(p)	_BIC (P4OUT, p)

#define	cswitch_on(p)	_BIS (P4OUT, p)
#define	cswitch_off(p)	_BIC (P4OUT, p)

#ifdef	BT_MODULE_LINKMATIK
#define	blue_reset_set		_BIS (P5OUT,0x08)
#define	blue_reset_clear	_BIC (P5OUT,0x08)
#define	blue_escape_set		_BIS (P5OUT,0x10)
#define	blue_escape_clear	_BIC (P5OUT,0x10)
#define	blue_ready		(P2IN & 0x10)
#endif

#ifdef	BT_MODULE_BTM182
#define	blue_reset_set		_BIS (P5OUT,0x08)
#define	blue_reset_clear	_BIC (P5OUT,0x08)
#define	blue_escape_set		_BIS (P5OUT,0x10)
#define	blue_escape_clear	_BIC (P5OUT,0x10)
#define	blue_ready		((P2IN & 0x10) == 0)
#endif

#ifdef	BT_MODULE_BOLUTEK
// No RESET pin
#define	blue_reset_set		blue_escape_set
#define	blue_reset_clear	blue_escape_clear
#define	blue_escape_set		_BIS (P5OUT,0x10)
#define	blue_escape_clear	_BIC (P5OUT,0x10)
#define	blue_ready		(P2IN & 0x10)
#endif

#define	blue_power_up		cswitch_on (CSWITCH_EXT)
#define	blue_power_down		cswitch_off (CSWITCH_EXT)
