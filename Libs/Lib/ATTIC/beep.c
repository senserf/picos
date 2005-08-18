/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

//+++ "__beeper.c"

int __beeper (word, address);

void beep (word del, word tone, word st) {

	if (del > 20)
		del = 20;

	if (tone > 6)
		tone = 6;
	else if (tone < 1)
		tone = 1;

	if (running (__beeper)) {
		if (st != NONE)
			proceed (st);
		return;
	}

	del = del << 8 | tone;

	if (st == NONE)
		fork (__beeper, (address) del);
	else
		call (__beeper, (address) del, st);
}
