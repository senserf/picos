#include "sysio.h"
#include "kernel.h"
#include "pin_sensor.h"

#ifdef	PIN_SENSOR_P1_IRQ
#define	clear_p1_irq	_BIC (P1IFG, PIN_SENSOR_P1_IRQ)
#define	enable_p1_irq	_BIS (P1IE, PIN_SENSOR_P1_IRQ)
#define	disable_p1_irq	_BIC (P1IE, PIN_SENSOR_P1_IRQ)
#else
#define	clear_p1_irq	CNOP
#define	enable_p1_irq	CNOP
#define	disable_p1_irq	CNOP
#endif

#ifdef	PIN_SENSOR_P2_IRQ
#define	clear_p2_irq	_BIC (P2IFG, PIN_SENSOR_P2_IRQ)
#define	enable_p2_irq	_BIS (P2IE, PIN_SENSOR_P2_IRQ)
#define	disable_p2_irq	_BIC (P2IE, PIN_SENSOR_P2_IRQ)
#else
#define	clear_p2_irq	CNOP
#define	enable_p2_irq	CNOP
#define	disable_p2_irq	CNOP
#endif

#define	clear_irq	do { clear_p1_irq; clear_p2_irq; } while (0)
#define	enable_irq	do { enable_p1_irq; enable_p2_irq; } while (0)
#define	disable_irq	do { disable_p1_irq; disable_p2_irq; } while (0)

// ============================================================================

void pin_sensor_init () {

#ifdef	PIN_SENSOR_P1_EDGES
	P1IES = (P1IES & ~PIN_SENSOR_P1_BITS) | PIN_SENSOR_P1_EDGES;
#endif

#ifdef	PIN_SENSOR_P2_EDGES
	P2IES = (P2IES & ~PIN_SENSOR_P2_BITS) | PIN_SENSOR_P2_EDGES;
#endif

}

void pin_sensor_read (word st, const byte *junk, address val) {

	if (val == NULL) {
		// Called to issue a wait request
		if (st == WNONE)
			// Make sure this is not WNONE
			return;
		cli;
		clear_irq;
		enable_irq;
		when (&pin_sensor_read, st);
		sti;
		release;
	}
	((byte*)val) [0] =
#ifdef	PIN_SENSOR_P1_BITS
				(P1IN & PIN_SENSOR_P1_BITS)
#ifdef	PIN_SENSOR_P1_EDGES
					^ PIN_SENSOR_P1_EDGES
#endif

#else
				0
#endif
	;

	((byte*)val) [1] =
#ifdef	PIN_SENSOR_P2_BITS
				(P2IN & PIN_SENSOR_P2_BITS)
#ifdef	PIN_SENSOR_P2_EDGES
					^ PIN_SENSOR_P2_EDGES
#endif

#else
				0
#endif
	;
}

void pin_sensor_interrupt () {

	i_trigger ((word)(&pin_sensor_read));

	disable_irq;
	clear_irq;

	RISE_N_SHINE;
}
