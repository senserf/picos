/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Includes BlueTooth LinkMatik 2.0 connected via J3:
//
//	GND  ->       (0)
//	P3.7 -> TX    (1)
//	P3.6 -> RX    (2)
// 	P5.6 -> Reset (3)
//	P2.5 -> ESC   (4)
//	P2.6 -> ATTN  (5)
//	Vcc  ->       (9)
//

// ============================================================================
// Pin allocation (Sep 30, 2009):
//
//	J3: 	============= LCD side
//		VUSB	RXUA
//		TXUA  	GND
//		Vcc	Veref+
//		P3.7	P3.6
//		P5.6	P2.5
//		P2.6	P4.0
//		P4.4	P4.5
//		P4.6	Vref-
//		Vcc  	GND
//		=============
//
//	J5:	============= External side
//              GND     P1.6
//		P4.7	P6.7
//              GND	P6.2
//		P6.6	P6.0
//	        GND	P6.4
//	        GND	P6.1
//	        GND	P6.3
//	        GND	P6.5
//		=============


// ============================================================================
// CC1100 on P1
// ============================================================================
// 0 = RF SI OUT
// 1 = RF CLK OUT
// 2 = RF SO IN
// 3 = RF GDO2 unused, but must be input
// 4 = RF GDO0 IN
// 5 = RF CSN OUT
// 6,7 NC
#define	PIN_DEFAULT_P1DIR	(0x23+0xC0)

// P2.0 INT1 RTC
// P2.1 SCL RTC
// P2.2 INT2 RTC
// P2.3 OPTIONS BUTTON
// P2.4 SOFT RESET
// P2.5 Blue ESC
// P2.6 Blue ATTN
// P2.7 SDA RTC
#define	PIN_DEFAULT_P2DIR	0x20	// ESC is OUT

// Soft reset
#define	SOFT_RESET_BUTTON_PRESSED	((P2IN & 0x10) == 0)

// P3.0 POWER Analog SWITCH 4	LCD
// P3.1 POWER Analog SWITCH 2	EE
// P3.2 POWER Analog SWITCH 1	SD
// P3.3 POWER Analog SWITCH 3 	GPS
// P3.4 U0 TX
// P3.5 U0 RX
// P3.6 U1 TX (Blue)
// P3.7 U1 RX (Blue)
#define PIN_DEFAULT_P3DIR	0x5f

// P4.0 LCD  and CONN
// P4.1 LED
// P4.2 LED
// P4.3 LED
// P4.4 LCD  and CONN
// P4.5 LCD  and CONN
// P4.6 LCD  and CONN
// P4.7 NC
#define	PIN_DEFAULT_P4DIR	0x8e

// P5.0 EPR CS
// P5.1 EPR/SD SI
// P5.2 EPR/SD SO
// P5.3 EPR/SD CLK
// P5.4 SEN VRGEN
// P5.5 SD CS
// P5.6 Blue Reset
// P5.7 Voltage sensor ON (LOW!!)
#define	PIN_DEFAULT_P5DIR	0x40

// NC
#define	PIN_DEFAULT_P6DIR	0xFF

// Bluetooth special pins
#define	blue_cmdmode		_BIS (P2OUT,0x20)
#define	blue_datamode		_BIC (P2OUT,0x20)
#define	blue_reset		do { \
					_BIS(P5OUT, 0x40); \
					mdelay (30); \
					_BIC(P5OUT, 0x40); \
				} while (0)
#define	blue_attention		(P2IN & 0x40)


#define	PIN_MAX			0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

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

// ============================================================================
#if 0

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
