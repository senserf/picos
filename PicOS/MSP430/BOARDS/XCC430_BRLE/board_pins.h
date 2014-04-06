/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

// P1.5 = UARTRX, P1.6 = UARTTX, P1.7 = BUTTON (pressed low, requires pullup)
// P1.0 = LED (on high)
//
// P1.1 = INT1 (BMA250)
// P1.2 = INT2 (BMA250)
//
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0x5F

// Internal pullup for P1.7
#define	PIN_DEFAULT_P1OUT	0x00
#define	PIN_DEFAULT_P1REN	0x00

// P2.2 BRLE PIO_2	(status indicator)
#define	PIN_DEFAULT_P2DIR	0xFB
#define	PIN_DEFAULT_P2OUT	0xFB

// P3.0 BMA250 PS
// P3.5 BRLE PIO_3	(sleep mode toggle)
// P3.6 = LED (on high); no need to change default setting for the pin
#define	PIN_DEFAULT_P3DIR	0xFF
#define	PIN_DEFAULT_P3OUT	0x00

// P4.4 BMA250 POWER (P4.7 is shorted to Vcc on my broken board)
#define	PIN_DEFAULT_P4DIR	0x7F
#define	PIN_DEFAULT_P4OUT	0x00

// P5.0, P5.1 == LF crystal
// P5.2	BMA250 CSB
// P5.3	BRLE PIO_6	(command mode toggle)
// P5.4 BRLE PIO_4	(multipurpose input)
// P5.5 BMA250 SCK
// P5.6 BMA250 SDI
// P5.7 BMA250 SDO
#define PIN_DEFAULT_P5SEL	0x03
#define	PIN_DEFAULT_P5DIR	0xFC
#define	PIN_DEFAULT_P5OUT	0x00

#define	BUTTON_LIST 	{ \
				BUTTON_DEF (1, 0x80, 0), \
			}

#define	BUTTON_M1	0

#define	BUTTON_PIN_P1_IRQ	0x80
#define	BUTTON_DEBOUNCE_DELAY	64
#define	BUTTON_PRESSED_LOW	0

// Current measurement on EM430F6137RF900 development board:
//
//	CPU Idle in PD mode (normal OS activity): 2.7uA at 3.3V
//	----------- PU -------------------------: 452uA
//	CPU spin                                : 2.8mA
//
// Added 100uF capacitor Vdd-GND, no noticeable increase in current

// ============================================================================

#include "board_rtc.h"

// #define	EXTRA_INITIALIZERS	CNOP

// P2 pins are analog-capable; BRLE pins made available for testing
#define	PIN_LIST	{	\
	PIN_DEF	(P2, 0),	\
	PIN_DEF	(P2, 1),	\
	PIN_DEF	(P2, 2),	\
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 4),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P2, 7),	\
	PIN_DEF	(P1, 3),	\
	PIN_DEF	(P1, 4),	\
	PIN_DEF	(P1, 7),	\
	PIN_DEF (P3, 1),	\
	PIN_DEF (P3, 2),	\
	PIN_DEF (P3, 3),	\
	PIN_DEF (P3, 4),	\
	PIN_DEF (P3, 5),	\
	PIN_DEF (P3, 7),	\
	PIN_DEF (P4, 0),	\
	PIN_DEF (P4, 1),	\
	PIN_DEF (P4, 2),	\
	PIN_DEF (P4, 3),	\
	PIN_DEF (P4, 5),	\
	PIN_DEF (P4, 6),	\
	PIN_DEF (P4, 7),	\
	PIN_DEF (P5, 3),	\
	PIN_DEF (P5, 4),	\
}

// BRLE pins: 
//	 2 - PIO_2 (status, read)
//	15 - PIO_3 (sleep mode toggle, write)
//	24 - PIO_6 (command mode toggle, write)
//	25 - PIO_4 (multipurpose, write)

#define	PIN_MAX		26
#define	PIN_MAX_ANALOG	7
#define	PIN_DAC_PINS	0x00

// ============================================================================

#include "bma250.h"

#define bma250_bring_down	do { \
					_BIC (P4OUT, 0x10); \
					_BIS (P5DIR, 0x80); \
					_BIS (P1DIR, 0x06); \
					_BIC (P5OUT, 0x24); \
				} while (0)

#define	bma250_bring_up		do { \
					_BIS (P5OUT, 0x24); \
					_BIC (P1DIR, 0x06); \
					_BIC (P5DIR, 0x80); \
					_BIS (P4OUT, 0x10); \
				} while (0)

#define	bma250_csel	_BIC (P5OUT, 0x04)
#define	bma250_cunsel	_BIS (P5OUT, 0x04)

#define	bma250_enable	_BIS (P1IE, 0x06)
#define	bma250_disable	_BIC (P1IE, 0x06)
#define	bma250_clear	_BIC (P1IFG, 0x06)
#define	bma250_int	(P1IFG & 0x06)

#define	bma250_clkl	_BIC (P5OUT, 0x20)
#define	bma250_clkh	_BIS (P5OUT, 0x20)

#define	bma250_outl	_BIC (P5OUT, 0x40)
#define	bma250_outh	_BIS (P5OUT, 0x40)

#define	bma250_data	(P5IN & 0x80)

// Note: this delay only applies when writing. 15us didn't work, 20us did,
// so 40 looks like a safe bet
#define	bma250_delay	udelay (40)

// ============================================================================

#include "analog_sensor.h"
#include "sensors.h"

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,		\
		INTERNAL_VOLTAGE_SENSOR,		\
		DIGITAL_SENSOR (0, NULL, bma250_read),	\
	}
#define	SENSOR_ANALOG
#define	N_HIDDEN_SENSORS	2

#define	SENSOR_DIGITAL
#define	SENSOR_EVENTS

// ============================================================================

#define	EXTRA_INITIALIZERS	__pi_rtc_init ()
