/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_ser_h
#define	__pg_ser_h

#include "ser_select.h"

//+++ "ser_out.c" "ser_outb.c" "ser_in.c" "ser_select.c"

int ser_out (word, const char*);
int ser_in (word, char*, int);
int ser_outb (word, const char*);

#endif
