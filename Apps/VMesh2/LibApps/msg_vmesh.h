#ifndef __msg_vmesh_h
#define __msg_vmesh_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "msg_vmeshStructs.h"
typedef enum {
	msg_null, msg_master, msg_trace, msg_traceAck,
      	msg_cmd, msg_new, msg_bindReq, msg_bind, msg_br,
	msg_alrm, msg_st, msg_stAck, msg_stNack, msg_io, msg_ioAck
} msgType;

#endif