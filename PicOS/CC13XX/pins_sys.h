#ifndef	__pg_pins_sys_h
#define	__pg_pins_sys_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "portmap.h"

#define	PIN_DEF(p)	(p)

#include "board_pins.h"

// Up to four LEDS

#ifdef	LED0_pin
#ifndef	LED0_polarity
#define	LED0_polarity	1
#endif
#if	LED0_polarity
#define	LED0_ON		GPIO_setDio (LED0_pin)
#define	LED0_OFF	GPIO_clearDio (LED0_pin)
#else
#define	LED0_ON		GPIO_clearDio (LED0_pin)
#define	LED0_OFF	GPIO_setDio (LED0_pin)
#endif
#endif

#ifdef	LED1_pin
#ifndef	LED1_polarity
#define	LED1_polarity	1
#endif
#if	LED1_polarity
#define	LED1_ON		GPIO_setDio (LED1_pin)
#define	LED1_OFF	GPIO_clearDio (LED1_pin)
#else
#define	LED1_ON		GPIO_clearDio (LED1_pin)
#define	LED1_OFF	GPIO_setDio (LED1_pin)
#endif
#endif

#ifdef	LED2_pin
#ifndef	LED2_polarity
#define	LED2_polarity	1
#endif
#if	LED2_polarity
#define	LED2_ON		GPIO_setDio (LED2_pin)
#define	LED2_OFF	GPIO_clearDio (LED2_pin)
#else
#define	LED2_ON		GPIO_clearDio (LED2_pin)
#define	LED2_OFF	GPIO_setDio (LED2_pin)
#endif
#endif

#ifdef	LED3_pin
#ifndef	LED3_polarity
#define	LED3_polarity	1
#endif
#if	LED3_polarity
#define	LED3_ON		GPIO_setDio (LED3_pin)
#define	LED3_OFF	GPIO_clearDio (LED3_pin)
#else
#define	LED3_ON		GPIO_clearDio (LED3_pin)
#define	LED3_OFF	GPIO_setDio (LED3_pin)
#endif
#endif

// ============================================================================

#ifndef	PIN_MAX_ANALOG
#define	PIN_MAX_ANALOG			0
#endif

#ifndef	PIN_DAC_PINS
#define	PIN_DAC_PINS			0
#endif

#ifdef PIN_LIST

//+++ pins_sys.c

/*
 * GP pin operations
 */
Boolean __pi_pin_available (word);
Boolean __pi_pin_adc_available (word);
word __pi_pin_ivalue (word);
word __pi_pin_ovalue (word);
Boolean __pi_pin_adc (word);
Boolean __pi_pin_output (word);
void __pi_pin_set (word);
void __pi_pin_clear (word);
void __pi_pin_set_input (word);
void __pi_pin_set_output (word);
void __pi_pin_set_adc (word);

#else	/* NO PIN_LIST */

#define __pi_pin_available(a)		0
#define __pi_pin_adc_available(a)	0
#define __pi_pin_ivalue(a)		0
#define __pi_pin_ovalue(a)		0
#define __pi_pin_adc(a)			0
#define __pi_pin_output(a)		0
#define __pi_pin_set(a)			CNOP
#define __pi_pin_clear(a)		CNOP
#define __pi_pin_set_input(a)		CNOP
#define __pi_pin_set_output(a)		CNOP
#define __pi_pin_set_adc(a)		CNOP

#endif	/* PIN_LIST or NO PIN_LIST */

#ifndef	PIN_DAC_PINS
#define	PIN_DAC_PINS			0
#endif

#if PIN_DAC_PINS

Boolean __pi_pin_dac_available (word);
Boolean __pi_pin_dac (word);
void __pi_clear_dac (word);
void __pi_set_dac (word);
void __pi_write_dac (word, word, word);

#else

#define	__pi_pin_dac_available(a)	NO
#define	__pi_pin_dac(a)			NO
#define	__pi_clear_dac(a)		CNOP
#define	__pi_set_dac(a)			CNOP
#define	__pi_write_dac(a,b,c)		CNOP

#endif	/* PIN_DAC_PINS */

// ============================================================================
// This is for the sensor/actuator-type pin interface =========================
// ============================================================================

#define	INPUT_PIN(p,e)		{ p, e }
#define	OUTPUT_PIN(p,e)		{ p, e }

#if defined(INPUT_PIN_LIST) || defined(OUTPUT_PIN_LIST)

typedef struct {

	byte	pnum:7,		// Pin number
		edge:1;		// Polarity 0: on==1, 1: on==0

} piniod_t;

#endif

#if defined(INPUT_PIN_LIST)

extern const piniod_t __input_pins [];
void __pinlist_setirq (int);

// No need for initialization, edge set globally
#define	__pinsen_setedge_irq		CNOP
#define	INPUT_PINLIST_GPIO(p)		((p).pnum)
#define	__pinsen_clear_and_enable	__pinlist_setirq (1)
#define	__pinsen_disable_and_clear	__pinlist_setirq (0)

#define	pin_sensor_int		(HWREG (GPIO_BASE + GPIO_O_EVFLAGS31_0) & \
					INPUT_PINLIST_GPIOS)	
#endif

// ============================================================================
// To switch the I2C bus, the arguments are two pin numbers
// ============================================================================

#if I2C_INTERFACE > 1
// A pair of pins (sda << 8) | scl
void __select_i2c_bus (word);
#endif

#endif
