#ifndef __msg_tags_h
#define __msg_tags_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_tagStructs.h"

typedef enum {
	msg_null,

#include "tags_msgSet.inc"

} msgType;

#define APP_TAGS	0x0100
#define MSG_ENABLED	0x0100 // TAGS only
#define is_msg_enabled(type)	((type) & MSG_ENABLED)
#define is_app_valid(type)	((type) & APP_TAGS)

#endif
