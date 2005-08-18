/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* KLUDGE ALERT
   h_flags & 0x8000 carries strong signal indicator
   bits 0x7000 are spare
   the rest is a transient host id for neighbourhood simulation
*/
#include "sysio.h"
#include "tarp.h"
#include "app_tarp_if.h"
#include "msg_tarp.h"

#define TARP_CACHES_MALLOCED    0

// rcv, snd, fwd:
tarpCountType tarp_count = {0, 0, 0};

// level, slack, rte_rec, nhood
tarpParamType tarp_param = {2, 0, 1, 0};

#if TARP_CACHES_MALLOCED
static ddcType  * ddCache = NULL;
static spdcType * spdCache = NULL;
#else
static ddcType  _ddCache;
static spdcType _spdCache;
static ddcType  * ddCache  = &_ddCache;
static spdcType * spdCache = &_spdCache;
#endif

// set a rssi threshold for spd updates
#define SPD_RSSI_THRESHOLD	2
static bool strong_signal = YES;

// (h == master_host) should not get here
 // find the index
static word tarp_findInSpd (id_t host) {
	int i;

	if (host == 0)
		return spdCacheSize;

	if (spdCache->head)
		i = spdCache->head;
	else
		i = spdCacheSize;
	--i;

	while (i != spdCache->head) {
		if (host == spdCache->en[i].host)
			return i;
		if (--i < 0)
			i = spdCacheSize -1;
	}
	return spdCacheSize;
}

void tarp_init() {
#if TARP_CACHES_MALLOCED
	ddCache = (ddcType *)
		umalloc (sizeof(ddcType));
	if (ddCache == NULL)
		syserror (EMALLOC, "ddCache");

	spdCache = (spdcType *) umalloc (sizeof(spdcType));
	if (spdCache == NULL)
		syserror (EMALLOC, "spdCache");
#endif
	memset (ddCache, 0,  sizeof(ddcType));
	memset (spdCache, 0, sizeof(spdcType));
}

// !FAST_HOSTS is not only about slower msg ratio, but also about skewed
// DD presence for most active hosts... still worth it, I think.
#define FAST_HOSTS 0

// it says: whatever is more than 3 packets behind is a duplicate
// (in the shadow of the dd entry)
#define CBUFSIZE 256
#define SHADOW 4
#define in_shadow(c, m) ((((m) <= (c) && (m) > (c) - SHADOW) || \
			((m) > (c) && (m) > CBUFSIZE - SHADOW + (c))) ? 1 : 0)

static bool dd_fresh (headerType * buffer) {
	int i;

	if (buffer->snd == master_host) {
		if (in_shadow(ddCache->m_seq, buffer->seq_no))
			return false;
		ddCache->m_seq = buffer->seq_no;
		return true;
	}
	if (msg_isMaster (buffer->msg_type) && (
		buffer->rcv == 0 || buffer->rcv == local_host)) {
		master_host = buffer->snd; // my master ** kludge for upd_spd
		ddCache->m_seq = buffer->seq_no;
		if (strong_signal)
			spdCache->m_hop = (word)buffer->hoc <<8; // is simplest best?
		return true;
	}

	if (ddCache->head)
		i = ddCache->head;
	 else
		i = ddCacheSize;
	 --i;

	 while (i != ddCache->head) {
#if FAST_HOSTS
		 if (buffer->snd == ddCache->en[i].host &&
			 in_shadow(ddCache->en[i].seq, buffer->seq_no))
			 return false;
#else
		 if (buffer->snd == ddCache->en[i].host) {
			 if (in_shadow(ddCache->en[i].seq, buffer->seq_no))
				 return false;
			 ddCache->en[i].seq = buffer->seq_no;
			 return true;
		 }
#endif
		 if (--i < 0)
			 i = ddCacheSize -1;
	 }
	 ddCache->en[ddCache->head].host = buffer->snd;
	 ddCache->en[ddCache->head].seq = buffer->seq_no;
	 if (++ddCache->head >= ddCacheSize)
		 ddCache->head = 0;
	 return true;
}

// rssi may be involved to filter out sources blinking on perimeter,
// if so, do it on a tarp_option
static void upd_spd (headerType * msg) {
	word i;
	if (msg->snd == master_host) {
		spdCache->m_hop = (word)msg->hoc <<8; // clears retries or empty write
		return;
	}
	if ((i = tarp_findInSpd (msg->snd)) < spdCacheSize) {
		spdCache->en[i].hop = (word)msg->hoc <<8;
		return;
	}
	spdCache->en[spdCache->head].host = msg->snd;
	spdCache->en[spdCache->head].hop = (word)msg->hoc <<8;
	if (++spdCache->head >= spdCacheSize)
		spdCache->head = 0;
	return;
}

static int check_spd (headerType * msg) {
	int i, j;

	if (msg->rcv == master_host) {
		i = (msg->hco > 0 ? msg->hco : tarp_maxHops) - msg->hoc -
			(spdCache->m_hop >>8) +
			((spdCache->m_hop & 0x00FF) >> tarp_param.rte_rec) +
			tarp_param.slack;
		if (i < 0)
			spdCache->m_hop++;
		return i;
	}

	if ((i = tarp_findInSpd(msg->rcv)) >= spdCacheSize) {
		return tarp_maxHops;
	}

	j = (msg->hco > 0 ? msg->hco : tarp_maxHops) - msg->hoc -
		(spdCache->en[i].hop >>8) +
	       	((spdCache->en[i].hop & 0x00FF) >> tarp_param.rte_rec) +
		tarp_param.slack;
	if (j < 0)
		spdCache->en[i].hop++; // failed retries ++
	return j;
}

//#if NHOOD
static bool in_range (word h) {
	if (tarp_param.nhood == 0)
		return YES;
	// "in line"
	//return local_host == h+1 || local_host == h-1;
	/*
	  5-6...
	 / \
	3   4
	|  /
	2-1     i+12 (13..18, as on dm2100)
	*/
	switch (local_host) {
		case 13:
			return h == 14 || h == 16;
		case 14:
			return h == 15 || h == 13;
		case 15:
			return h == 14 || h == 17;
		case 16:
			return h == 13 || h == 17;
		case 17:
			return h == 15 || h == 16 || h == 18;
		default:
			return local_host == h +1 || local_host == h-1;
	}
	//*/
}
//#endif

int tarp_rx (address buffer, int length, int *ses) {

	address	dup;
	headerType * msgBuf = (headerType *)(buffer+1);

	tarp_count.rcv++;
	++msgBuf->hoc;

	if (length == 0) { // nothing in the recv buffer... can it happen?
		//net_diag (D_WARNING, "Zero length");
		return TCV_DSP_DROP;
	}

	if (tarp_param.nhood != 0) {
		if (!in_range(msgBuf->h_flags & 0x0FFF)) {
			return TCV_DSP_DROP;
		}
		msgBuf->h_flags &= 0xF000; // out with previous node
		msgBuf->h_flags |= local_host; // add lh
	}

	if (msg_isProxy (msgBuf->msg_type)) {
		return msgBuf->rcv == 0 || msgBuf->rcv == local_host ?
			TCV_DSP_RCV : TCV_DSP_DROP;
	}
	if (msgBuf->snd == local_host) { // my own -- drop?
		return TCV_DSP_DROP;
	}
	if (msgBuf->h_flags & 0x8000)
		strong_signal = NO;
	else {
		strong_signal =
		  buffer[(length >>1) -1] > SPD_RSSI_THRESHOLD ? YES : NO;
		if (!strong_signal)
			msgBuf->h_flags |= 0x8000;
	}
	if (tarp_param.level && !dd_fresh(msgBuf)) {	 // check and update DD
		//diag ("N %d %d %d", msgBuf->snd, msgBuf->seq_no, msgBuf->hoc);
		return TCV_DSP_DROP;	//duplicate
	}
	//diag ("Y %d %d %d", msgBuf->snd, msgBuf->seq_no, msgBuf->hoc);

	if (tarp_param.level > 1 && strong_signal)
		upd_spd (msgBuf);

	if (msgBuf->rcv == local_host)
		return TCV_DSP_RCV;

	if (msgBuf->rcv == 0) {
		if ((dup = tcvp_new (msg_isTrace (msgBuf->msg_type) ?
		  length +sizeof(id_t) : length, TCV_DSP_XMT, *ses)) == NULL)
			diag ("Tarp dup failed");
		else {
			memcpy ((char *)dup, (char *)buffer, length);
			if (msg_isTrace (msgBuf->msg_type)) // crap kludge
			  memcpy ((char *)dup + tr_offset (msgBuf),
				(char *)&local_host, sizeof(id_t));
			tarp_count.fwd++;
		}
		return TCV_DSP_RCV; // the original
	}

	if (msgBuf->hoc >= tarp_maxHops) {
		//diag ("Max %d %d", msgBuf->snd, msgBuf->seq_no);
		return TCV_DSP_DROP;
	}

	if (tarp_param.level <= 1 || check_spd (msgBuf) >= 0) {
		tarp_count.fwd++;
		if (!msg_isTrace (msgBuf->msg_type))
			return TCV_DSP_XMT;
		if ((dup = tcvp_new (length +sizeof(id_t), TCV_DSP_XMT, *ses))
			== NULL)
			diag ("Tarp dup2 failed");
		else {
			memcpy ((char *)dup, (char *)buffer, length);
			memcpy ((char *)dup + tr_offset (msgBuf),
				(char *)&local_host, sizeof(id_t));
		}
	}
	return TCV_DSP_DROP;
}

word    tarp_flags		= 0;
static word tarp_cyclingSeq	= 0;

static void setHco (headerType * msg) {
	word i;

	if (tarp_param.level < 2 || msg_isProxy (msg->msg_type) ||
			msg->rcv == 0) {
		msg->hco = 0;
		return;
	}

	if (msg->rcv == master_host) {
		msg->hco = (spdCache->m_hop>>8) + (tarp_flags & 0x000F);
		return;
	}

	if ((i = tarp_findInSpd(msg->rcv)) < spdCacheSize)
		msg->hco = (spdCache->en[i].hop >>8) +
			(tarp_flags & 0x000F);

	else
		msg->hco = 0;
}

int tarp_tx (address buffer) {
	headerType * msgBuf = (headerType *)(buffer+1);
	int rc = (tarp_flags & TARP_URGENT ? TCV_DSP_XMTU : TCV_DSP_XMT);

	tarp_count.snd++;
	msgBuf->hoc = 0;
	setHco(msgBuf);
	if (++tarp_cyclingSeq & 0xFF00)
		tarp_cyclingSeq = 1;
	msgBuf->seq_no = tarp_cyclingSeq;
	if (tarp_param.nhood != 0)
		msgBuf->h_flags |= (local_host & 0x0FFF); // add lh
	msgBuf->snd = local_host;
	 msgBuf->h_flags = local_host;
	// clear flags (meant: exceptions) every time tarp_tx is called
	tarp_flags = 0;
	return rc;
}
