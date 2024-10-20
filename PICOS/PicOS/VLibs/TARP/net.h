/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __net_h
#define __net_h

#include "netparams.h"

//+++ "net.cc"

int net_opt (int opt, address arg);
int net_qera (int d);
int net_qsize (int d);
int net_init (word phys, word plug);
int net_rx (word state, char ** buf_ptr, address rssi_ptr, byte encr);
int net_tx (word state, char * buf, int len, byte encr);
int net_close (word state);

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
// N=0 XP=7 CAV=0
#define DEF_NET_PXOPTS 0x7000
extern  word            net_pxopts;
#endif

#endif
