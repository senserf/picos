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

typedef struct headerStruct {
	msg_t   msg_type;
	seq_t   seq_no;
	nid_t   snd;
	nid_t   rcv;
	word	hoc   :4; // # of hops so far
	word	hco   :4; // range or perceived distance
	word	prox  :1; // proxy msg (no fwd)
	word	weak  :1; // weak signal (overshot?) happened on the way
	word	spare :6;
} headerType;
#define in_header(buf, field)   (((headerType *)(buf))->field)

#endif
