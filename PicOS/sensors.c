/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"
#include "pins.h"

// ============================================================================

#ifdef	SENSOR_LIST

#ifndef	SENSOR_DIGITAL
#ifndef	SENSOR_ANALOG
#error "S: SENSOR_LIST defined, but neither SENSOR_DIGITAL, nor SENSOR_ANALOG!"
#endif
#endif

static const i_sensdesc_t sensor_list [] = SENSOR_LIST;

#define	N_SENSORS 	(sizeof (sensor_list) / sizeof (d_sensdesc_t))

#define	sensors	((d_sensdesc_t*) sensor_list)

#ifdef	SENSOR_INITIALIZERS

// Some sensors have to be initialized

void zz_init_sensors () {

	int i;
	void (*f) (void);

	for (i = 0; i < N_SENSORS; i++)
		if ((sensors [i] . tp & 0x80) &&
		    (f = sensors [i] . fun_ini) != NULL)
			// Note: the arg is either nothing (no controllers) or
			// WNONE (selecting the initializer function)
			(*f) ();

}

#endif	/* SENSOR_INITIALIZERS */

#endif	/* SENSOR_LIST */

// ============================================================================

word read_sensor (word st, word sn, address val) {

#ifdef	SENSOR_LIST

	const d_sensdesc_t *s;

	if (sn >= N_SENSORS) {
		// Commissioned by Wlodek
		(*val)++;
		return ERROR;
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

	return 0;
#else
	syserror (EREQPAR, "read_sensor");
#endif
}
