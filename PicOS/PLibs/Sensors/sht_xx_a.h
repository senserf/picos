/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_sht_xx_a_h
#define	__pg_sht_xx_a_h

// SHT sensor, array version

#include "sht_xx_sys.h"
//+++ "sht_xx_a.c"

#include "sht_xx_cmd.h"

void shtxx_a_temp (word, const byte*, address);
void shtxx_a_humid (word, const byte*, address);
void shtxx_a_init (void);

#endif
