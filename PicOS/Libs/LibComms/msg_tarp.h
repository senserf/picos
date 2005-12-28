#ifndef __msg_tarp_h
#define __msg_tarp_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

typedef word nid_t;
typedef byte seq_t;
typedef byte msg_t;
typedef byte hop_t;

typedef struct headerStruct {
	msg_t   msg_type;
	seq_t   seq_no;
	nid_t   snd;
	nid_t   rcv;
	hop_t   hoc;    // # of hops so far
	hop_t   hco;    // last one from the destination to me
} headerType;
#define in_header(buf, field)   (((headerType *)buf)->field)

#endif
