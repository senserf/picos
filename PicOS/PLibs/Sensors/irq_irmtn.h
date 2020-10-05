/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_irq_irmtn_h
#define __pg_irq_irmtn_h

if (irmtn_int) {
	irmtn_clear;
	if (irmtn_icounter != WNONE)
		irmtn_icounter++;
}

#endif
