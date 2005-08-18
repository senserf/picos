#ifndef __msg_tarp_h
#define __msg_tarp_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

typedef word id_t;
typedef unsigned char seq_t;
typedef unsigned char msg_t;
typedef unsigned char hop_t;

typedef struct headerStruct {
	msg_t   msg_type;
	seq_t   seq_no;
	id_t    snd;
	id_t    rcv;
	hop_t   hoc;    // # of hops so far
	hop_t   hco;    // last one from the destination to me
	word	h_flags; // nhood, other tricks?
	//word	make_it_12B;
} headerType;
#define in_header(buf, field)   (((headerType *)buf)->field)

#endif
