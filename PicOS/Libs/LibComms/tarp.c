/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010 			*/
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

#ifndef _TARP_T_RX
#define _TARP_T_RX	0
#endif

#ifndef _TARP_T_LIGHT
#define _TARP_T_LIGHT	0
#endif

#ifndef _TARP_T_RTR
#define _TARP_T_RTR   0
#endif

#if _TARP_T_RX
#define dbug_rx(a, ...)	diag (a, ## __VA_ARGS__)
#else
#define dbug_rx(a, ...)	CNOP
#endif

#if _TARP_T_LIGHT
#define dbug_lte(a, ...) diag (a, ## __VA_ARGS__)
#else
#define dbug_lte(a, ...) CNOP
#endif

#if _TARP_T_RTR
#define dbug_rtr(a, ...) diag (a, ## __VA_ARGS__)
#else
#define dbug_rtr(a, ...) CNOP
#endif


// (h == master_host) should not get here
 // find the index
__PRIVF (PicOSNode, word, findInSpd) (nid_t host) {
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

#if TARP_RTR

__PRIVF (PicOSNode, word, findInRtr) (nid_t sndr, seq_t seqn, address pkt) {

	sint i;

	if (rtrCache->head)
		i = rtrCache->head;
	else
		i = rtrCacheSize;
	--i;

	while (i != rtrCache->head) {
		if ((pkt != NULL || pkt == NULL && sndr == 0 && seqn == 0) && 
				pkt == rtrCache->pkt[i] || 
		    pkt == NULL && rtrCache->pkt[i] != NULL &&
		      in_header(rtrCache->pkt[i] +1, seq_no) == seqn &&
		      in_header(rtrCache->pkt[i] +1, snd) == sndr) {
#if 0
			diag ("verbose found at %u: %x %u %u", 
					i, pkt, sndr, seqn);
#endif
			return i;
		}
		if (--i < 0)
			i = rtrCacheSize -1;
	}

	//diag ("verbose not found %x %u %u", pkt, sndr, seqn);
	return rtrCacheSize;
}

__PRIVF (PicOSNode, void, ackForRtr) (headerType * b, int * ses) {
	address dum;

	if (guide_rtr (b) < 2) // not interesting msg
		return;


	if ((dum = tcvp_new (sizeof (headerType) + 6, TCV_DSP_XMTU, *ses)) ==
			NULL) {
		dbug_rtr ("no dummy ack");
		return;
	}
	*dum = net_id;
	dum[1] = 0; // msg_null - hopefully, invalidates all else
	memcpy ((char *)dum +3, (char *)b +1, sizeof (headerType) -1);
#if 0
	diag ("verbose dummy out for s%u s%u t%u r%u", b->snd, b->seq_no,
			b->msg_type, b->rcv);
#endif
}
#endif

__PUBLF (PicOSNode, void, tarp_init) (void) {
#if TARP_CACHES_MALLOCED
	ddCache = (ddcType *)
		umalloc (sizeof(ddcType));
	if (ddCache == NULL)
		syserror (EMALLOC, "ddCache");

	spdCache = (spdcType *) umalloc (sizeof(spdcType));
	if (spdCache == NULL)
		syserror (EMALLOC, "spdCache");
#if TARP_RTR
	rtrCache = (rtrcType *) umalloc (sizeof(rtrcType));
	if (rtrCache == NULL)
		syserror (EMALLOC, "rtrCache");
#endif

#endif
	memset (ddCache, 0,  sizeof(ddcType));
	memset (spdCache, 0, sizeof(spdcType));
#if TARP_RTR
	memset (rtrCache, 0, sizeof(rtrcType));
	rtrCache->fecnt = rtrCacheSize;
#endif
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
	if ((i = findInSpd (msg->snd)) < spdCacheSize) {
		spdCache->en[i].hop = (word)(msg->hoc & 0x7F) << 8;
		return;
	}
	spdCache->en[spdCache->head].host = msg->snd;
	spdCache->en[spdCache->head].hop = (word)(msg->hoc & 0x7F) << 8;
	if (++spdCache->head >= spdCacheSize)
		spdCache->head = 0;
	return;
}

/* check_spd assesses deviation from an shortest route (so far):
   > 0 -- shortcut by so many hops
   0   -- on an optimal route
   < 0 -- detour by so many hops

   If there is no entry present, the assessment is based on the packet data,
   adjusted with TARP settings. Roughly, it is number of hops 'to go' -1.

   The packet should not be forwarded if the function returns a negative value.
*/
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
		if (i < 0 && tarp_rte_rec != 0)
			spdCache->m_hop++;

		dbug_rx ("%u %u spdm %d %u %u %u %u", 
			msg->msg_type, msg->snd, i,
			msg->hco, msg->hoc, spdCache->m_hop, msg->seq_no);

		return i;
	}

	if ((i = findInSpd(msg->rcv)) >= spdCacheSize) {

		dbug_rx ("%u %u spdno %d %u %u %u", msg->msg_type, msg->snd,
			msg->hco - (msg->hoc & 0x7F) + tarp_slack -1,
			msg->hco, msg->hoc, msg->seq_no);

		return msg->hco - (msg->hoc & 0x7F) + tarp_slack -1;
	}

	j = (msg->hco > 0 ? msg->hco : tarp_maxHops) -
		(msg->hoc & 0x7F) -
		(spdCache->en[i].hop >>8) +
		((spdCache->en[i].hop & 0x00FF) >> tarp_rte_rec) +
		tarp_slack;
	if (j < 0 && tarp_rte_rec != 0)
		spdCache->en[i].hop++; // failed routing attempts ++

	dbug_rx ("%u %u spd %d %u %u %u %u", 
			msg->msg_type, msg->snd, j, msg->hco,
	  		msg->hoc, spdCache->en[i].hop, msg->seq_no);

	return j;
}

__PUBLF (PicOSNode, int, tarp_rx) (address buffer, int length, int *ses) {

	address dup;
	headerType * msgBuf = (headerType *)(buffer+1);
	byte rssi;

#if TARP_RTR
	word i;
#endif
	if (length < sizeof(headerType) + 6) { // sid, entropy, rssi
		diag ("tarp rcv bad length %d", length);
		return TCV_DSP_DROP;
	}

	if (msgBuf->msg_type == 0) { // dummy ack from dst
#if TARP_RTR
#if 0
		diag ("verbose dummy in for s%u s%u r%u", 
				msgBuf->snd, msgBuf->seq_no,
				msgBuf->rcv);
#endif
		if (guide_rtr (msgBuf) > 0 &&
			(i = findInRtr (msgBuf->snd, msgBuf->seq_no, NULL)) <
			       	rtrCacheSize) {
			 dbug_rtr ("%x-%u.%u unhooked-dum at %u %u",
					rtrCache->pkt[i],
                                        msgBuf->snd, msgBuf->seq_no,
                                        i, rtrCache->fecnt +1);
// PG =========================================================================
// Should have a function for this ============================================
// ============================================================================
			if (tcvp_issettimer (rtrCache->pkt [i]) ||
			    tcvp_isqueued (rtrCache->pkt [i])) {
				// The timer is running or queued to transmit,
				// feel free to kill
				tcv_drop (rtrCache->pkt[i]);
				rtrCache->fecnt++;
			} else {
				// Being transmitted, make sure for the
				// last time
				rtrCache->rcnt[i] = TARP_RTR;
			}
// ============================================================================
		}
#endif
		return TCV_DSP_DROP;
	}

#if TARP_RTR
	if (guide_rtr (msgBuf)  == 2 && 
		(i = findInRtr (msgBuf->snd, msgBuf->seq_no, NULL)) <
			rtrCacheSize) {
		dbug_rtr ("%x-%u.%u unhooked-ack at %u %u (%x %u %u)",
                                        rtrCache->pkt[i], 
                                        msgBuf->snd, msgBuf->seq_no,
                                        i, rtrCache->fecnt +1,
					tcvp_gethook (rtrCache->pkt [i]),
					tcvp_isqueued (rtrCache->pkt [i]),
					tcvp_issettimer (rtrCache->pkt [i])
			);
// PG =========================================================================
			if (tcvp_issettimer (rtrCache->pkt [i]) ||
			    tcvp_isqueued (rtrCache->pkt [i])) {
				// The timer is running or queued to transmit,
				// feel free to kill
				tcv_drop (rtrCache->pkt[i]);
				rtrCache->fecnt++;
			} else {
				// Being transmitted, make sure for the
				// last time
				rtrCache->rcnt[i] = TARP_RTR;
			}
// ============================================================================
	}
#endif

#if DM2200
	// RSSI on TR8100 is LSB, MSB is 0
	tarp_ctrl.ssignal = ((rssi = buffer[(length >>1) -1]) >=
			tarp_ctrl.rssi_th) ?  YES : NO;
#else
	// assuming CC1100: RSSI is MSB
	tarp_ctrl.ssignal = ((rssi = (buffer[(length >>1) -1] >> 8)) >=
			tarp_ctrl.rssi_th) ?  YES : NO;
#endif

	dbug_lte ("%u %u %u rcv %u", msgBuf->msg_type, msgBuf->snd,
			msgBuf->seq_no, (word)seconds());

	dbug_rx ("%u %u ssig %u drop %u", msgBuf->msg_type, msgBuf->snd,
		tarp_ctrl.ssignal, tarp_drop_weak);

	if (!tarp_ctrl.ssignal) {
		dbg_8 (0x0600 | msgBuf->hoc + 1);
		if (tarp_drop_weak) {
			return TCV_DSP_DROP;
		}
	}

	tarp_ctrl.rcv++;
	if (*buffer == 0)  { // from unbound node

		dbug_rx ("%u %u from unbound %s", msgBuf->msg_type, msgBuf->snd,
			net_id == 0 || !msg_isNew(msgBuf->msg_type) ?  
				"drop" : "rcv");
		return net_id == 0 || !msg_isNew(msgBuf->msg_type) ?
			TCV_DSP_DROP : TCV_DSP_RCV;
	}

	if (msgBuf->snd == local_host) {
		// my own echo -- drop it
		dbug_rx ("%u %u drop echo", msgBuf->msg_type, msgBuf->snd);
		return TCV_DSP_DROP;
	}

	if (net_id == 0 && !msg_isBind (msgBuf->msg_type)) {

		dbug_rx ("%u %u drop no net_id", msgBuf->msg_type, msgBuf->snd);
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

#if TARP_RTR
	// ack duplicates as well - otherwise the 1st lost ack
	// invalidates retries
	if (msgBuf->rcv == local_host)
		ackForRtr (msgBuf, ses);
#endif

	if (tarp_level && !dd_fresh(msgBuf)) {  // check and update DD

		dbug_rx ("%u %u drop dup %u", msgBuf->msg_type, msgBuf->snd,
			msgBuf->seq_no);
		return TCV_DSP_DROP;    //duplicate
	}

	if (tarp_level > 1 && tarp_ctrl.ssignal)
		upd_spd (msgBuf);

	if (msgBuf->rcv == local_host) {
		dbug_rx ("%u %u rcv is me %u", msgBuf->msg_type, msgBuf->snd, 
			msgBuf->seq_no);
		return TCV_DSP_RCV;
	}

	if (msgBuf->rcv == 0) {
#if TARP_RTR
		// if we ever want bcast retried... for now, this is redundant
		ackForRtr (msgBuf, ses);
#endif
		if (!tarp_fwd_on || (msgBuf->hoc & 0x7F) >= msgBuf->hco) {

		dbug_rx ("%u %u bcast just rcv %u", msgBuf->msg_type, 
				msgBuf->snd, msgBuf->seq_no);
			return TCV_DSP_RCV;
		}

		if ((dup = tcvp_new (msg_isTrace (msgBuf->msg_type) ?
		  length +sizeof(nid_t) : length, TCV_DSP_XMT, *ses)) == NULL) {
			dbg_8 (0x2000); // Tarp dup failed
			diag ("Tarp dup failed");
		} else {
			memcpy ((char *)dup, (char *)buffer, length);
			if (msg_isTrace (msgBuf->msg_type)) { // crap kludge
#if 0
			  memcpy ((char *)dup + tr_offset (msgBuf),
				(char *)&local_host, sizeof(nid_t));
#endif
			  *((char *)dup + tr_offset (msgBuf)) = 
				  (byte)local_host;
			  *((char *)dup + tr_offset (msgBuf) +1) = rssi;
			}
			tarp_ctrl.fwd++;
		}
		dbug_rx ("%u %u bcast cpy & rcv %u", msgBuf->msg_type, 
			msgBuf->snd, msgBuf->seq_no);
		return TCV_DSP_RCV; // the original
	}

	if ((msgBuf->hoc & 0x7F) >= tarp_maxHops) {

		dbug_rx ("%u %u Max drop %d", msgBuf->msg_type, msgBuf->snd, 
			msgBuf->seq_no);
		return TCV_DSP_DROP;
	}

	if (tarp_fwd_on &&
		(tarp_level < 2 || check_spd (msgBuf) >= 0)) {
		tarp_ctrl.fwd++;
#if TARP_RTR
		ackForRtr (msgBuf, ses);
#endif
		if (!msg_isTrace (msgBuf->msg_type)) {

			dbug_rx ("%u %u xmit %u", msgBuf->msg_type, 
					msgBuf->snd, msgBuf->seq_no);
			return TCV_DSP_XMT;
		}

		if ((dup = tcvp_new (length +sizeof(nid_t), TCV_DSP_XMT, *ses))
			== NULL) {
			dbg_8 (0x3000); // Tarp dup2 failed
			diag ("Tarp dup2 failed");
		} else {
			memcpy ((char *)dup, (char *)buffer, length);
#if 0
			memcpy ((char *)dup + tr_offset (msgBuf),
				(char *)&local_host, sizeof(nid_t));
#endif
			*((char *)dup + tr_offset (msgBuf)) = (byte)local_host;
			*((char *)dup + tr_offset (msgBuf) +1) = rssi;
			dbug_rx ("%u %u cpy trace", 
					msgBuf->msg_type, msgBuf->snd);
		}
	}

	dbug_rx ("%u %u (&) drop %u", msgBuf->msg_type, msgBuf->snd, 
		msgBuf->seq_no);
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

	if ((i = findInSpd(msg->rcv)) < spdCacheSize)
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

	dbug_lte ("%u %u %u snd %u", msgBuf->msg_type, msgBuf->rcv,
		msgBuf->seq_no, (word)seconds());

	dbug_rx ("%u %u tx %u %u %u", msgBuf->msg_type, msgBuf->snd,
			msgBuf->rcv, msgBuf->hco, msgBuf->seq_no);
	return rc;
}

#if TARP_RTR
__PUBLF (PicOSNode, int, tarp_xmt) (address buffer) {

        headerType * msgBuf = (headerType *)(buffer+1);
	word i;

	// msg_null - ack at dst || irrelevant
	if (msgBuf->msg_type == 0 || guide_rtr (msgBuf) < 2)
		return TCV_DSP_DROP;


	if ((i = findInRtr (0, 0, buffer)) < rtrCacheSize) {

		if (++rtrCache->rcnt[i] > TARP_RTR) {
			rtrCache->fecnt++;
			dbug_rtr ("%x-%u.%u t%u r%u unhooked-rtr at %u %u",
                                        rtrCache->pkt[i],
					in_header(buffer +1, snd),
					in_header(buffer +1, seq_no),
					in_header(buffer +1, msg_type),
					in_header(buffer +1, rcv),
                                        i, rtrCache->fecnt);
			return TCV_DSP_DROP; // tcv will clear the hook
		}

               dbug_rtr ("rtr %u %x-%u.%u", rtrCache->rcnt[i],
                                rtrCache->pkt[i],
                                in_header(buffer +1, snd),
                                in_header(buffer +1, seq_no));

CacheIt:
		tcvp_settimer (buffer, TARP_RTR_TOUT);
		return TCV_DSP_PASS;
	}

	if (rtrCache->fecnt > 0) {
		if ((i = findInRtr (0, 0, 0)) < rtrCacheSize) {
			tcvp_hook (buffer, &rtrCache->pkt[i]);
			rtrCache->rcnt[i] = 0;
			rtrCache->fecnt--;
			dbug_rtr ("%x-%u.%u hooked at %u %u", buffer,
					in_header(buffer +1, snd),
					in_header(buffer +1, seq_no),
 					i, rtrCache->fecnt);

		} else { // something pojebalos
			dbug_rtr ("no place? %d", rtrCache->fecnt);
			return TCV_DSP_DROP;
		}

		goto CacheIt;
	}
#if 0
// we'll see if this early funeral is needed
	// if the cache is full, drop if the oldest had no retry
	if (rtrCache->rcnt[rtrCache->head] > 0) {
		dbug_rtr ("%x used %u hook upd %x-%u.%u at %u %u",
			       rtrCache->pkt[rtrCache->head], 
			       rtrCache->rcnt[rtrCache->head], buffer,
                                        in_header(buffer +1, snd),
                                        in_header(buffer +1, seq_no),
                                        i, rtrCache->fecnt);

		tcv_drop (rtrCache->pkt[rtrCache->head]);
		tcvp_hook (buffer, &rtrCache->pkt[rtrCache->head]);
		rtrCache->rcnt[rtrCache->head] = 0;

		if (++rtrCache->head >= rtrCacheSize)
			rtrCache->head = 0;

		goto CacheIt;
        }
#endif

	dbug_rtr ("rtr full %u", rtrCache->rcnt[rtrCache->head]);
	return TCV_DSP_DROP;
}
#endif

