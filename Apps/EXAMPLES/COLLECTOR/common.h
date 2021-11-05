#ifndef __pg_common_h
#define	__pg_common_h

// Blinking LED interface
#include "blink.h"

// The NULL plugin is needed by both node types
#include "plug_null.h"

// For now, it is just the three-vector of acceleration, each value takes
// one word
#define	SAMPLE_LENGTH	(3*2)

#endif
