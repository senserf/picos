/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "analog_actuator.h"
#include "pins.h"

//
// Driver for a generic DAC actuator
//

#ifndef	actuator_dac_prelude
#define	actuator_dac_prelude(p)		CNOP
#endif

#ifndef	actuator_dac_postlude
#define	actuator_dac_postlude(p)	CNOP
#endif

static byte dac_state;

void analog_actuator_write (word state, const a_actudesc_t *par, word val) {

	if (dac_state == 2 || val > 4095) {
		// We have been resumed to return (and clean up) after a delay
		dac_state = 0;
		actuator_dac_off (par);
		actuator_dac_postlude ((byte*)par);
		return;
	}

	// Starting up
	if (dac_state == 0) {
		// Prelude required
		actuator_dac_prelude ((byte*)par);
		actuator_dac_config (par);
		dac_state = 1;
	}

	actuator_dac_setvalue (par, val);

	if (par->interval) {
		dac_state = 2;
		delay (par->interval, state);
		release;
	}
}
