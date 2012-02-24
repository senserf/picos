/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Includes BlueTooth LinkMatic 2.0 and BTM-182
//
//
//	MSP430  LM		BTM-182
// 	======  ===========     ======================================
//	P3.7 -> TX    (1)	TX (pulled up)
//	P3.6 -> RX    (2)	RX
// 	P1.7 -> Reset (3)	Reset
//	P1.6 -> ESC   (4)	PIO 4 (causes disconnection when high)
//	P6.7 -> ATTN  (5)	PIO 6 (low if connected)
//	P5.4 -> Power supply (optional)
//

// ============================================================================
// CC1100 on P1
// ============================================================================
// 0 = RF SI OUT
// 1 = RF CLK OUT
// 2 = RF SO IN
// 3 = RF GDO2 unused, but must be input
// 4 = RF GDO0 IN
// 5 = RF CSN OUT
// 6 = BT ESC OUT
// 7 = BT RESET OUT

#ifdef	LINKMATIC
// This is for backward compatibility, I am assuming we are retiring LinkMatic,
// so this constant is undefined by default
#define	PIN_DEFAULT_P1DIR	(0x23+0xC0)
#else
// Reset on BTM-182 is input/open
#define	PIN_DEFAULT_P1DIR	(0x23+0x40)
#endif

// 0, 1, 7 hang loose, 4 = soft reset button, must be IN
// 2, 3, 5, 6 general, unused by default
#define	PIN_DEFAULT_P2DIR	0xEF

// This also means that CTS/RTS are disconnected from RXD1/TXD1, as they should
// be
#define	PIN_DEFAULT_P3DIR	0x50

// Soft reset
#define	SOFT_RESET_BUTTON_PRESSED	((P2IN & 0x10) == 0)

// 1, 2, 3 = LEDs, 0, 4-6 = general unused by default, 7 = power for BT
#define PIN_DEFAULT_P4DIR	0xFF
#define	PIN_DEFAULT_P4OUT	0x8E	// LEDs off (high) by default

// ============================================================================
// P5 used by EEPROM
// ============================================================================
// 0 = EEPROM CS OUT
// 1 = EEPROM SI OUT
// 2 = EEPROM SO IN
// 3 = EEPROM SCK OUT
// 4 = SD Card CS OUT
#define	PIN_DEFAULT_P5DIR	0xFB
//#define	PIN_DEFAULT_P5OUT	0x01	// Default CS is up
#define	PIN_DEFAULT_P5OUT	0x0B	// Default both CS, CLK are up

#define	PIN_DEFAULT_P6DIR	0x7F

#define	PIN_LIST	{	\
	PIN_DEF	(P6, 0),	\
	PIN_DEF	(P6, 1),	\
	PIN_DEF	(P6, 2),	\
	PIN_DEF	(P6, 3),	\
	PIN_DEF	(P6, 4),	\
	PIN_DEF	(P6, 5),	\
	PIN_DEF	(P6, 6),	\
	PIN_DEF	(P2, 2),	\
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P4, 0),	\
	PIN_DEF	(P4, 4),	\
	PIN_DEF	(P4, 5),	\
	PIN_DEF	(P4, 6)		\
}

#define	PIN_MAX			15	// Number of pins
#define	PIN_MAX_ANALOG		7	// Number of available analog pins
#define	PIN_DAC_PINS		0x0000

// ============================================================================
#if 1

#include "analog_sensor.h"
#include "sensors.h"

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,	\
		INTERNAL_VOLTAGE_SENSOR		\
	}
#define	SENSOR_ANALOG
#define	N_HIDDEN_SENSORS	2

#endif
// ============================================================================

// Bluetooth special pins, polarity may differ depending on the module
#define	blue_escape_set		_BIS (P1OUT,0x40)
#define	blue_escape_clear	_BIC (P1OUT,0x40)
#define	blue_power_up		_BIS (P4OUT,0x80)
#define	blue_power_down		_BIC (P4OUT,0x80)
#define	blue_status		(P6IN & 0x80)

#ifdef LINKMATIC
#define	blue_reset_set		_BIS (P1OUT,0x80)
#define	blue_reset_clear	_BIC (P1OUT,0x80)
#else
#define	blue_reset_set		_BIS (P1DIR,0x80)
#define	blue_reset_clear	_BIC (P1DIR,0x80)
#endif
