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

// ============================================================================

#endif
