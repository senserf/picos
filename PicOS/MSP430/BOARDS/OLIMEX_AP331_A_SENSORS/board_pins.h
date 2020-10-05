/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// P1.5 = UARTRX, P1.6 = UARTTX, P1.1 = BUTTON (pressed low, explicit pullup)
// P1.0 = LED (on high)
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0xDD

// P3.6 = LED (on high); no need to change default setting for the pin

// P2.4/2.6 analog input
#define	PIN_DEFAULT_P2SEL		0x70
#define	PIN_DEFAULT_P2DIR		0x80
#define	PIN_DEFAULT_P2OUT		0x00
#define	PIN_DEFAULT_P2REN		0x00

// P3.1 = Battery voltage sensor on (pull down)
#define	PIN_DEFAULT_P3OUT		0x00
#define	PIN_DEFAULT_P3DIR		0xFD
#define	PIN_DEFAULT_P3REN		0x00

// P5.0, P5.1 == LF crystal
#define PIN_DEFAULT_P5SEL	0x03
#define	PIN_DEFAULT_P5DIR	0xFC

#define	BUTTON_LIST 	{ \
				BUTTON_DEF (1, 0x02, 0), \
			}

#define	BUTTON_M1	0

#define	BUTTON_PIN_P1_IRQ	0x02
#define	BUTTON_DEBOUNCE_DELAY	64
#define	BUTTON_PRESSED_LOW	1

#include "board_rtc.h"

// #define	EXTRA_INITIALIZERS	CNOP

// P2 pins are analog-capable
#define	PIN_LIST	{	\
	PIN_DEF	(P2, 0),	\
	PIN_DEF	(P2, 1),	\
	PIN_DEF	(P2, 2),	\
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 7),	\
	PIN_DEF	(P1, 2),	\
	PIN_DEF	(P1, 3),	\
	PIN_DEF	(P1, 4),	\
	PIN_DEF	(P1, 7),	\
	PIN_DEF (P3, 0),	\
	PIN_DEF (P3, 1),	\
	PIN_DEF (P3, 2),	\
	PIN_DEF (P3, 3),	\
	PIN_DEF (P3, 4),	\
	PIN_DEF (P3, 5),	\
	PIN_DEF (P3, 7)		\
}

#define	PIN_MAX		16
#define	PIN_MAX_ANALOG	5
#define	PIN_DAC_PINS	0x00

// ============================================================================
#if 1

#include "analog_sensor.h"
#include "sensors.h"

// Portmapper for the sensor pins

#define	PIN_PORTMAP	{ 	portmap_entry (P2MAP0, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_ANALOG, \
						PM_ANALOG, \
						PM_ANALOG, \
						PM_NONE) \
			}

// Reference doesn't go out
#define	ADC_REFERENCE_OUT	0

#define	CHA_SEN_PIN		4
#define	CHA_SEN_SHT		4
#define	CHA_SEN_ISI		1
#define	CHA_SEN_NSA		64
// Reference to Vss
#define	CHA_SEN_URE		ADC_SREF_VVSS
#define	CHA_SEN_ERE		0

#define	BAT_SEN_PIN		5
#define	BAT_SEN_SHT		4
#define	BAT_SEN_ISI		1
#define	BAT_SEN_NSA		64
#define	BAT_SEN_URE		ADC_SREF_VVSS
#define	BAT_SEN_ERE		0
#if 0
// Reference to Internal 2.5V
#define	BAT_SEN_URE		ADC_SREF_RVSS
#define	BAT_SEN_ERE		ADC_FLG_REF25 | ADC_FLG_REFON
#endif

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,  \
		INTERNAL_VOLTAGE_SENSOR,      \
		ANALOG_SENSOR ( CHA_SEN_ISI,  \
				CHA_SEN_NSA,  \
				CHA_SEN_PIN,  \
				CHA_SEN_URE,  \
				CHA_SEN_SHT,  \
				CHA_SEN_ERE), \
		ANALOG_SENSOR ( BAT_SEN_ISI,  \
				BAT_SEN_NSA,  \
				BAT_SEN_PIN,  \
				BAT_SEN_URE,  \
				BAT_SEN_SHT,  \
				BAT_SEN_ERE), \
	}

#define	sensor_adc_prelude(p) \
		do { \
			if (ANALOG_SENSOR_PIN (p) == BAT_SEN_PIN) { \
				_BIS (P3DIR, 0x02); \
				mdelay (20); \
			} \
		} while (0);

#define	sensor_adc_postlude(p)	_BIC (P3DIR, 0x02)


#define	SENSOR_ANALOG
#define	N_HIDDEN_SENSORS	2
#define	SENSOR_CHARGE		0
#define	SENSOR_BATTERY		1

#endif
// ============================================================================

