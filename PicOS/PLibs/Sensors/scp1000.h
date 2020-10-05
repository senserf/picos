/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_scp1000_h
#define	__pg_scp1000_h

#include "scp1000_sys.h"
//+++ "scp1000.c"

void scp1000_init ();
void scp1000_read (word, const byte*, address);

#endif
