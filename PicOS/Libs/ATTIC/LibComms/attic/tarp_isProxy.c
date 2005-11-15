/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "msg_tags.h"
#include "sysio.h"

bool tarp_isProxy (word msg) {

	if (msg == msg_pong) // APPS_... should be embedded in msgType(?)
		return YES;
	return NO;
}
