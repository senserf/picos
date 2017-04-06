/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

// P1.5 = UARTRX, P1.6 = UARTTX, P1.1 = BUTTON (pressed low, explicit pullup)
// P1.4 = MAX30102 INT, active low, pulled up externally
// P1.0 = LED (on high)
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0xCD
#define	PIN_DEFAULT_P1IES	0x10

// P2.1 = MAX30102 SDA (open drain), pulled up externally
// P2.0 = MAX30102 SCL pulled up externally
#define	PIN_DEFAULT_P2DIR	0xFC
#define	PIN_DEFAULT_P2OUT	0x00

// P3.6 = LED (on high); no need to change default setting for the pin

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

// ============================================================================

#include "max30102.h"

//#define	max30102_i2c_delay	udelay (20)

#define	max30102_i2c_delay	CNOP

#define	max30102_sda_lo		do { _BIS (P2DIR, 0x02); max30102_i2c_delay; } \
					while (0)

#define	max30102_sda_hi		do { _BIC (P2DIR, 0x02); max30102_i2c_delay; } \
					while (0)

#define	max30102_scl_lo		do { _BIS (P2DIR, 0x01); max30102_i2c_delay; } \
					while (0)

#define	max30102_scl_hi		do { _BIC (P2DIR, 0x01); max30102_i2c_delay; } \
					while (0)

#define	max30102_sda		(P2IN & 0x02)
#define	max30102_data		((P1IN & 0x10) == 0)
#define	max30102_event		((aword)(max30102_read_sample))

#define	max30102_enable		do { \
					_BIS (P1IE, 0x10); \
					if (max30102_data) \
						_BIS (P1IFG, 0x10); \
				} while (0)

#define	max30102_disable	_BIC (P1IE, 0x10)

#define	max30102_int		(P1IFG & 0x10)
#define	max30102_clear		_BIC (P1IFG, 0x10)

// ============================================================================

#include "board_rtc.h"

// #define	EXTRA_INITIALIZERS	CNOP

#if 0

// P2 pins are analog-capable
#define	PIN_LIST	{	\
	PIN_DEF	(P2, 2),	\
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 4),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P2, 7),	\
	PIN_DEF	(P1, 2),	\
	PIN_DEF	(P1, 3),	\
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
#define	PIN_MAX_ANALOG	6
#define	PIN_DAC_PINS	0x00

#endif

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
