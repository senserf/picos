#include "sysio.h"

word irmtn_counter;

void irmtn_count (word st, const byte *p, address v) {

	cli;
	*v = irmtn_counter;
	irmtn_counter = 0;
	sti;
}
