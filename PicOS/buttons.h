#ifndef	__pg_buttons_h
#define	__pg_buttons_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "buttons_sys.h"

#define	BUTTON_PORT(b)			(((b) >> 12) & 0xf)
#define	BUTTON_REPEAT(b)		(((b) >> 8 ) & 0xf)
#define	BUTTON_PIN(b)			((byte) (b))

#define	BUTTON_DEF(port,pin,repeat)	( ((port) << 12) | \
					  ((repeat) << 8) | \
						(pin))

#define	BUTTON_PRESSED_EVENT ((word)&button_list)

extern const word button_list [];

#endif
