//
// A NULL modem at 115200 bps
//
#include "sysio.h"

typedef struct {

	sint uafrom, uato;
	byte led;
	char buff;

} repeater_data_t;

repeater_data_t A = { UART_A, UART_B, 1, 0 };
repeater_data_t B = { UART_B, UART_A, 2, 0 };

// ============================================================================

fsm repeater (repeater_data_t *d) {

	state UREAD:

		io (UREAD, d->uafrom, READ, &(d->buff), 1);

	state UWRITE:

		io (UWRITE, d->uato, WRITE, &(d->buff), 1);
		leds_all (0);
		leds (d->led, 1);
		sameas UREAD;
}

// ============================================================================

fsm root {

	state INIT:

		leds_all (0);
		leds (0, 1);
		runfsm repeater (&A);
		runfsm repeater (&B);
		finish;
}
