#include <ecog.h>
#include <ecog1.h>
#include "kernel.h"

#if	CHIPCON
#include "chipcon.h"
#endif

#if	ETHERNET_DRIVER
#include "ethernet.h"
#endif

#if	RADIO_TYPE == RADIO_XEMICS
#include "radio.h"
#endif

void __irq_entry gpio_int (void) {

/*
 * There is no way to do it right, e.g., to use a jump table filled by
 * registering driver-specific GPIO interrupt routines, because the C
 * compiler won't let you handle a pointer to an __irq_entry function.
 * At least I've tried.
 */

#if	CHIPCON
#include "irq_chipcon.h"
#endif

#if	ETHERNET_DRIVER
#include "irq_ethernet.h"
#endif

#if	RADIO_TYPE == RADIO_XEMICS
#include "irq_xemics.h"
#endif

}
