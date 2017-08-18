#ifndef	__plug_boss_h_
#define	__plug_boss_h_
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

//+++ "plug_boss.cc"

extern trueconst tcvplug_t plug_boss;

#define	BOSS_PO_PRO	0	// Packet offset: protocol
#define	BOSS_PO_FLG	1	// Packet offset: flag
#define	BOSS_BI_CUR	1	// Bit 0
#define BOSS_BI_EXP	2	// Bit 1
#define	BOSS_BI_ACK	4	// Bit 2 == ACK only

#define	BOSS_PR_DIR	0xAC	// Protocol == direct
#define	BOSS_PR_ABP	0xAD	// Protocol == AB

#endif
