/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_sht_xx_h
#define	__pg_sht_xx_h

#include "sht_xx_sys.h"
//+++ "sht_xx.c"

#include "sht_xx_cmd.h"

void shtxx_temp (word, const byte*, address);
void shtxx_humid (word, const byte*, address);
void shtxx_init (void);

#endif
