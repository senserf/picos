/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// Departures from default pre-initialization

#if CC1100

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

#else

#define	PIN_DEFAULT_P1DIR	0xFF

#endif

// ============================================================================

// 0, 1, 7 hang loose, 4 = soft reset button, must be IN
// 2, 3, 5, 6 general, unused by default
#define	PIN_DEFAULT_P2DIR	0xEF

#define	SOFT_RESET_BUTTON_PRESSED	((P2IN & 0x10) == 0)

// ============================================================================

#if UART_DRIVER > 1
// ============================================================================
// This also means that CTS/RTS are disconnected from RXD1/TXD1, as they should
// be
// ============================================================================
#define	PIN_DEFAULT_P3DIR	0x5F

#else

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
// ============================================================================
#endif

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
//
// 4 = Voltage for ADC resistance test
//
#define	PIN_DEFAULT_P5DIR	0xFB
//#define	PIN_DEFAULT_P5OUT	0x01	// Default CS is up
#define	PIN_DEFAULT_P5OUT	0x0B	// Default both CS, CLK are up

#define	PIN_DEFAULT_P6DIR	0x00
#define	PIN_DEFAULT_P6SEL	0xFF

#define	PIN_MAX			0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0x0000

#include "analog_sensor.h"
#include "sensors.h"
#define	SENSOR_ANALOG

#define	ANA_SEN_SHT		4	// For all analog sensors
#define	ANA_SEN_ISI		0
#define	ANA_SEN_NSA		32
#define	ANA_SEN_URE		SREF_AVCC_AVSS
#define	ANA_SEN_ERE		0

#define	ANA_SEN0_PIN		4
#define	ANA_SEN1_PIN		5
#define	ANA_SEN2_PIN		6
#define	ANA_SEN3_PIN		7

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,		\
		INTERNAL_VOLTAGE_SENSOR,		\
		ANALOG_SENSOR (	ANA_SEN_ISI, 		\
				ANA_SEN_NSA, 		\
				ANA_SEN0_PIN,		\
				ANA_SEN_URE, 		\
				ANA_SEN_SHT,		\
				ANA_SEN_ERE), 		\
		ANALOG_SENSOR (	ANA_SEN_ISI, 		\
				ANA_SEN_NSA, 		\
				ANA_SEN1_PIN,		\
				ANA_SEN_URE, 		\
				ANA_SEN_SHT,		\
				ANA_SEN_ERE), 		\
		ANALOG_SENSOR (	ANA_SEN_ISI, 		\
				ANA_SEN_NSA, 		\
				ANA_SEN2_PIN,		\
				ANA_SEN_URE, 		\
				ANA_SEN_SHT,		\
				ANA_SEN_ERE), 		\
		ANALOG_SENSOR (	ANA_SEN_ISI, 		\
				ANA_SEN_NSA, 		\
				ANA_SEN3_PIN,		\
				ANA_SEN_URE, 		\
				ANA_SEN_SHT,		\
				ANA_SEN_ERE), 		\
	}

#define	N_HIDDEN_SENSORS	2

#define	sensor_adc_prelude(p) \
		do { \
			_BIS (P5OUT, 0x10); \
			mdelay (40); \
		} while (0)

#define	sensor_adc_postlude(p) \
		do { \
			_BIC (P5OUT, 0x10); \
		} while (0)

