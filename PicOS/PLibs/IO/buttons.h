#ifndef	__pg_buttons_h
#define	__pg_buttons_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2018                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "buttons_sys.h"

//+++ "buttons.c"

void	buttons_action (void (*action)(word));

#define	button_down(a)	button_still_pressed (__button_list [a])
#define	button_up(a)	(!button_down (a))

#endif
