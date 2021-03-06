/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_ab_modes_h__
#define	__pg_ab_modes_h__

#define	AB_MODE_HOLD	0
#define	AB_MODE_PASSIVE	1
#define	AB_MODE_ACTIVE	2

// These two funtions are shared by the serial and binary variants
void ab_init (int);
void ab_mode (byte);

#endif
