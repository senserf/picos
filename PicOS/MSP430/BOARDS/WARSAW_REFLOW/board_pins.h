/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
//	P1.6	= Vcc sen
//	P1.7	= Switch
//	P2.2	= SO
//	P2.3	= CS
//	P2.4	= SCK
//

#define	PIN_DEFAULT_P1DIR	0xC0
#define	PIN_DEFAULT_P2DIR	0x9F	// 0,1 and 7 hang loose
#define	PIN_DEFAULT_P3DIR	0xC9

// EEPROM: 0-CS, 1-SI, 2-SO, 3-SCK
#define	PIN_DEFAULT_P5DIR	0xFB
#define	PIN_DEFAULT_P5OUT	0x09

#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off by default
#define	PIN_DEFAULT_P4DIR	0x8E	//

#define	PIN_DEFAULT_P6DIR	0x00
#define	PIN_DEFAULT_P6SEL	0x00

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

#include "max6675.h"
#include "analog_sensor.h"
#include "sensors.h"

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,	\
		INTERNAL_VOLTAGE_SENSOR,	\
		DIGITAL_SENSOR (0, NULL, max6675_read) \
	}

#define	N_HIDDEN_SENSORS	2

// Pin definitions for SCA3100
#define	max6675_bring_down	do { \
					_BIC (P1OUT, 0x40); \
					_BIS (P2DIR, 0x04); \
					max6675_csel; \
				} while (0)

#define	max6675_bring_up	do { \
					max6675_cunsel; \
					_BIC (P2DIR, 0x04); \
					_BIS (P1OUT, 0x40); \
				} while (0)

#define	max6675_csel		_BIC (P2OUT, 0x08)
#define	max6675_cunsel		_BIS (P2OUT, 0x08)

#define	max6675_clkh		_BIS (P2OUT, 0x10)
#define	max6675_clkl		_BIC (P2OUT, 0x10)

#define	max6675_data		(P2IN & 0x04)

#define	SENSOR_ANALOG		// To make sure analog sensors are processed
#define	SENSOR_DIGITAL		// To make sure digital sensors are processed

#define	owen_on()		_BIS (P1OUT, 0x80)
#define	owen_off()		_BIC (P1OUT, 0x80)
