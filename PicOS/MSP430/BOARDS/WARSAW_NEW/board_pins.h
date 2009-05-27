/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// ============================================================================
// Pin allocation (Feb 23, 2009):
// ============================================================================
// P1.0 RF RSI
// P1.1 RF RSCLK
// P1.2 RF RSO
// P1.3 RF GDO2                                            
// P1.4 RF GD02
// P1.5 RF CSN
// ------------------------------------
// P1.6 SEN SHT DATA        
// P1.7 SEN SHT CLK           
// ------------------------------------
// P2.0 INT1 RTC
// P2.1 SCL RTC
// P2.2 INT2 RTC
// P2.3 OPTIONS BUTTON
// P2.4 SOFT RESET
// P2.5 CONN
// P2.6 CONN
// P2.7 SDA RTC
// ------------------------------------
// P3.0 POWER Analog SWITCH 4	LCD
// P3.1 POWER Analog SWITCH 2	EE
// P3.2 POWER Analog SWITCH 1	SD
// P3.3 POWER Analog SWITCH 3 	GPS
// P3.4 U0 TX
// P3.5 U0 RX
// P3.6 U1 TX GPS   and CONN
// P3.7 U1 RX GPS  and CONN
// ------------------------------------
// P4.0 LCD  and CONN
// P4.1 LED
// P4.2 LED
// P4.3 LED
// P4.4 LCD  and CONN
// P4.5 LCD  and CONN
// P4.6 LCD  and CONN
// P4.7 SEN ECHxx EXC
// ------------------------------------
// P5.0 EPR CS
// P5.1 EPR/SD SI
// P5.2 EPR/SD SO
// P5.3 EPR/SD CLK
// P5.4 SEN VRGEN
// P5.5 SD CS
// P5.6 GPS ENABLE  and CONN
// P5.7 SENS's (Detection) 
// ------------------------------------
// P6.0 SEN PAR#1
// P6.1 SEN PAR#2 / PYR#2 (Detection)
// P6.2 SEN ECHxx (Detection)
// P6.3 SEN PAR#2
// P6.4 SEN PYR#1
// P6.5 SEN PYR#2 
// P6.6 SEN PAR#1 / PYR#1 (Detection)                                           
// P6.7 SEN ECHxx
// ============================================================================

//
// Power measurements using MSP-TS430PM64 socket board
//
//	freeze current = 7.7uA DC, 11.22uA RMS
//	it is about 10uA with RTC included
//	active current (CPU idling, components switched off) = 284uA DC/RMS
//	EEPROM on but idle = 361uA
//	EEPROM writing = 7.5mA, reading = 5mA
//	SD open, idle = 5mA, WR = 24.3mA, RD = 23mA
//	
// Tested resistor in UARTs RX/DX - to avoid parasite powering of the board
// when Vcc goes down. 2.2K on both is OK (at 1152000), but 22K is too much
// for TX (seems OK for RX).
// Use 2.2K for RX.
//
// Note: unitialized RTC drains about 150uA!!!!
//
// Note: delays in lcd_st7036.c may have to be adjusted if the display doesn't
// work (I have reduced them drastically)
//

// ============================================================================
// Radio is on P1
// ============================================================================
// 0 = RF SI OUT
// 1 = RF CLK OUT
// 2 = RF SO IN
// 3 = RF GDO2 unused, but must be input
// 4 = RF GDO0 IN
// 5 = RF CSN OUT

#define	PIN_DEFAULT_P1DIR	0x23

// RTC on P2: all pins input (grounded when set to OUT) as we operate in open
// drain mode
#define	PIN_DEFAULT_P2DIR	0x00

// Soft reset
#define	RESET_ON_KEY_PRESSED	((P2IN & 0x10) == 0)

// All four switches start off
#define PIN_DEFAULT_P3DIR	0x5f

// ============================================================================
// Second UART (for GPS) is initially OFF; we cannot initialize it at the
// beginning and keep it this way because it is shared with SPI (for SD and
// EEPROM)
//
// This will make sure the second UART is not pre-inited
//
#undef	UART_PREINIT_B

// I am not sure if we need a "driver" for the GPS module, which simply appears
// as a dynamically configurable UART with (basically) ASCII output. What we
// seem to be needing, though, is a nice way to dynamically configure the
// UART

// 4800 bps
#if UART_CLOCK_RATE == 32768
#define	GPS_UBR0	0x06
#define	GPS_UBR1	0
#define	GPS_UMCTL	0x6f
#else
#define	GPS_UMCTL		0
#define	GPS_UBR0		((UART_CLOCK_RATE/4800) % 256)
#define	GPS_UBR1		((UART_CLOCK_RATE/4800) / 256)
#endif

#if 0
#define	gps_bring_up	do { \
				_BIC (P5OUT, 0x40); \
				cswitch_on (CSWITCH_GPS); \
				_BIS (P3SEL, 0xc0); \
				_BIS (UCTL1, SWRST); \
				UTCTL1 = UART_UTCTL; \
				UBR01 = GPS_UBR0; \
				UBR11 = GPS_UBR1; \
				UMCTL1 = GPS_UMCTL; \
				_BIS (IFG2, UTXIFG1); \
				_BIS (ME2, UTXE1 + URXE1); \
				_BIS (UCTL1, CHAR); \
				_BIC (UCTL1, SYNC + MM); \
				_BIC (UCTL1, SWRST); \
				mdelay (10); \
				_BIS (P5OUT, 0x40); \
				zz_uart [1] . flags = 0; \
			} while (0)
#endif

#define	gps_bring_up	do { \
				_BIC (P5OUT, 0x40); \
				cswitch_on (CSWITCH_GPS); \
				_BIS (P3SEL, 0xc0); \
				_BIS (UCTL1, SWRST); \
				UTCTL1 = UART_UTCTL; \
				UBR01 = GPS_UBR0; \
				UBR11 = GPS_UBR1; \
				UMCTL1 = GPS_UMCTL; \
				_BIS (IFG2, UTXIFG1); \
				_BIS (ME2, UTXE1 + URXE1); \
				_BIS (UCTL1, CHAR); \
				_BIC (UCTL1, SYNC + MM + SWRST); \
				mdelay (10); \
				_BIS (P5OUT, 0x40); \
				zz_uart [1] . flags = 0; \
			} while (0)

#define	gps_bring_down	do { \
				_BIC (P5OUT, 0x40); \
				mdelay (10); \
				_BIS (UCTL1, SWRST); \
				cswitch_off (CSWITCH_GPS); \
				_BIC (IFG2, UTXIFG1); \
				_BIC (ME2, UTXE1 + URXE1); \
				_BIC (P3SEL, 0xc0); \
				_BIC (UCTL1, SWRST); \
			} while (0)

// ============================================================================

#define	PIN_DEFAULT_P4DIR	0x0e

// LEDs initially OFF
#define	PIN_DEFAULT_P4OUT	0x0e

// P5 = EEPROM/SD; default is input (high Z) for power down; P5.6 GPS;
#define	PIN_DEFAULT_P5DIR	0x40

#define	PIN_DEFAULT_P6DIR	0x00

#define	PIN_LIST	{	\
	PIN_DEF (P6, 0),	\
	PIN_DEF (P6, 1),	\
	PIN_DEF (P6, 2),	\
	PIN_DEF (P6, 3),	\
	PIN_DEF (P6, 4),	\
	PIN_DEF (P6, 5),	\
	PIN_DEF (P6, 6),	\
	PIN_DEF	(P2, 7),	\
	PIN_DEF	(P2, 6), 	\
}

#define	PIN_MAX			10	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0x0706	// Two DAC pins: #6 and #7

//
// Component power switch (on P3)
//
#define	CSWITCH_LCD		1	// P3.0
#define CSWITCH_EE		2	// P3.1
#define	CSWITCH_SD		4	// P3.2
#define	CSWITCH_GPS		8	// P3.3

#define	cswitch_on(p)		_BIS (P3OUT, (p) & 0x0f)
#define	cswitch_off(p)		_BIC (P3OUT, (p) & 0x0f)

#define	CSWITCH_ALL	(CSWITCH_LCD + CSWITCH_EE + CSWITCH_SD + CSWITCH_GPS)

#include "board_sensors.h"
