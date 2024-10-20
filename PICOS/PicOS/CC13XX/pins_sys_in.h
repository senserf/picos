/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_pins_sys_in_h
#define	__pg_pins_sys_in_h

#define	__port_in_value(p) (GPIO_readDio ((p)->pnum) ^ (p)->edge)

#endif
