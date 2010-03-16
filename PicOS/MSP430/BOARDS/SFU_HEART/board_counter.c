#include "kernel.h"
#include "pins.h"


word	HeartRateIntervals [N_HEART_RATE_INTERVALS];
word	HRINext;
word	HRTimer;

void hrc_start () {

	for (HRINext = 0; HRINext < N_HEART_RATE_INTERVALS; HRINext++)
		HeartRateIntervals [HRINext] = ~1024;

	HRINext = 0;
	enable_heart_rate_counter;
	utimer (&HRTimer, YES);
	utimer_set (HRTimer, 0xffff);
}

void hrc_stop () {

	utimer (&HRTimer, NO);
	disable_heart_rate_counter;
}

word hrc_get () {

	word w, i;

	for (w = 0, i = 0; i < N_HEART_RATE_INTERVALS; i++)
		w += ~HeartRateIntervals [i];

	return (w == 0) ? 0 :
		(word)(((lword)60*(1024*N_HEART_RATE_INTERVALS)) / w);
}
