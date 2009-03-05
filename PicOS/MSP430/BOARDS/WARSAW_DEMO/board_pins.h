/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Departures from default pre-initialization

// 0 = RF SI OUT
// 1 = RF CLK OUT
// 2 = RF SO IN
// 3 = RF GDO2 unused, but must be input
// 4 = RF GDO0 IN
// 5 = RF CSN OUT
// 6,7 General, unused by default
#define	PIN_DEFAULT_P1DIR	0xE3

// P2.3-6 used by buttons (should be in); 0, 1, 7 hang loose; 2 available?
#define	PIN_DEFAULT_P2DIR	0x87

// 0 doubles with RXD1 (as CTS, and is in the way, so must be input)
// 1 doubles with TXD0 (and is in the way, so must be input)
// 2 doubles with RXD0 (and is in the way, so must be input)
// 3 doubles with TXD1 ....
// 4 is TXD0 (output)
// 5 is RXD0 (input)
// 6 is TXD1
// 7 is RXD1
#define	PIN_DEFAULT_P3DIR	0x50
//#define	PIN_DEFAULT_P3DIR	0xC9

// 1, 2, 3 = LEDs, 0, 4 = buttons, 5-7 general
#define PIN_DEFAULT_P4DIR	0xEE
#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off (high) by default

// 0 = STORAGE CS OUT
// 1 = STORAGE SI OUT
// 2 = STORAGE SO IN
// 3 = STORAGE SCK OUT
// 4 = general and unused by default, 5, 6, 7 hang loose
#define	PIN_DEFAULT_P5DIR	0xFB
//#define	PIN_DEFAULT_P5OUT	0x01	// Default CS is up
#define	PIN_DEFAULT_P5OUT	0x0B	// Default CS is up

// P6.3-6 used by the LCD: intialization in board_lcd.h

#define	RESET_ON_KEY_PRESSED	((P2IN & 0x10) == 0)

#define	PIN_DEFAULT_P6DIR	0xFF

#define	PIN_LIST	{	\
	PIN_DEF	(P6, 0),	\
	PIN_DEF	(P6, 1),	\
	PIN_DEF	(P6, 2),	\
	PIN_DEF	(P6, 7),	\
	PIN_DEF	(P1, 6),	\
	PIN_DEF	(P1, 7),	\
	PIN_DEF	(P2, 2),	\
	PIN_DEF	(P4, 5),	\
	PIN_DEF	(P4, 6),	\
	PIN_DEF	(P4, 7),	\
	PIN_DEF	(P5, 4),	\
}

#define	PIN_MAX			11	// Number of pins
#define	PIN_MAX_ANALOG		4	// Number of available analog pins
#define	PIN_DAC_PINS		0x00

#define	P2_PINS_INTERRUPT_MASK	0x78

// Only the first four buttons for now

#define	BUTTON_LIST	{	\
		BUTTON_DEF (2, 0x08, 0), \
		BUTTON_DEF (2, 0x10, 0), \
		BUTTON_DEF (2, 0x20, 0), \
		BUTTON_DEF (2, 0x40, 0)  \
	}

#define	BUTTON_0	0
#define	BUTTON_1	0
#define	BUTTON_2	0
#define	BUTTON_3	0

#define BUTTON_PRESSED_LOW	1
