#ifndef __net_h
#define __net_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007.			*/
/* All rights reserved.							*/
/* ==================================================================== */

//+++ "net.c"

#ifndef __SMURPH__

// This file appears to be needed for historical reasons only. Its role has
// been taken over by net_node_data.h. We retain it as a way to request the
// compilation of net.c.

// We need method headers in here

int net_opt (int opt, address arg);
int net_qera (int d);
int net_qsize (int d);
int net_init (word phys, word plug);
int net_rx (word state, char ** buf_ptr, address rssi_ptr, byte encr);
int net_tx (word state, char * buf, int len, byte encr);
int net_close (word state);

// This one to be provided by the praxis
Boolean msg_isClear (byte);

#endif

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
// N=0 XP=7 CAV=0
#define DEF_NET_PXOPTS 0x7000
extern  word            net_pxopts;
#endif

#endif
