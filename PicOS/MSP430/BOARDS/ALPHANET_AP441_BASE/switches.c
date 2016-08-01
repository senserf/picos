#include "switches.h"

void read_switches (switches_t *s) {
//
//
	byte a;

	_BIC (P5DIR, 0xF0);
	_BIS (P5OUT, 0xF0);
	_BIS (P5REN, 0xF0);
	udelay (100);
	a = ~P5IN;
	_BIC (P5REN, 0xF0);
	_BIC (P5OUT, 0xF0);
	_BIS (P5DIR, 0xF0);

	s->S0 = a >> 4;

	// ========================

	_BIC (P3DIR, 0xFF);
	_BIS (P3OUT, 0xFF);
	_BIS (P3REN, 0xFF);
	udelay (100);
	a = ~P3IN;
	_BIC (P5REN, 0xFF);
	_BIC (P3OUT, 0xFF);
	_BIS (P3DIR, 0xFF);

	s->S1 = a & 0x0F;
	s->S2 = (a >> 4) & 0x0F;
}
