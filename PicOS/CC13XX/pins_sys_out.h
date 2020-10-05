/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_pins_sys_out_h
#define	__pg_pins_sys_out_h

#define	__port_out_value(p,v)	do { \
		if ((v) ^ (p)->edge) \
			GPIO_setDio ((p)->pnum); \
		else \
			GPIO_clearDio ((p)->pnum); \
	} while (0)
#endif
