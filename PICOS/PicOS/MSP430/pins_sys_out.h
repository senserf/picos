/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_pins_sys_out_h
#define	__pg_pins_sys_out_h

//+++ pins_sys_out.c

volatile byte *__port_out (word);

#define	__port_out_value(p,v)	do { \
		volatile byte *t = __port_out (p); \
		if ((v) ^ (p)->edge) \
			_BIS (*t, 1 << (p)->pnum); \
		else \
			_BIC (*t, 1 << (p)->pnum); \
	} while (0)

#endif
