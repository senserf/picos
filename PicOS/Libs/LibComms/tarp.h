#ifndef __tarp_h
#define __tarp_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007.			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "sysio.h"
#include "tcvplug.h"
#include "msg_tarp.h"

#define TARP_CACHES_MALLOCED	0
#define	TARP_CACHES_TEST	0

//+++ "tarp.c"

// I'm missing them...
#ifndef true
#define true YES
#endif

#ifndef false
#define false NO
#endif

#define ddCacheSize		10
#define spdCacheSize		20
#define rtrCacheSize		10
#define tarp_maxHops		10

#ifndef TARP_RTR
#define TARP_RTR	0
#endif

#ifndef DEFAULT_RSSI_THOLD
#define DEFAULT_RSSI_THOLD	80
#endif

/*
 C compilter on eCog produces wrong code if chars are mixed with words
 in arrays. In general, we should stay away from chars there. Hopefully,
 other compilers behave, so we could use chars is a more reliable fashion.
 That's why we have seq_t, for example.

 This version (with chars) may blow up eCog's compiler...
 */

// DD cache struct
typedef struct ddc {
	// Note: int changed to sint. The idea is to use this type consistently
	// in all structures (like packet headers) to distinguish it from int
	// in the simulator. We want to preserve in the simulator the original
	// size of all items that contribute to packets. Notably, we cannot do
	// this with pointers (as in TCV buffers), and then we play tricks.
	sint	head;
	seq_t	m_seq;
	byte	spare;
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
	sint	head;
	word	m_hop; // check if it's really better with int instead word
	struct	spde en[spdCacheSize];
} spdcType;

typedef struct rtrc {
	byte	head;
	byte	fecnt;
	address pkt [rtrCacheSize];
	nid_t	sndr [rtrCacheSize];
	seq_t	seqn [rtrCacheSize];
	byte	rcnt [rtrCacheSize];
} rtrcType; // 1 + 1 + 10* (2+2+1+1) bytes

typedef struct tarpCtrlStruct {
	word	rcv;
	word	snd;
	word	fwd;
	word	param :8;
	word	flags :8;
	word	rssi_th :8;  // rssi threshold
	word	ssignal :8; // spare: a bool that can be local in tarp_rx
} tarpCtrlType;

// param
#define tarp_fwd_on	(tarp_ctrl.param & 1)
#define tarp_slack	((tarp_ctrl.param >> 1) & 3)
#define tarp_drop_weak	((tarp_ctrl.param >> 3) & 1)
#define tarp_rte_rec	((tarp_ctrl.param >> 4) & 3)
#define tarp_level	((tarp_ctrl.param >> 6) & 3)

// flags
#define tarp_setretry(r)   (tarp_ctrl.flags |= (r & 0x0F))
#define TARP_URGENT	0x10

extern  tarpCtrlType	tarp_ctrl;
extern  nid_t		net_id;
extern  nid_t		local_host;
extern  nid_t		master_host;

#ifndef	__SMURPH__
#include "tarp_hooks.h"
#endif

#endif
