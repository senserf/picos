/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"
#include "pins.h"

#ifdef	SENSOR_LIST

static const word sensor_list [] = SENSOR_LIST;

#define	N_SENSORS \
		(sizeof (sensor_list) / sizeof (d_sensdesc_t))

#define	sensors	((d_sensdesc_t*) sensor_list)

void zz_init_sensors () {

#ifndef	NO_DIGITAL_SENSORS

	int i;
	void (*f) (void);

	for (i = 0; i < N_SENSORS; i++)
		if ((sensors [i] . tp & 0x80) &&
		    (f = sensors [i] . fun_ini) != NULL)
			(*f) ();
#endif

}

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

#ifdef	NO_DIGITAL_SENSORS

	analog_sensor_read (st, ((const a_sensdesc_t*)s), val);

#else
#ifdef	NO_ANALOG_SENSORS

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
