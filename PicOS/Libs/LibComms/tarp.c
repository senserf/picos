/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007.			*/
/* All rights reserved.							*/
/* ==================================================================== */

#ifndef	__SMURPH__

#include "sysio.h"

#endif

#include "tarp.h"
#include "msg_tarp.h"

#ifndef	__SMURPH__
#include "tarp_node_data.h"
#endif

#if TARP_CACHES_TEST

__PUBLF (PicOSNode, int, getSpdCacheSize) () {
	return spdCacheSize;
}

__PUBLF (PicOSNode, int, getDdCacheSize) () {
	return ddCacheSize;
}

__PUBLF (PicOSNode, int, getDd) (int i, word * host, word * seq) {
	*host = _ddCache.node[i];
	*seq  = _ddCache.seq[i];
	return _ddCache.head;
}

__PUBLF (PicOSNode, int, getSpd) (int i, word * host, word * hop) {
	*host = _spdCache.en[i].host;
	*hop  = _spdCache.en[i].hop;
	return _spdCache.head;
}

__PUBLF (PicOSNode, word, getDdM) (word * seq) {
	*seq  = _ddCache.m_seq;
	return master_host;
}

__PUBLF (PicOSNode, word, getSpdM) (word * hop) {
	*hop  = _spdCache.m_hop;
	return master_host;
}

#endif

#define _TARP_T_RX	0

// (h == master_host) should not get here
 // find the index
__PRIVF (PicOSNode, word, tarp_findInSpd) (nid_t host) {
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

__PUBLF (PicOSNode, void, tarp_init) (void) {
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
	tarp_cyclingSeq = rnd() & 0xFF;
}

// !FAST_HOSTS is not only about slower msg ratio, but also about skewed
// DD presence for most active hosts... still worth it, I think.
#define FAST_HOSTS 0

#if 1
// it says: whatever is more than 3 packets behind is a duplicate
// (in the shadow of the dd entry)
#define CBUFSIZE 256
#define SHADOW 4
#define in_shadow(c, m) ((((m) <= (c) && (m) > (c) - SHADOW) || \
			((m) > (c) && (m) > CBUFSIZE - SHADOW + (c))) ? 1 : 0)
#endif

//#define in_shadow(c, m) ((c) == (m))

__PRIVF (PicOSNode, Boolean, dd_fresh) (headerType * buffer) {
	int i;

	if (buffer->snd == master_host) {
		if (in_shadow(ddCache->m_seq, buffer->seq_no))
			return false;
		ddCache->m_seq = buffer->seq_no;
		return true;
	}
	if (msg_isMaster (buffer->msg_type) && (
		buffer->rcv == 0 || buffer->rcv == local_host)) {
		if (master_host != buffer->snd) {
			// kludge for upd_spd
			master_host = buffer->snd;
			set_master_chg ();
		}
		ddCache->m_seq = buffer->seq_no;
		if (tarp_ctrl.ssignal) {
			spdCache->m_hop = (word)(buffer->hoc & 0x7F) << 8;
		}
		return true;
	}

	if (ddCache->head)
		i = ddCache->head;
	 else
		i = ddCacheSize;
	 --i;

	 while (i != ddCache->head) {
#if FAST_HOSTS
		 if (buffer->snd == ddCache->node[i] &&
			 in_shadow(ddCache->seq[i], buffer->seq_no))
			 return false;
#else
		 if (buffer->snd == ddCache->node[i]) {
			 if (in_shadow(ddCache->seq[i], buffer->seq_no))
				 return false;
			 ddCache->seq[i] = buffer->seq_no;
			 return true;
		 }
#endif
		 if (--i < 0)
			 i = ddCacheSize -1;
	 }
	 ddCache->node[ddCache->head] = buffer->snd;
	 ddCache->seq[ddCache->head] = buffer->seq_no;
	 if (++ddCache->head >= ddCacheSize)
		 ddCache->head = 0;
	 return true;
}

// rssi may be involved to filter out sources blinking on perimeter,
// if so, do it on a tarp_option
__PRIVF (PicOSNode, void, upd_spd) (headerType * msg) {
	word i;
	if (msg->snd == master_host) {
		// clears retries or empty write:
		spdCache->m_hop = (word)(msg->hoc & 0x7F) << 8;
		return;
	}
	if ((i = tarp_findInSpd (msg->snd)) < spdCacheSize) {
		spdCache->en[i].hop = (word)(msg->hoc & 0x7F) << 8;
		return;
	}
	spdCache->en[spdCache->head].host = msg->snd;
	spdCache->en[spdCache->head].hop = (word)(msg->hoc & 0x7F) << 8;
	if (++spdCache->head >= spdCacheSize)
		spdCache->head = 0;
	return;
}

__PRIVF (PicOSNode, int, check_spd) (headerType * msg) {
	int i, j;

	if (msg->rcv == master_host) {
		// hco should not be 0 any more... keep it until we can
		// test things properly
		i = (msg->hco > 0 ? msg->hco : tarp_maxHops) -
			(msg->hoc & 0x7F) -
			(spdCache->m_hop >>8) +
			((spdCache->m_hop & 0x00FF) >> tarp_rte_rec) +
			tarp_slack;
		if (i <= 0 && tarp_rte_rec != 0)
			spdCache->m_hop++;

#if _TARP_T_RX
diag ("%u %u spdm %d %u %u", msg->msg_type, msg->snd,  i, msg->hco, msg->hoc);
#endif

		return i;
	}

	if ((i = tarp_findInSpd(msg->rcv)) >= spdCacheSize) {

#if _TARP_T_RX
diag ("%u %u spdno %d %u %u", msg->msg_type, msg->snd, tarp_maxHops,
msg->hco, msg->hoc);
#endif

		return msg->hco - (msg->hoc & 0x7F) + tarp_slack;
	}

	j = (msg->hco > 0 ? msg->hco : tarp_maxHops) -
		(msg->hoc & 0x7F) -
		(spdCache->en[i].hop >>8) +
		((spdCache->en[i].hop & 0x00FF) >> tarp_rte_rec) +
		tarp_slack;
	if (j <= 0 && tarp_rte_rec != 0)
		spdCache->en[i].hop++; // failed routing attempts ++

#if _TARP_T_RX
diag ("%u %u spd %d %u %u", msg->msg_type, msg->snd, j, msg->hco, msg->hoc);
#endif

	return j;
}

__PUBLF (PicOSNode, int, tarp_rx) (address buffer, int length, int *ses) {

	address dup;
	headerType * msgBuf = (headerType *)(buffer+1);

#if DM2200
	// RSSI on TR8100 is LSB, MSB is 0
	tarp_ctrl.ssignal = (buffer[(length >>1) -1] >=
			tarp_ctrl.rssi_th) ?  YES : NO;
#else
	// assuming CC1100: RSSI is MSB
	tarp_ctrl.ssignal = ((buffer[(length >>1) -1] >> 8) >=
			tarp_ctrl.rssi_th) ?  YES : NO;
#endif

#if _TARP_T_RX
diag ("%u %u ssig %u drop %u", msgBuf->msg_type, msgBuf->snd,
  tarp_ctrl.ssignal, tarp_drop_weak);
#endif

	if (!tarp_ctrl.ssignal) {
		dbg_8 (0x0600 | msgBuf->hoc + 1);
		if (tarp_drop_weak) {
			return TCV_DSP_DROP;
		}
	}

	tarp_ctrl.rcv++;
	if (*buffer == 0)  { // from unbound node

#if _TARP_T_RX
diag ("%u %u from unbound %s", msgBuf->msg_type,
  msgBuf->snd, net_id == 0 || !msg_isNew(msgBuf->msg_type) ?  "drop" : "rcv");
#endif
		return net_id == 0 || !msg_isNew(msgBuf->msg_type) ?
			TCV_DSP_DROP : TCV_DSP_RCV;
	}

	if (length == 0 || msgBuf->snd == local_host) {
		// nothing in the recv buffer... can it happen?
		// or my own echo -- drop it
#if _TARP_T_RX
diag ("%u %u drop %s", msgBuf->msg_type, msgBuf->snd,
  length == 0 ? "empty" : "my own");
#endif
		return TCV_DSP_DROP;
	}

	if (net_id == 0 && !msg_isBind (msgBuf->msg_type)) {

#if _TARP_T_RX
diag ("%u %u drop no net_id", msgBuf->msg_type, msgBuf->snd);
#endif
		return TCV_DSP_DROP;
	}

	msgBuf->hoc++;

	if (msgBuf->hoc & 0x80) {
		dbg_8 (0x0700 | msgBuf->hoc);
		tarp_ctrl.ssignal = NO;
	} else {
		if (!tarp_ctrl.ssignal)
			msgBuf->hoc |= 0x80;
	}

	if (tarp_level && !dd_fresh(msgBuf)) {  // check and update DD

#if _TARP_T_RX
diag ("%u %u drop dup", msgBuf->msg_type, msgBuf->snd);
#endif
		return TCV_DSP_DROP;    //duplicate
	}

	if (tarp_level > 1 && tarp_ctrl.ssignal)
		upd_spd (msgBuf);

	if (msgBuf->rcv == local_host) {

#if _TARP_T_RX
diag ("%u %u rcv is me", msgBuf->msg_type, msgBuf->snd);
#endif
		return TCV_DSP_RCV;
	}

	if (msgBuf->rcv == 0) {
		if (!tarp_fwd_on || (msgBuf->hoc & 0x7F) >= msgBuf->hco) {

#if _TARP_T_RX
diag ("%u %u bcast just rcv", msgBuf->msg_type, msgBuf->snd);
#endif
			return TCV_DSP_RCV;
		}

		if ((dup = tcvp_new (msg_isTrace (msgBuf->msg_type) ?
		  length +sizeof(nid_t) : length, TCV_DSP_XMT, *ses)) == NULL) {
			dbg_8 (0x2000); // Tarp dup failed
			diag ("Tarp dup failed");
		} else {
			memcpy ((char *)dup, (char *)buffer, length);
			if (msg_isTrace (msgBuf->msg_type)) // crap kludge
			  memcpy ((char *)dup + tr_offset (msgBuf),
				(char *)&local_host, sizeof(nid_t));
			tarp_ctrl.fwd++;
		}
#if _TARP_T_RX
diag ("%u %u bcast cpy & rcv", msgBuf->msg_type, msgBuf->snd);
#endif
		return TCV_DSP_RCV; // the original
	}

	if ((msgBuf->hoc & 0x7F) >= tarp_maxHops) {

#if _TARP_T_RX
diag ("%u %u Max drop %d", msgBuf->msg_type, msgBuf->snd, msgBuf->seq_no);
#endif
		return TCV_DSP_DROP;
	}

	if (tarp_fwd_on &&
		(tarp_level < 2 || check_spd (msgBuf) > 0)) {
		tarp_ctrl.fwd++;
		if (!msg_isTrace (msgBuf->msg_type)) {

#if _TARP_T_RX
diag ("%u %u xmit", msgBuf->msg_type, msgBuf->snd);
#endif
			return TCV_DSP_XMT;
		}

		if ((dup = tcvp_new (length +sizeof(nid_t), TCV_DSP_XMT, *ses))
			== NULL) {
			dbg_8 (0x3000); // Tarp dup2 failed
			diag ("Tarp dup2 failed");
		} else {
			memcpy ((char *)dup, (char *)buffer, length);
			memcpy ((char *)dup + tr_offset (msgBuf),
				(char *)&local_host, sizeof(nid_t));
#if _TARP_T_RX
diag ("%u %u cpy trace", msgBuf->msg_type, msgBuf->snd);
#endif
		}
	}

#if _TARP_T_RX
diag ("%u %u (&) drop", msgBuf->msg_type, msgBuf->snd);
#endif
	return TCV_DSP_DROP;
}

__PRIVF (PicOSNode, void, setHco) (headerType * msg) {
	word i;

	if (msg->hco != 0) // application decided
		return;
	if (tarp_level < 2 || msg->rcv == 0) {
		msg->hco = tarp_maxHops;
		return;
	}
	if (msg->rcv == master_host) {
		msg->hco = (spdCache->m_hop>>8) + (tarp_ctrl.flags & 0x0F);
		return;
	}

	if ((i = tarp_findInSpd(msg->rcv)) < spdCacheSize)
		msg->hco = (spdCache->en[i].hop >>8) +
			(tarp_ctrl.flags & 0x0F);

	else
		msg->hco = tarp_maxHops;
}

__PUBLF (PicOSNode, int, tarp_tx) (address buffer) {
	headerType * msgBuf = (headerType *)(buffer+1);
	int rc = (tarp_ctrl.flags & TARP_URGENT ? TCV_DSP_XMTU : TCV_DSP_XMT);

	tarp_ctrl.snd++;
	msgBuf->hoc = 0;
	setHco(msgBuf);
	if (++tarp_cyclingSeq & 0xFF00)
		tarp_cyclingSeq = 1;
	msgBuf->seq_no = tarp_cyclingSeq;
	msgBuf->snd = local_host;
	// clear flags (meant: exceptions) every time tarp_tx is called
	tarp_ctrl.flags = 0;
	return rc;
}

#undef _TARP_T_RX
