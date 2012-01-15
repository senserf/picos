#ifndef	__pg_raw_cc1101_h
#define	__pg_raw_cc1101_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "cc1101.h"

//+++ "raw_cc1101.c"

extern	byte	rrf_patable [8];
extern	byte	rrf_regs [];

void rff_set_reg (byte addr, byte val);
byte rff_get_reg (byte addr);
void rff_set_reg_burst (byte addr, byte *buffer, word count);
void rff_get_reg_burst (byte addr, byte *buffer, word count);
void rff_set_reg_group (const byte *grp);
byte rff_status ();
void rff_enter_idle ();
void rff_enter_rx ();
int rff_rx_status ();
void rff_rx_reset ();
void int rff_cts ();
void rff_pd ();
void rff_chip_reset ();
void rff_init ();

#define	rff_get_fifo_byte()	rff_get_reg (CCxxx0_RXFIFO)
#define	rff_get_fifo_bytes(buf,len) \
		rff_get_reg_burst (CCxxx0_RXFIFO, (byte*)(buf), (byte)(len))

#define	rff_set_fifo_byte(b)	rff_set_reg (CCxxx0_TXFIFO, (byte)(b))
#define	rff_set_fifo_bytes(buf,len) \
		rff_set_reg_burst (CCxxx0_TXFIFO, (byte*)(buf), (byte)(len))

#endif
