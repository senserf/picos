#ifndef	__pg_raw_cc1101_h
#define	__pg_raw_cc1101_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "cc1100.h"

//+++ "raw_cc1101.c"

extern	byte	rrf_patable [8];
extern	byte	rrf_regs [];

void rrf_set_reg (byte addr, byte val);
byte rrf_get_reg (byte addr);
void rrf_set_reg_burst (byte addr, byte *buffer, word count);
void rrf_get_reg_burst (byte addr, byte *buffer, word count);
void rrf_set_reg_group (const byte *grp);
byte rrf_status ();
void rrf_enter_idle ();
void rrf_enter_rx ();
int rrf_rx_status ();
void rrf_rx_reset ();
int rrf_cts ();
void rrf_pd ();
void rrf_chip_reset ();
void rrf_init ();

#define	rrf_get_fifo_byte()	rrf_get_reg (CCxxx0_RXFIFO)
#define	rrf_get_fifo_bytes(buf,len) \
		rrf_get_reg_burst (CCxxx0_RXFIFO, (byte*)(buf), (byte)(len))

#define	rrf_set_fifo_byte(b)	rrf_set_reg (CCxxx0_TXFIFO, (byte)(b))
#define	rrf_set_fifo_bytes(buf,len) \
		rrf_set_reg_burst (CCxxx0_TXFIFO, (byte*)(buf), (byte)(len))

#endif
