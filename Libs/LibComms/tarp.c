/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "tarp.h"
#include "app_tarp_if.h"
#include "msg_tarp.h"
#include "diag.h"

#define TARP_CACHES_MALLOCED    0

tarpCountType tarp_count = {0, 0, 0};
word	tarp_level = 2;
word    tarp_slack = 0;

#if TARP_CACHES_MALLOCED
static ddcType  * ddCache = NULL;
static spdcType * spdCache = NULL;
#else
static ddcType  _ddCache;
static spdcType _spdCache;
static ddcType  * ddCache  = &_ddCache;
static spdcType * spdCache = &_spdCache;
#endif

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

static bool dd_fresh (headerType * buffer) {
	int i;

	if (buffer->snd == master_host) {
		if (ddCache->m_seq == buffer->seq_no)
			return false;
		ddCache->m_seq = buffer->seq_no;
		return true;
	}
	if (msg_isMaster (buffer->msg_type) && (
		buffer->rcv == 0 || buffer->rcv == local_host)) {
		master_host = buffer->snd; // my master ** kludge for upd_spd
		ddCache->m_seq = buffer->seq_no;
		spdCache->m_hop = buffer->hoc <<8; // is simplest best?
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
			 buffer->seq_no == ddCache->en[i].seq)
			 return false;
#else
		 if (buffer->snd == ddCache->en[i].host) {
			 if (buffer->seq_no == ddCache->en[i].seq)
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


static void dumpMsg (headerType * msg) {

	net_diag (D_INFO, "Msg(%x): seq(%u) snd(%lu) rcv(%lu) hoc(%u) hco(%u)", 
		msg->msg_type, msg->seq_no, msg->snd,
		msg->rcv, msg->hoc, msg->hco);

}

// rssi may be involved to filter out sources blinking on perimeter,
// if so, do it on a tarp_option
static void upd_spd (headerType * msg) {
	word i;
	if (msg->snd == master_host) {
		spdCache->m_hop = msg->hoc <<8; // clears retries or empty write
		return;
	}
	if ((i = tarp_findInSpd (msg->snd)) < spdCacheSize) {
		spdCache->en[i].hop = msg->hoc <<8;
		return;
	}
	spdCache->en[spdCache->head].host = msg->snd;
	spdCache->en[spdCache->head].hop = msg->hoc <<8;
	if (++spdCache->head >= spdCacheSize)
		spdCache->head = 0;
	return;
}

static int check_spd (headerType * msg) {
	int i, j;

	if (msg->rcv == master_host) {
		i = (msg->hco > 0 ? msg->hco : tarp_maxHops) - msg->hoc -
			(spdCache->m_hop >>8) + (spdCache->m_hop & 0x00FF) +
			tarp_slack;;
		if (i < 0)
			spdCache->m_hop++;
		net_diag (D_DEBUG, "Spd(m) %d.%d:%d", spdCache->m_hop >>8,
			spdCache->m_hop & 0x00FF, i);
		return i;
	}

	if ((i = tarp_findInSpd(msg->rcv)) >= spdCacheSize)
		return tarp_maxHops;

	j = (msg->hco > 0 ? msg->hco : tarp_maxHops) - msg->hoc -
		(spdCache->en[i].hop >>8) + (spdCache->en[i].hop & 0x00FF) +
		tarp_slack;;
	if (j < 0)
		spdCache->en[i].hop++; // failed retries ++
	net_diag (D_DEBUG, "Spd[%d] %d.%d:%d", i, spdCache->en[i].hop >>8,
			spdCache->en[i].hop & 0x00FF, j);
	return j;
}

int tarp_rx (address buffer, int length, int *ses) {

	address	dup;
	headerType * msgBuf = (headerType *)(buffer+1);
	id_t sorig, rorig, llhost;  // crap

	tarp_count.rcv++;
	++msgBuf->hoc;
	sorig = msgBuf->snd;
	rorig = msgBuf->rcv;
	llhost = wtonl(local_host);
	msgBuf->snd = ntowl(sorig);
	msgBuf->rcv = ntowl(rorig);

	if (length == 0) { // nothing in the recv buffer... can it happen?
		net_diag (D_WARNING, "Zero length");
		return TCV_DSP_DROP;
	}
	if (msg_isProxy (msgBuf->msg_type)) {
		net_diag (D_DEBUG, "Proxy (%lu, %u)", msgBuf->snd,
				msgBuf->seq_no);
		return msgBuf->rcv == 0 || msgBuf->rcv == local_host ?
			TCV_DSP_RCV : TCV_DSP_DROP;
	}
	/*/ demo cheat CHEAT Cheat
	// that was for meter demo -- 1,4 2,5 could not talked directly
	if (msgBuf->hoc == 1 && local_host + msgBuf->snd == 5) {
		net_diag (D_DEBUG, "Drop direct");
		return TCV_DSP_DROP;
	}
	*/
	if (msgBuf->snd == local_host) { // my own -- drop?
		net_diag (D_DEBUG, "Echo (%u, %u)", msgBuf->seq_no,
			msgBuf->hoc);
		return TCV_DSP_DROP;
	}
	if (tarp_level && !dd_fresh(msgBuf))	 // check and update DD
		return TCV_DSP_DROP;	//duplicate

	if (tarp_level > 1)
		upd_spd (msgBuf);

	if (msgBuf->rcv == local_host)
		return TCV_DSP_RCV;

	if (msgBuf->rcv == 0) {
		if ((dup = tcvp_new (msg_isTrace (msgBuf->msg_type) ?
		  length +sizeof(id_t) : length, TCV_DSP_XMT, *ses)) == NULL)
			net_diag (D_WARNING, "Tarp dup failed");
		else {
			memcpy ((char *)dup, (char *)buffer, length);
			// another kludge alert:
			((headerType *)(dup+1))->snd = sorig;
			((headerType *)(dup+1))->rcv = rorig;
			if (msg_isTrace (msgBuf->msg_type)) // crap kludge
			  memcpy ((char *)dup + tr_offset (msgBuf),
				(char *)&llhost, sizeof(id_t));
			tarp_count.fwd++;
			net_diag (D_DEBUG, "Forwarding:");
			dumpMsg(msgBuf);
		}
		return TCV_DSP_RCV; // the original
	}

	if (msgBuf->hoc >= tarp_maxHops) {
		net_diag (D_WARNING, "Hoc maxed dropped");
		dumpMsg(msgBuf);
		return TCV_DSP_DROP;
	}

	if (tarp_level <= 1 || check_spd (msgBuf) >= 0) {
		// restore as from the net:
		msgBuf->snd = sorig;
		msgBuf->rcv = rorig;

		if (!msg_isTrace (msgBuf->msg_type))
			return TCV_DSP_XMT;
		if ((dup = tcvp_new (length +sizeof(id_t), TCV_DSP_XMT, *ses))
			== NULL)
			net_diag (D_WARNING, "Tarp dup2 failed");
		else {
			memcpy ((char *)dup, (char *)buffer, length);
			memcpy ((char *)dup + tr_offset (msgBuf),
				(char *)&llhost, sizeof(id_t));
		}
	}
	return TCV_DSP_DROP;
}

word    tarp_flags		= 0;
static char tarp_cyclingSeq	= 0;

static void setHco (headerType * msg) {
	word i;

	if (tarp_level < 2 || msg_isProxy (msg->msg_type) || msg->rcv == 0) {
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
	msgBuf->seq_no = ++tarp_cyclingSeq;
	msgBuf->snd = wtonl(local_host);
	msgBuf->rcv = wtonl(msgBuf->rcv);

	// clear flags (meant: exceptions) every time tarp_tx is called
	tarp_flags = 0;
	return rc;
}
