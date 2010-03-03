#include "sysio.h"
#include "pins.h"

word irmtn_acounter, irmtn_icounter;

void irmtn_count (word st, const byte *p, address v) {

	cli;

	if (p [1]) {
		// Interrupt count
		*v = irmtn_icounter;
		irmtn_icounter = 0;
	} else {
		// Event count
		if ((*v = irmtn_acounter) == 0 && irmtn_active)
			*v = 1;
		irmtn_acounter = 0;
	}

	sti;
}
