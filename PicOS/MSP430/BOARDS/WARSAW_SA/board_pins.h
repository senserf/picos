/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// ============================================================================
// Radio is on P1
// ============================================================================
// 0 = RF SI OUT
// 1 = RF CLK OUT
// 2 = RF SO IN
// 3 = RF GDO2 unused, but must be input
// 4 = RF GDO0 IN
// 5 = RF CSN OUT
// 6,7 General, unused by default
#define	PIN_DEFAULT_P1DIR	0xE3

// ============================================================================

// 0, 1, 7 hang loose, 4 = soft reset button, must be IN
// 2, 3, 5, 6 general, unused by default
#define	PIN_DEFAULT_P2DIR	0xEF

#define	SOFT_RESET_BUTTON_PRESSED	((P2IN & 0x10) == 0)

// ============================================================================

// 0 doubles with RXD1 (as CTS, and is in the way, so must be input)
// 1 doubles with TXD0 (and is in the way, so must be input)
// 2 doubles with RXD0 (and is in the way, so must be input)
// 3 doubles with TXD1 ....
// 4 is TXD0 (output)
// 5 is RXD0 (input)
// 6 is TXD1
// 7 is RXD1
#define	PIN_DEFAULT_P3DIR	0x50

// 1, 2, 3 = LEDs, 0, 4-7 = general unused by default
#define PIN_DEFAULT_P4DIR	0xFF
#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off (high) by default

// ============================================================================
// P5 used by EEPROM/SDCARD
// ============================================================================
// 0 = EEPROM CS OUT
// 1 = STORAGE SI OUT
// 2 = STORAGE SO IN
// 3 = STORAGE SCK OUT
// 4 = SD card CS OUT
#define	PIN_DEFAULT_P5DIR	0xFB
//#define	PIN_DEFAULT_P5OUT	0x01	// Default CS is up
#define	PIN_DEFAULT_P5OUT	0x0B	// Default both CS, CLK are up

#define	PIN_DEFAULT_P6DIR	0xFF

#define	PIN_MAX			0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0
