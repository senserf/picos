#ifndef	__pg_emi_h
#define	__pg_emi_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// EMI definitions depending on the memory chip configured

#if	SDRAM_PRESENT

#if	TARGET_BOARD == BOARD_CYAN

#define	SDRAM_END		0x800000L	/* External memory size */

// Chip select for SDRAM

#define	sdram_init	do { \
 \
	rg.emi.sdram_cfg = 0x940d; \
	rg.emi.sdram_refr_per = 0x330c; \
	rg.emi.sdram_refr_cnt = 0x0010; \
	rg.emi.ctrl_sts = 0x0804; \
	fd.emi.sdram_cust_adr.adr = 0x400; \
	rg.emi.sdram_cust_cmd = 0x7c85; \
	fd.emi.sdram_cust_adr.adr = 0x030; \
	rg.emi.sdram_cust_cmd = 0x7c01; \
	rg.emi.sdram_cust_cmd = 0x0c63; \
 \
			} while (0)

#endif	/* BOARD_CYAN */

#if	TARGET_BOARD == BOARD_GEORGE

#if 0
// SRAM: BS62LV4000 (512 KB = 256 KW)

#define	SDRAM_END		0x40000L	/* External memory size */
#endif

#define	SDRAM_END		0x10000L	/* 64 K words only */

// 0x0A88
// 0x3AAA
#define	sdram_init	do { \
 \
	rg.emi.bus_cfg1 = 0x0008; \
	rg.emi.bus_cfg2 = 0x3AAA; \
	rg.emi.ctrl_sts = 0x0020; \
 \
			} while (0)


#endif	/* BOARD_GEORGE */

#endif	/* SDRAM_PRESENT */

#endif
