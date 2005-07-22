#ifndef __msg_tarp_h
#define __msg_tarp_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

//      Avoiding bytes and chars, even in the msg buffers

typedef struct headerStruct {
	word    msg_type;
	word    seq_no;
	lword   snd;
	lword   rcv;
	word    hoc;    // # of hops so far
	word    hco;    // last one from the destination to me
} headerType; // 16 bytes
#define in_header(buf, field)   (((headerType *)buf)->field)

#endif
