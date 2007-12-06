#ifndef __net_h
#define __net_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007.			*/
/* All rights reserved.							*/
/* ==================================================================== */

//+++ "net.c"

// This file appears to be needed for historical reasons only. Its role has
// been taken over by net_node_data.h. We retain it as a way to request the
// compilation of net.c.

// We need method headers in here

int _da (net_opt)   (int opt, address arg);
int _da (net_qera)  (int d);
int _da (net_init)  (word phys, word plug);
int _da (net_rx)    (word state, char ** buf_ptr, address rssi_ptr, byte encr);
int _da (net_tx)    (word state, char * buf, int len, byte encr);
int _da (net_close) (word state);

// This one to be provided by the praxis
__VIRTUAL Boolean _da (msg_isClear) (byte) __ABSTRACT;

#endif
