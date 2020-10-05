/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_ab_h__
#define	__pg_ab_h__

//+++ "ab.cc" "ab_common.cc"
#include "form.h"
#include "ab_modes.h"

char *ab_outf (word, const char*, ...);
char *ab_out (word, char*);
int ab_inf (word, const char*, ...);
char *ab_in (word);

#endif
