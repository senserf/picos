#ifndef	__pg_butts_h
#define	__pg_butts_h

#include "sysio.h"

#define	DEBUGGING

#define	BUTTON_LT		0	// Left Top
#define	BUTTON_LB		1	// Left Bottom
#define	BUTTON_RT		2	// Right Top	aka >
#define	BUTTON_RB		3	// Right Bottom	aka <

#define	N_BUTTONS		4

#define	DEPRESS_CHECK_INTERVAL	16

#define	LONG_PRESS_TIME		((3 * 1024) / DEPRESS_CHECK_INTERVAL)

extern	byte TheButton, WatchBeingSet;
extern	sint RFC;

// ============================================================================
#ifdef DEBUGGING

void rms (word, word);

#else

#define	rms (a,b)	do { } while (0)

#endif
// ============================================================================

#endif
