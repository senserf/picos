#include "kernel.h"
/*
 * This code handles all interrupts triggered by P2 pins
 */

#if		DM2100
#include	"dm2100.h"
#endif

interrupt (PORT2_VECTOR) p2_int () {

#if	DM2100
#include "irq_dm2100_pins.h"
#endif

// Here room for more functions

}
