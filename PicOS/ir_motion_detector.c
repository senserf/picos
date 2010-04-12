#include "sysio.h"
#include "pins.h"

word irmtn_icounter;

void irmtn_count (word st, const byte *p, address v) {

	cli;

	// Event count
	if ((*v = irmtn_icounter) == 0 && irmtn_active)
		*v = 1;
	irmtn_icounter = 0;

	sti;
}
