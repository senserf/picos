#include "emulator.h"

void rds (word st, address where) {
//
// Return three random numbers
//
	sint i;

	for (i = 0; i < 3; i++)
		where [i] = rnd ();
}
