/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// P1.0 = LED1 (on high)
// P1.1 = BUTTON1 (presed low, explicit pullup)
// P1.5 = UARTRX, P1.6 = UARTTX

// P2.2 = BAT sensor activator, normally input (open), pull down to measure
// P2.3 = BAT sensor (0.456V when P2.2 pulled down)
// P2.6 = LED2 (on high)
// P2.7 = BUTTON2 (pressed low, explicit pullup)

// P3.6 = LCD_SCL
// P3.7 = LCL_SDA


#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0xDD

#define	PIN_DEFAULT_P2SEL	0x04	// External voltage sensor
#define	PIN_DEFAULT_P2DIR	0x73	// 0,1,4,5 loose, 2o, 3s, 6led, 7but

#define	PIN_DEFAULT_P3DIR	0x3F	// All unused except 6 and 7

// P5.0, P5.1 == LF crystal
#define PIN_DEFAULT_P5SEL	0x03
#define	PIN_DEFAULT_P5DIR	0xFC

#define	BUTTON_LIST 	{ \
				BUTTON_DEF (1, 0x02, 0), \
				BUTTON_DEF (2, 0x80, 0), \
			}

#define	BUTTON_M1	0
#define	BUTTON_M2	1

#define	P1_BUTTONS_INTERRUPT_MASK	0x02
#define	P2_BUTTONS_INTERRUPT_MASK	0x80

#define	BUTTON_DEBOUNCE_DELAY		64
#define	BUTTON_PRESSED_LOW		1

#include "board_rtc.h"

// #define	EXTRA_INITIALIZERS	CNOP

// P2 pins are analog-capable
#define	PIN_LIST	{	\
	PIN_DEF	(P2, 0),	\
	PIN_DEF	(P2, 1),	\
	PIN_DEF	(P2, 4),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P1, 2),	\
	PIN_DEF	(P1, 3),	\
	PIN_DEF	(P1, 4),	\
	PIN_DEF	(P1, 7),	\
	PIN_DEF (P3, 0),	\
	PIN_DEF (P3, 1),	\
	PIN_DEF (P3, 2),	\
	PIN_DEF (P3, 3),	\
	PIN_DEF (P3, 4),	\
	PIN_DEF (P3, 5)		\
}

#define	PIN_MAX		14
#define	PIN_MAX_ANALOG	4
#define	PIN_DAC_PINS	0x00

// ============================================================================

#include "analog_sensor.h"
#include "sensors.h"

#define	SEN_POWER_SHT	4	// 8 cycles = 490us
#define	SEN_POWER_ISI	0	// Inter-sample interval
#define	SEN_POWER_NSA	16	// Samples to average
#define	SEN_POWER_URE	ADC_SREF_RVSS	// Internal

#define	SEN_EPOWER_PIN	3	// External voltage pin
#define	SEN_EPOWER_ERE	ADC_FLG_REFON

#define	SENSOR_LIST { \
		ANALOG_SENSOR (SEN_POWER_ISI,	\
			       SEN_POWER_NSA,	\
			       SEN_EPOWER_PIN,	\
			       SEN_POWER_URE,	\
			       SEN_POWER_SHT,   \
			       SEN_EPOWER_ERE), \
		INTERNAL_TEMPERATURE_SENSOR,	\
		INTERNAL_VOLTAGE_SENSOR,	\
	}
#define	SENSOR_ANALOG
#define	N_HIDDEN_SENSORS	3

#define	sensor_adc_prelude(p) \
			do { \
			    if (ANALOG_SENSOR_PIN (p) == SEN_EPOWER_PIN) { \
				_BIS (P2DIR, 0x02); \
				mdelay (100); \
			    } \
			} while (0)

#define	sensor_adc_postlude(p)  _BIC (P2DIR, 0x02)
