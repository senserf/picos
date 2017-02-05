#include "sysio.h"
#include "kernel.h"
#include "pin_sensor.h"

#ifdef INPUT_PIN_LIST

#include "pins_sys_in.h"

// ============================================================================
// Pin sensor =================================================================
// ============================================================================

static const piniod_t input_pins [] = INPUT_PIN_LIST;

void pin_sensor_init () { __pinsen_setedge_irq; }

void pin_sensor_read (word st, const byte *junk, address val) {

	const piniod_t *p;
	word i;
	byte v;

	if (val == NULL) {
		// Called to issue a wait request
		if (st == WNONE)
			// Make sure this is not WNONE
			return;
		cli;
		__pinsen_clear_irq;
		__pinsen_enable_irq;
		when (&input_pins, st);
		sti;
		release;
	}

	*val = 0;
	for (i = 0, p = input_pins;
	    		  i < sizeof (input_pins) / sizeof (piniod_t); i++, p++)
		*val |= __port_in_value (p) << i;
}

void pin_sensor_interrupt () {

	i_trigger ((word)(&input_pins));

	__pinsen_disable_irq;
	__pinsen_clear_irq;

	RISE_N_SHINE;
}

#endif /* INPUT_PIN_LIST */

#ifdef OUTPUT_PIN_LIST

#include "pins_sys_out.h"

// ============================================================================
// Pin actuator ===============================================================
// ============================================================================

static const piniod_t output_pins [] = OUTPUT_PIN_LIST;

void pin_actuator_write (word st, const byte *junk, address val) {

	const piniod_t *p;
	word i;

	for (i = 0, p = output_pins;
		      i < sizeof (output_pins) / sizeof (piniod_t); i++, p++)
		__port_out_value (p, (*val >> i) & 1);
}

#endif /* OUTPUT_PIN_LIST */
