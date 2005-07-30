#include "kernel.h"
/*
 * This code handles all interrupts triggered by P1 pins
 */

#if		CHIPCON
#include	"chipcon.h"
#endif

#if		DM2100
#include	"dm2100.h"
#endif

interrupt (PORT1_VECTOR) p1_int () {

#if	CHIPCON
#include "irq_chipcon.h"
#endif

#if	DM2100
#include "irq_dm2100_pins.h"
#endif

// Here room for more functions

}
