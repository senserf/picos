#ifndef	__pg_switches_h
#define	__pg_switches_h

//+++ "switches.c"

#include "sysio.h"

typedef struct {
	byte	S0, S1, S2, S3;
} switches_t;

void read_switches (switches_t*);

#endif
