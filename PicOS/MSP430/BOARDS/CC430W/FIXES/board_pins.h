/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// These are the "correct" definitions for the fixed version of the board
// 111012

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
//	4 - CONN	J7.11	[VREF-]
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
#define	PIN_DEFAULT_P4OUT	0xE8

// P5:
//	0 - 32K crystal
//	1 - 32K crystal
//	2 - CMA3000	CSB
//	3 - CONN	J7.6
//	4 - CONN	J7.5
//	5 - CMA3000	SCL
//	6 - CMA3000	MOSI SDA ->sensor
//	7 - CMA3000	MISO	 <-sensor
#define	PIN_DEFAULT_P5SEL	0x03
// Note: bits 0 and 1 on DIR MUST be zero (DIR must be IN) for the crystal to
// work!!!!
#define	PIN_DEFAULT_P5DIR	0xE4
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
	PIN_DEF	(P2, 4),	\
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

#define	PIN_MAX		18
#define	PIN_MAX_ANALOG	6
#define	PIN_DAC_PINS	0x00

#define	CSWITCH_EE	2
#define	CSWITCH_EXT	4

#define	cswitch_on(p)	_BIS (P4OUT, p)
#define	cswitch_off(p)	_BIC (P4OUT, p)
