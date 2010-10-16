/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Includes BlueTooth LinkMatic 2.0
//
//	P3.7 -> TX    (1)
//	P3.6 -> RX    (2)
// 	P1.7 -> Reset (3)
//	P1.6 -> ESC   (4)
//	P6.7 -> ATTN  (5)
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
#define	PIN_DEFAULT_P1DIR	(0x23+0xC0)

// 0, 1, 7 hang loose, 4 = soft reset button, must be IN
// 2, 3, 5, 6 general, unused by default
#define	PIN_DEFAULT_P2DIR	0xEF

// This also means that CTS/RTS are disconnected from RXD1/TXD1, as they should
// be
#define	PIN_DEFAULT_P3DIR	0x50

// Soft reset
#define	SOFT_RESET_BUTTON_PRESSED	((P2IN & 0x10) == 0)

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
	PIN_DEF	(P2, 4),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P4, 0),	\
	PIN_DEF	(P4, 4),	\
	PIN_DEF	(P4, 5),	\
	PIN_DEF	(P4, 6),	\
	PIN_DEF	(P4, 7),	\
	PIN_DEF (P5, 4)		\
}

#define	PIN_MAX			18	// Number of pins
#define	PIN_MAX_ANALOG		7	// Number of available analog pins
#define	PIN_DAC_PINS		0x0000

// Bluetooth special pins
#define	blue_cmdmode		_BIS (P1OUT,0x40)
#define	blue_datamode		_BIC (P1OUT,0x40)
#define	blue_reset		do { \
					_BIS(P1OUT, 0x80); \
					mdelay (30); \
					_BIC(P1OUT, 0x80); \
				} while (0)
#define	blue_attention		(P6IN & 0x80)





