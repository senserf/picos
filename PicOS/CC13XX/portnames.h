/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_portnames_h
#define	__pg_portnames_h		1


// Transformations of port operations, such that port pins can be
// referenced uniformly as a continuous range of numbers. This is
// trivial on CC13XXWARE, but extremely messy on MSP430.

#define	_PDS(p,v)	GPIO_setOutputEnableDio (p, v)
#define	_PFS(p,v)	CNOP
#define	_PVS(p,v)	do { if (v) GPIO_setDio (p) else GPIO_clearDio (p); } \
				while (0)
#define	_PHS(p,v)	CNOP
#define	_PPS(p,v)	CNOP

#define	_PH(p)		0
#define _PP(p)		0

#endif
