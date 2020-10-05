/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_ds18b20_h
#define	__pg_ds18b20_h

#include "ds18b20_sys.h"
//+++ "ds18b20.c"

void ds18b20_read (word, word, address);
void ds18b20_init ();

#endif
