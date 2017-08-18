/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"
#include "pins.h"

// ============================================================================
// ============================================================================

#ifdef	SENSOR_LIST
#if !defined(SENSOR_DIGITAL) && !defined(SENSOR_ANALOG)
#error "S: SENSOR_LIST defined, but neither SENSOR_DIGITAL, nor SENSOR_ANALOG!"
#endif

// ============================================================================

#ifndef	N_HIDDEN_SENSORS
#define	N_HIDDEN_SENSORS	0
#endif

static const i_sensdesc_t sensor_list [] = SENSOR_LIST;

#define	N_SENSORS 	(sizeof (sensor_list) / sizeof (d_sensdesc_t))

#define	sensors	((d_sensdesc_t*) sensor_list)

#if 0
void dump_sensor_list () {

	sint i, j;

	for (i = 0; i < N_SENSORS; i++) {
		diag ("Sensor %d", i);
		for (j = 0; j < sizeof (i_sensdesc_t); j++)
			diag ("  %x", *(((byte*)&(sensor_list [i])) + j));
	}
}

#endif

#ifdef	SENSOR_INITIALIZERS

// Some sensors have to be initialized

void __pi_init_sensors () {

	sint i;
	void (*f) (void);

	for (i = 0; i < N_SENSORS; i++)
		if ((sensors [i] . tp & 0x80) &&
		    (f = sensors [i] . fun_ini) != NULL)
			(*f) ();

}

#endif	/* SENSOR_INITIALIZERS */

#endif	/* SENSOR_LIST */

// ============================================================================

void read_sensor (word st, sint sn, address val) {

#ifdef	SENSOR_LIST

	const d_sensdesc_t *s;

	if ((word)(sn += N_HIDDEN_SENSORS) >= N_SENSORS) {
		// Commissioned by Wlodek
		(*val)++;
		return;
	}

	s = sensors + sn;

#ifndef	SENSOR_DIGITAL

	analog_sensor_read (st, ((const a_sensdesc_t*)s), val);

#else
#ifndef	SENSOR_ANALOG

	(*(s->fun_val)) (st, (const byte*)s, val);
#else
	if ((s->tp & 0x80))
		(*(s->fun_val)) (st, (const byte*)s, val);
	else
		analog_sensor_read (st, ((const a_sensdesc_t*)s), val);
#endif
#endif

#else
	syserror (EREQPAR, "read_sensor");
#endif
}

// ============================================================================

#ifdef	SENSOR_EVENTS

void wait_sensor (sint sn, word st) {
//
// Wait for a sensor event; only available for "digital" sensors
//
#if defined(SENSOR_LIST) && defined(SENSOR_DIGITAL)

	const d_sensdesc_t *s;

	if ((word)(sn += N_HIDDEN_SENSORS) < N_SENSORS &&
	   ((s = sensors + sn) -> tp & 0x80)) {
		// Otherwise illegal
		(*(s->fun_val)) (st, (const byte*)s, NULL);
	} else
#endif
	syserror (EREQPAR, "wait_sensor");
}
#endif
