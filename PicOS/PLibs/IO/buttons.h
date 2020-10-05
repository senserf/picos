/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_buttons_h
#define	__pg_buttons_h

#include "buttons_sys.h"

//+++ "buttons.c"

void	buttons_action (void (*action)(word));

#define	button_down(a)	button_still_pressed (__button_list [a])
#define	button_up(a)	(!button_down (a))

#endif
