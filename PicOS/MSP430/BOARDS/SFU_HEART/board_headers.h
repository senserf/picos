#ifndef	__board_headers_h
#define	__board_headers_h

// Extra headers for the praxis and, possibly, board specific system inserts

#define	N_HEART_RATE_INTERVALS	8

extern word HeartRateIntervals [];
extern word HRINext, HRTimer;

void hrc_start (), hrc_stop ();
word hrc_get ();

#endif