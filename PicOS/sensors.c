/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"
#include "pins.h"

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

void read_sensor (word st, word sn, address val) {

	sensdesc_t *s;

	if (sn >= N_SENSORS)
		syserror (EREQPAR, "read_sensor");

	(*(sensmap [sn] . fun_val)) (st, val);
}