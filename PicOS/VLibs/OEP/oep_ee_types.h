/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __oep_ee_types_h__
#define	__oep_ee_types_h__

typedef struct {
//
// This is the data structure that must be maintained while a transaction is
// in progress: it stores the origin and length of the EEPROM block, as well
// as the number of chunks (-1)
//
	lword FWA, LEN;
	word NC;

} ee_rxdata_t;

#endif
