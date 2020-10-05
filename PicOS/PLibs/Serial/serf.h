/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_serf_h
#define	__pg_serf_h

#include "ser_select.h"
#include "form.h"

//+++ "ser_outf.c" "ser_inf.c" "ser_select.c"

int ser_outf (word, const char*, ...);
int ser_inf (word, const char*, ...);

#endif
