/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "actuators.h"
#include "pins.h"

#ifdef	ACTUATOR_LIST

#if !defined(ACTUATOR_DIGITAL) && !defined(ACTUATOR_ANALOG)
#error "S: ACTUATOR_LIST defined, but neither ACTUATOR_DIGITAL, nor ACTUATOR_ANALOG!"
#endif

#ifndef	N_HIDDEN_ACTUATORS
#define	N_HIDDEN_ACTUATORS	0
#endif

static const i_actudesc_t actuator_list [] = ACTUATOR_LIST;

#define	N_ACTUATORS	(sizeof (actuator_list) / sizeof (i_actudesc_t))

#define	actuators ((d_actudesc_t*) actuator_list)

#ifdef	ACTUATOR_INITIALIZERS

void __pi_init_actuators () {

	int i;
	void (*f) (void);

	for (i = 0; i < N_ACTUATORS; i++)
		if ((actuators [i] . tp & 0x80) &&
		    (f = actuators [i] . fun_ini) != NULL)
			(*f) ();
}

#endif	/* ACTUATOR_INITIALIZERS */

#endif	/* ACTUATOR_LIST */

// ============================================================================

void write_actuator (word st, sint sn, address val) {

#ifdef	ACTUATOR_LIST

	const d_actudesc_t *s;

	if ((word)(sn += N_HIDDEN_ACTUATORS) >= N_ACTUATORS)
		syserror (EREQPAR, "write_actuator");

	s = actuators + sn;

#ifndef	ACTUATOR_DIGITAL

	analog_actuator_write (st, (const a_actudesc_t*)s, *val);

#else
#ifndef	ACTUATOR_ANALOG

	(*(s->fun_val)) (st, (const byte*)s, val);
#else
	if ((s->tp & 0x80))
		(*(s->fun_val)) (st, (const byte*)s, val);
	else
		analog_actuator_write (st, (const a_actudesc_t*)s, *val);
#endif
#endif

#else
	syserror (EREQPAR, "write_actuator");
#endif
}
