#ifndef __msg_pegs_h
#define __msg_pegs_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_tagStructs.h"
#include "msg_pegStructs.h"

typedef enum {
	msg_null,

#include "tags_msgSet.inc"

	, msg_new, msg_newAck, msg_master, msg_report, msg_reportAck,
	msg_fwd, msg_alrm, msg_findTag
} msgType;

#define APP_TAGS	0x0200
#define MSG_ENABLED	0x0300 // PEGS and TAGS
#define is_msg_enabled(type)	((type) & MSG_ENABLED)
#define is_app_valid(type)	((type) & APP_TAGS & APP_PEGS)

#endif
