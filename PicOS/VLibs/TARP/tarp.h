#ifndef __tarp_h_
#define __tarp_h_
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2016.			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "sysio.h"
#include "tcvplug.h"

#include "msg_tarp.h"

//+++ "tarp.cc"

// I'm missing them...
#ifndef true
#define true YES
#endif

#ifndef false
#define false NO
#endif

// See modsyms.h (PG)
#define ddCacheSize		TARP_DDCACHESIZE
#define spdCacheSize		TARP_SPDCACHESIZE
#define rtrCacheSize		TARP_RTRCACHESIZE
//#define tarp_maxHops		125 check DD... seriously
//#define tarp_maxHops		5
#define tarp_maxHops		TARP_MAXHOPS

#if TARP_RTR
#if TCV_TIMERS == 0 || TCV_HOOKS == 0
#error missing TCV_TIMERS or TCV_HOOKS
#endif
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
	byte	rcnt [rtrCacheSize];
	byte	hoc [rtrCacheSize];
} rtrcType; // 1 + 1 + 10 * (2+1+1) bytes

// In March 2015 flags :8 were replaced with pp_urg, pp_widen, spare fields
// Apparently. flags were not used, but the constructs were preserved in
// pp_urg and pp_widen fields. Now, we have 4 spare bits for extravagance.
typedef struct tarpCtrlStruct {
	word	rcv;
	word	snd;
	word	fwd;
	word	param :8;
	word	pp_urg 	 :1; // pp: per packet (tx clears these)
	word	pp_widen :3;
	word	mchg_msg :1;
	word	mchg_set :1;
	word	spare 	:2;
	word	rssi_th :8;  // rssi threshold
	word	ssignal :8; // spare: a bool that can be local in tarp_rx
} tarpCtrlType;

// param
#define tarp_fwd_on		(tarp_ctrl.param & 1)
#define tarp_slack		((tarp_ctrl.param >> 1) & 3)
#define tarp_drop_weak	((tarp_ctrl.param >> 3) & 1)
#define tarp_rte_rec	((tarp_ctrl.param >> 4) & 3)
#define tarp_level		((tarp_ctrl.param >> 6) & 3)
#define set_tarp_fwd(p)			do { tarp_ctrl.param =((tarp_ctrl.param & 0xFE) | ((p) & 1)); } while (0)
#define set_tarp_slack(p)		do { tarp_ctrl.param =((tarp_ctrl.param & 0xF9) | (((p) & 3) << 1)); } while (0)
#define set_tarp_drop_weak(p)	do { tarp_ctrl.param =((tarp_ctrl.param & 0xF7) | (((p) & 1) << 3)); } while (0)
#define set_tarp_rte_rec(p)		do { tarp_ctrl.param =((tarp_ctrl.param & 0xCF) | (((p) & 3) << 4)); } while (0)
#define set_tarp_level(p)		do { tarp_ctrl.param =((tarp_ctrl.param & 0x3F) | (((p) & 3) << 6)); } while (0)

extern  tarpCtrlType	tarp_ctrl;
extern  nid_t		net_id;
extern  nid_t		local_host;
extern  nid_t		master_host;
#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
// N=0 XP=7 CAV=0
#define DEF_TARP_PXOPTS	0x7000
extern	word		tarp_pxopts;
#endif

int tr_offset (headerType*);
Boolean msg_isBind (msg_t);
Boolean msg_isTrace (msg_t);
Boolean msg_isMaster (msg_t);
Boolean msg_isNew (msg_t);
Boolean msg_isClear (byte);
#if TARP_RTR
word guide_rtr (headerType*);
#endif
void set_master_chg (void);

#endif