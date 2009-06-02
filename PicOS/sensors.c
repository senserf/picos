/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"
#include "pins.h"

#ifdef	SENSOR_LIST
static sensdesc_t sensmap [] = SENSOR_LIST;

#define	N_SENSORS	(sizeof (sensmap) / sizeof (sensdesc_t))

void zz_init_sensors () {

	int i;
	void (*f) (void);

	for (i = 0; i < N_SENSORS; i++) {
		if ((f = sensmap [i] . fun_ini) != NULL)
			(*f) ();
	}
}

#endif	/* SENSOR_LIST */

word read_sensor (word st, word sn, address val) {

#ifdef	SENSOR_LIST

	sensdesc_t *s;

	if (sn >= N_SENSORS) {
		// Commissioned by Wlodek
		(*val)++;
		return ERROR;
	}

	s = sensmap + sn;

	(*(s->fun_val)) (st, s->param, val);
	return 0;
#else
	syserror (EREQPAR, "read_sensor");
#endif
}
