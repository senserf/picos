#ifndef	__pg_sensors_h
#define	__pg_sensors_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
//+++ "sensors.c"

#if LITTLE_ENDIAN
#define	_bb2w(a,b)	((word)(a) | ((word)(b) << 8))
#else
#define	_bb2w(a,b)	((word)(b) | ((word)(a) << 8))
#endif

// Six bytes per sensor
#define	__ANALOG_SENSOR(isi,ns,a,b,c) \
	_bb2w (isi, a), _bb2w (b, c), ns

#define	__DIGITAL_SENSOR(par,ini,pro) \
	_bb2w (0x80, par), (word)(pro), (word)(ini)

// Note: ANALOG_SENSOR is arch-dependent, as the interpretation of ADC
// parameters is idiosyncratic - see analog_sensor_sys.h
#define	DIGITAL_SENSOR(par,ini,pro) __DIGITAL_SENSOR (par, ini, pro)

typedef	struct {

	byte	tp, adcpars [3];
	word	nsamples;

} a_sensdesc_t;

typedef struct {

	byte tp, num;
								// Processor
	void (*fun_val) (word, const byte*, address);
	void (*fun_ini) ();

} d_sensdesc_t;

word read_sensor (word, word, address);

#endif
