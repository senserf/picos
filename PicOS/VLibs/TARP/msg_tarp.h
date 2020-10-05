/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_tarp_h
#define __msg_tarp_h
#include "sysio.h"

typedef word nid_t;
typedef byte seq_t;
typedef byte msg_t;

typedef struct headerStruct {
	msg_t   msg_type;
	seq_t   seq_no;
	nid_t   snd;
	nid_t   rcv;
	word	hoc   :7; // # of hops so far
	word    prox  :1; // proxy msg (no fwd)
	word	hco   :7; // range or perceived distance
	word	weak  :1; // weak signal (overshot?) happened on the way
} headerType;
#define in_header(buf, field)   (((headerType *)(buf))->field)

#endif
