#ifndef __tarp_h
#define __tarp_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "sysio.h"
#include "tcvplug.h"
#include "msg_tarp.h"

//+++ "tarp.c"

// I'm missing them...
#ifndef true
#define true YES
#endif

#ifndef false
#define false NO
#endif

#define ddCacheSize		5
#define spdCacheSize		10
#define tarp_maxHops		10

/*
 C compilter on eCog produces wrong code if chars are mixed with words
 in arrays. In general, we should stay away from chars there. Hopefully,
 other compilers behave, so we could use chars is a more reliable fashion.
 That's why we have seq_t, for example.

 This version (with chars) may blow up eCog's compiler...
 */

// DD cache struct
typedef struct ddc {
	int	head;
	seq_t	m_seq;
	nid_t	node[ddCacheSize];
	seq_t	seq[ddCacheSize];
} ddcType;

// SPD entry
struct spde {
	nid_t	host;
	word	hop; // MSB - hoc, LSB - attempt count
};

// SPD cache struct
typedef struct spdc {
	int	head;
	word	m_hop; // check if it's really better with int instead word
	struct	spde en[spdCacheSize];
} spdcType;

typedef struct tarpCtrlStruct {
	word	rcv;
	word	snd;
	word	fwd;
	byte	param;
	byte	flags;
} tarpCtrlType;

// param
#define tarp_fwd_on	(tarp_ctrl.param & 1)
#define tarp_slack	((tarp_ctrl.param >> 1) & 7)
#define tarp_rte_rec	((tarp_ctrl.param >> 4) & 3)
#define tarp_level	((tarp_ctrl.param >> 6) & 3)

// flags
#define tarp_setretry(r)   (tarp_ctrl.flags |= (r & 0x0F))
#define TARP_URGENT	0x10
#endif
