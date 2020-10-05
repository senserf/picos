/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_switches_h
#define	__pg_switches_h

//+++ "switches.c"

#include "sysio.h"

typedef struct {
	byte	S0, S1, S2;
} switches_t;

void read_switches (switches_t*);

#endif
