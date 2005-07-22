#ifndef __msg_tarp_h
#define __msg_tarp_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

//      Avoiding bytes and chars, even in the msg buffers
#define id_t  lword
#define seq_t word
#define msg_t word
#define hop_t word

typedef struct headerStruct {
	msg_t   msg_type;
	seq_t   seq_no;
	id_t    snd;
	id_t    rcv;
	hop_t   hoc;    // # of hops so far
	hop_t   hco;    // last one from the destination to me
} headerType; // 16 bytes
#define in_header(buf, field)   (((headerType *)buf)->field)

#endif
