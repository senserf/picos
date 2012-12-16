/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "tcvplug.h"

static int tcv_ope_boss (int, int, va_list);
static int tcv_clo_boss (int, int);
static int tcv_rcv_boss (int, address, int, int*, tcvadp_t*);
static int tcv_frm_boss (address, tcvadp_t*);
static int tcv_out_boss (address);
static int tcv_xmt_boss (address);

const tcvplug_t plug_boss =
		{ tcv_ope_boss, tcv_clo_boss, tcv_rcv_boss, tcv_frm_boss,
			tcv_out_boss, tcv_xmt_boss, NULL,
				0x00E1 /* Plugin Id */ };

const word boss_wnone = WNONE;

#ifndef	__SMURPH__
#include "plug_boss_node_data.h"
#endif

#include "plug_boss.h"

// Two element array for two descriptors: AB, DIRECT
#define	boss_fdsc	_dac (PicOSNode, boss_fdsc)
#define	boss_exp	_dac (PicOSNode, boss_exp)
#define	boss_cur	_dac (PicOSNode, boss_cur)
#define	boss_out	_dac (PicOSNode, boss_out)
#define	boss_out_held	_dac (PicOSNode, boss_out_held)

// Byte access to packet
#define	pkb(p,n)	(((byte*)(p)) [n])

static int tcv_ope_boss (int phy, int fd, va_list ap) {

#if DIAG_MESSAGES > 1
	if (boss_fdsc != NONE)
		syserror (EREQPAR, "boss: ope");
#endif
	boss_fdsc = fd;
	tcvp_control (phy, PHYSOPT_SETSID, (address)&boss_wnone);
	return 0;
}

static int tcv_clo_boss (int phy, int fd) {

	int k;

	if (boss_fdsc == fd) {
		// Remove held packet, if any
		if (boss_out != NULL && boss_out_held)
			tcvp_dispose (boss_out, TCV_DSP_DROP);
		boss_out = NULL;
		boss_out_held = NO;
		boss_exp = boss_cur = 0;
		boss_fdsc = NONE;
		return 0;
	}
	return ERROR;
}

static void sphdr (address pkt, byte pr, byte ack) {

	pkb (pkt, BOSS_PO_PRO) = pr;
	pkb (pkt, BOSS_PO_FLG) = boss_cur | (boss_exp << 1) | ack;
}

static int tcv_rcv_boss (int phy, address p, int len, int *ses,
							     tcvadp_t *bounds) {
	byte bt, ie;
	int ds;

	if ((bt = pkb (p, BOSS_PO_PRO)) != BOSS_PR_DIR && bt != BOSS_PR_ABP)
		// Not one of ours
		return TCV_DSP_PASS;

	if ((*ses = boss_fdsc) == NONE)
		// We have no session yet
		return TCV_DSP_DROP;

	bounds->head = bounds->tail = 0;

	if (bt == BOSS_PR_DIR) {
		// A direct packet -> always receive (as urgent)
		return TCV_DSP_RCVU;
	}

	bt = pkb (p, BOSS_PO_FLG);

	if ((bt & BOSS_BI_ACK) == 0 && (bt & BOSS_BI_CUR) == boss_exp) {
		// Not a pure ACK and expected
		boss_exp ^= 1;
		if (boss_out != NULL)
			// Also flip in the outgoing packet
			pkb (boss_out, BOSS_PO_FLG) ^= BOSS_BI_EXP;
		// and receive
		ds = TCV_DSP_RCV;
	} else {
		ds = TCV_DSP_DROP;
	}

	// boss_exp has been settled
	ie = (bt & BOSS_BI_EXP) >> 1;

	if (boss_out != NULL) {
		// We have an outgoing packet in transit
		if (ie != boss_cur) {
			// Acknowledged
			if (boss_out_held)
				// The packet is being held, so we must scrap
				// it ourselves
				tcv_drop (boss_out);
			// Forget it; it may be too late to stop it, so better
			// don't try
			boss_out = NULL;
			trigger (BOSS_EV_OUT);
		} else {
			// Treat it as a NAK
			if (boss_out_held) {
				boss_out_held = NO;
				tcvp_dispose (boss_out, TCV_DSP_XMT);
			}
		}
	}

	boss_cur = ie;

	// Decide whether to send an explicit ACK
	if (boss_out == NULL && (bt & BOSS_BI_ACK) == 0 && (p = tcvp_new (4,
	    TCV_DSP_XMT, *ses)) != NULL) {
		sphdr (p, BOSS_PR_ABP, BOSS_BI_ACK);
	}

	return ds;
}
		
static int tcv_frm_boss (address p, tcvadp_t *bounds) {

	int res = 0;

	if (p == NULL && bounds->tail == 0) {
		// Called by tcv_wnps for a reliable (non-urgent) packet
		if (boss_out != NULL)
			// Block
			res = (int)(BOSS_EV_OUT);
	}

	bounds->head = bounds->tail = 2;
	return res;
}

static int tcv_out_boss (address p) {

	if (tcvp_isurgent (p)) {
		// Non-reliable packet
		pkb (p, BOSS_PO_PRO) = BOSS_PR_DIR;
		return TCV_DSP_XMTU;
	}

	// Reliable (non-urgent)
	if (boss_out != NULL)
		// Something went wrong; this can happen if multiple
		// processes try to write interleaving wnp/endp
		syserror (ENOTNOW, "boss: out");
	sphdr (p, BOSS_PR_ABP, 0);
	boss_out = p;
	boss_out_held = NO;
	return TCV_DSP_XMT;
}

static int tcv_xmt_boss (address p) {

	if (boss_out == p && !tcvp_isurgent (p)) {
		// Have to hold until acked
		boss_out_held = YES;
		return TCV_DSP_PASS;
	}
	return TCV_DSP_DROP;
}

#undef	boss_fdsc	
#undef	boss_exp
#undef	boss_cur
#undef	boss_out
#undef	boss_out_held
#undef	pkb
