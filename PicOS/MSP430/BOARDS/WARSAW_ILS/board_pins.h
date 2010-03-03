/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// ============================================================================

// P1:
// 0 = RF SI OUT
// 1 = RF CLK OUT
// 2 = RF SO IN
// 3 = RF GDO2 unused, but must be input
// 4 = RF GDO0 IN
// 5 = RF CSN OUT
// 6 = IR motion sensor input
// 7 = UNUSED, open

#define	PIN_DEFAULT_P1DIR	0xA3
#define	PIN_DEFAULT_P1IE	0x40

// ============================================================================

// P2:
// 0, 1, 7 hang loose
// 2 = IR motion sensor switch
// 4 = soft reset button, must be IN
// 3, 5, 6 general, unused by default
#define	PIN_DEFAULT_P2DIR	0xEF
// IR sensor turned on by default
#define	PIN_DEFAULT_P2OUT	0x04

// ============================================================================

// P3:
// 0 doubles with RXD1 (as CTS, and is in the way, so must be input)
// 1 doubles with TXD0 (and is in the way, so must be input)
// 2 doubles with RXD0 (and is in the way, so must be input)
// 3 doubles with TXD1 ....
// 4 is TXD0 (output)
// 5 is RXD0 (input)
// 6 is TXD1
// 7 is RXD1
#define	PIN_DEFAULT_P3DIR	0x50

// ============================================================================

// P4:
// 1, 2, 3 = LEDs, 0, 4-7 = general unused by default
#define PIN_DEFAULT_P4DIR	0xFF
#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off (high)

// ============================================================================

// P5:
// 0 = EEPROM CS OUT
// 1 = STORAGE SI OUT
// 2 = STORAGE SO IN
// 3 = STORAGE SCK OUT
// 4 = Voltage sensor on (active low)
#define	PIN_DEFAULT_P5DIR	0xFB
#define	PIN_DEFAULT_P5OUT	0x1B	// Default both CS, CLK are up

// P6:
// 5 = light sensor
// 6 = battery sensor
// 7 = analog motion

#define	PIN_DEFAULT_P6DIR	0x00
#define	PIN_DEFAULT_P6SEL	0xE0

#define	PIN_LIST	{	\
	PIN_DEF	(P6, 0),	\
	PIN_DEF	(P6, 1),	\
	PIN_DEF	(P6, 2),	\
	PIN_DEF	(P6, 3),	\
	PIN_DEF	(P6, 4),	\
	PIN_DEF	(P1, 7),	\
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P4, 0),	\
	PIN_DEF	(P4, 4),	\
	PIN_DEF	(P4, 5),	\
	PIN_DEF	(P4, 6),	\
	PIN_DEF	(P4, 7) 	\
}

#define	PIN_MAX			14	// Number of pins
#define	PIN_MAX_ANALOG		5	// Number of available analog pins

// ============================================================================

#include "analog_sensor.h"
#include "sensors.h"
#include "ir_motion_detector.h"

#define	ANA_SEN_SHT		4	// For all analog sensors
#define	ANA_SEN_ISI		0
#define	ANA_SEN_NSA		4
#define	ANA_SEN_URE		SREF_VREF_AVSS
#define	ANA_SEN_ERE		(REFON + REF2_5V)

#define	SENSOR_VINT_PIN		6
#define	SENSOR_LIGHT_PIN	5
#define	SENSOR_IRMTN_PIN	7

#define	irmtn_active		(P1IN & 0x40)
#define	irmtn_signal		(P1IFG & 0x40)
#define	irmtn_clear		_BIC (P1IFG, 0x40)
//+++ "p1irq.c"

#define	irmtn_on()		_BIS (P2OUT, 0x04)
#define	irmtn_off()		_BIC (P2OUT, 0x04)

#include "irq_timer_headers_irmtn.h"

#define	SENSOR_LIST { \
		ANALOG_SENSOR (	ANA_SEN_ISI, 		\
				ANA_SEN_NSA, 		\
				SENSOR_VINT_PIN,	\
				ANA_SEN_URE, 		\
				ANA_SEN_SHT,		\
				ANA_SEN_ERE), 		\
		ANALOG_SENSOR (	ANA_SEN_ISI, 		\
				ANA_SEN_NSA, 		\
				SENSOR_LIGHT_PIN,	\
				ANA_SEN_URE, 		\
				ANA_SEN_SHT,		\
				ANA_SEN_ERE), 		\
		DIGITAL_SENSOR (0, NULL, irmtn_count),	\
		DIGITAL_SENSOR (1, NULL, irmtn_count),	\
		ANALOG_SENSOR (	ANA_SEN_ISI, 		\
				ANA_SEN_NSA, 		\
				SENSOR_IRMTN_PIN,	\
				ANA_SEN_URE, 		\
				ANA_SEN_SHT,		\
				ANA_SEN_ERE) 		\
	}

#define	sensor_adc_prelude(p) \
		do { \
			if (ANALOG_SENSOR_PIN(p) == SENSOR_VINT_PIN) { \
				_BIC (P5OUT, 0x10); \
				mdelay (40); \
			} \
		} while (0);

#define	sensor_adc_postlude(p) \
		do { \
			_BIS (P5OUT, 0x10); \
		} while (0)

#define	SENSOR_ANALOG
#define	SENSOR_DIGITAL

#define	SENSOR_BATTERY		0
#define	SENSOR_LIGHT		1
#define	SENSOR_MOTION		2
#define	SENSOR_MOTION_ANALOG	3

