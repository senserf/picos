/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "actuators.h"
#include "pins.h"

#ifdef	ACTUATOR_LIST
static actudesc_t actumap [] = ACTUATOR_LIST;

#define	N_ACTUATORS	(sizeof (actumap) / sizeof (actudesc_t))

void __pi_init_actuators () {

	int i;
	void (*f) (void);

	for (i = 0; i < N_ACTUATORS; i++) {
		if ((f = actumap [i] . fun_ini) != NULL)
			(*f) ();
	}
}

#endif	/* ACTUATOR_LIST */

void write_actuator (word st, word sn, address val) {

#ifdef	ACTUATOR_LIST

	actudesc_t *s;

	if (sn >= N_ACTUATORS)
		syserror (EREQPAR, "write_actuator");

	(*(actumap [sn] . fun_val)) (st, val);
#else
	syserror (EREQPAR, "write_actuator");
#endif

}
