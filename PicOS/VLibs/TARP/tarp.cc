/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010 			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "tarp.h"
#include "net.h"

// rcv, snd, fwd, |10 10 0 01 1|, pp_urg,pp_widen,spare, rssi_th, ssignal
// param: |level, rte_rec, slack, routing|

tarpCtrlType tarp_ctrl = {0, 0, 0, TARP_DEF_PARAMS, 0,0,0,0,0,
	TARP_DEF_RSSITH, YES};

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
word tarp_pxopts = TARP_DEF_PXOPTS;
#endif

static word tarp_cyclingSeq = 0;

// explicit constants needed, apps will overwrite with derivatives of host_id
nid_t	net_id = 0xAA;

// keep consistent even if not needed (1.81 vs. 1.83 master_host)
// nid_t	_da (master_host) = 0xAA;
nid_t   master_host = 1; 
nid_t	local_host = 0xBB;

/* ================================================================ */

#if TARP_CACHES_MALLOCED

static ddcType	* ddCache = NULL;
static spdcType	* spdCache = NULL;

#if TARP_RTR
static rtrcType * rtrCache = NULL;
#endif

#else	/* not MALLOCED */

static ddcType	_ddCache;
static spdcType 	_spdCache;

#if TARP_RTR
static rtrcType	_rtrCache;
#endif

#define ddCache  (&_ddCache)
#define spdCache (&_spdCache)

#if TARP_RTR
#define rtrCache (&_rtrCache)
#endif

#endif	/* MALLOCED */

#if TARP_CACHES_TEST

int getSpdCacheSize () {
	return spdCacheSize;
}

int getDdCacheSize () {
	return ddCacheSize;
}

int getDd (int i, word * host, word * seq) {
	*host = _ddCache.node[i];
	*seq  = _ddCache.seq[i];
	return _ddCache.head;
}

int getSpd (int i, word * host, word * hop) {
	*host = _spdCache.en[i].host;
	*hop  = _spdCache.en[i].hop;
	return _spdCache.head;
}

word getDdM (word * seq) {
	*seq  = _ddCache.m_seq;
	return master_host;
}

word getSpdM (word * hop) {
	*hop  = _spdCache.m_hop;
	return master_host;
}

#endif

// These are debug options that should stay here (not in options.sys)
#define _TARP_T_RX	0
#define _TARP_T_LIGHT	0
#define _TARP_T_RTR	0

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
static word findInSpd (nid_t host) {

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

static word findInRtr (nid_t sndr, seq_t seqn, address pkt) {

	wint i;

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

static void ackForRtr (headerType * b, int * ses) {

	address dum;

	if (guide_rtr (b) < 2) // not interesting msg
		return;

	if ((dum = tcvp_new (sizeof (headerType) + NET_FRAME_LENGTH,
		TCV_DSP_XMTU, *ses)) ==
			NULL) {
		dbug_rtr ("no dummy ack");
		return;
	}
	*dum = net_id;
	dum [1] = 0; // msg_null - hopefully, invalidates all else
	memcpy ((char *)dum +3, (char *)b +1, sizeof (headerType) -1);

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
	// dupa it'll be an impressive catastrophe if header ever becomes an odd number DO NOT FIX IT HERE
	dum[((sizeof(headerType) + NET_FRAME_LENGTH) >> 1) -1] =
		tarp_pxopts;
#endif

#if 0
	diag ("verbose dummy out for s%u s%u t%x r%u", b->snd, b->seq_no,
			b->msg_type, b->rcv);
#endif
}
#endif

void tarp_init (void) {

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

// ============================================================================

#if TARP_COMPRESS

// I have tried to use an internal function (declared within cmp_discard) for
// the qualifier (at least for C compilation), which would've eliminated the
// need for a static variable, but it crashes gcc ;-(

#if 1

// Variant with an extra static variable + regular function

static headerType *cmp_msb;

static int cmp_qual (address pkt) {
//
// Packet qualifier for path compression
//
	headerType *h = (headerType*)(pkt + 1);

	return (h->snd == cmp_msb->snd && h->seq_no == cmp_msb->seq_no);
}

#endif

static void cmp_discard (headerType *p) {

#if 0
	// Variant with an internal function (crashes the compiler [gcc])
	int cmp_qual (address pkt) {
		headerType *h = (headerType*)(pkt + 1);
		return (h->snd == p->snd && h->seq_no == p->seq_no);
	};
#else
	cmp_msb = p;
#endif

#if 1
	// No trace (diag)
	net_opt (PHYSOPT_REVOKE, (address) cmp_qual);
#else
	// Trace (diag)
	{
		sint s = net_opt (PHYSOPT_REVOKE, (address) cmp_qual);
		if (s) {
			diag ("COMPRESSED: %d", s);
		}
	}
#endif
}

#endif	/* TARP_COMPRESS */

// ============================================================================

static Boolean dd_fresh (headerType * buffer) {

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
			spdCache->m_hop = (word)(buffer->hoc) << 8;
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
static void upd_spd (headerType * msg) {

	word i;
	if (msg->snd == master_host) {
		// clears retries or empty write:
		spdCache->m_hop = (word)(msg->hoc) << 8;
		return;
	}
	if ((i = findInSpd (msg->snd)) < spdCacheSize) {
		spdCache->en[i].hop = (word)(msg->hoc) << 8;
		return;
	}
	spdCache->en[spdCache->head].host = msg->snd;
	spdCache->en[spdCache->head].hop = (word)(msg->hoc) << 8;
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
static int check_spd (headerType * msg) {

	int i, j;

	if (msg->rcv == master_host) {
		// hco should not be 0 any more... keep it until we can
		// test things properly
		i = (msg->hco > 0 ? msg->hco : tarp_maxHops) -
			msg->hoc -
			(spdCache->m_hop >>8) +
			((spdCache->m_hop & 0x00FF) >> tarp_rte_rec) +
			tarp_slack;
		if (i < 0 && tarp_rte_rec != 0)
			spdCache->m_hop++;

		dbug_rx ("%x %u spdm %d %u %u %u %u", 
			msg->msg_type, msg->snd, i,
			msg->hco, msg->hoc, spdCache->m_hop, msg->seq_no);
#if TARP_COMPRESS
		if (i > -TARP_COMPRESS)
			// TARP_COMPRESS == 1 -> i >= 0 (optimal path),
			// TARP_COMPRESS  > 1 -> admitted suboptimal path,
			// probably a bad idea
			msg->msg_type |= TARP_OPTPATH_FLAG;
#endif
		return i;
	}

	if ((i = findInSpd(msg->rcv)) >= spdCacheSize) {

		dbug_rx ("%x %u spdno %d %u %u %u", msg->msg_type, msg->snd,
			msg->hco - msg->hoc + tarp_slack -1,
			msg->hco, msg->hoc, msg->seq_no);

		return msg->hco - msg->hoc + tarp_slack -1;
	}

	j = (msg->hco > 0 ? msg->hco : tarp_maxHops) -
		msg->hoc -
		(spdCache->en[i].hop >>8) +
		((spdCache->en[i].hop & 0x00FF) >> tarp_rte_rec) +
		tarp_slack;
	if (j < 0 && tarp_rte_rec != 0)
		spdCache->en[i].hop++; // failed routing attempts ++

	dbug_rx ("%x %u spd %d %u %u %u %u", 
			msg->msg_type, msg->snd, j, msg->hco,
	  		msg->hoc, spdCache->en[i].hop, msg->seq_no);

	return j;
}

static void do_rx_dup (address buffer, int length, int off, int ses) {
//
// Duplicate the packet and send it out, if off is nonzero, assume the message
// is a trace, so use off to insert host_id % 256 and RSSI
//
	headerType *msgBuf = (headerType*)(buffer + 1);
	address dup;
	int nlen;

	// Minimum physical length of the copy, need room for two bytes at
	// offset + packet trailer (offset is absolute, i.e., from the
	// beginning of physical packet)
	if ((nlen = off + NET_TRAILER_LENGTH + 2) > NET_MAXPLEN) {
		// Ignore
		dbug_rx ("%x %u cpy ovf %u", msgBuf->msg_type, msgBuf->snd,
			msgBuf->seq_no);
		return;
	}

	// If off is zero, nlen is definitely smaller than length, so length
	// will be used

	if (nlen < length)
		// Never decrease
		nlen = length;

	if ((dup = tcvp_new (nlen, TCV_DSP_XMT, ses)) == NULL) {
		dbg_8 (0x3000); // Tarp dup2 failed
		diag ("Tarp dup2 failed");
		// Do nothing
		return;
	}

	memcpy ((char*)dup, (char*)buffer, length);

	if (off) {
		*((byte*)dup + off    ) = (byte)local_host;
		*((byte*)dup + off + 1) = net_get_rssi (buffer, length);
	}

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
	dup [(nlen >> 1) -1] = tarp_pxopts;
#endif
	dbug_rx ("%x %u bcast cpy %u", msgBuf->msg_type, msgBuf->snd,
		msgBuf->seq_no);
}

// August 2015 pxopts inserts deserve more tests both with and without RADIO_OPTION_PXOPTIONS
int tarp_rx (address buffer, int length, int *ses) {

	headerType * msgBuf = (headerType *)(buffer+1);
	address dup;

#if TARP_RTR
	word i;
#endif
	if (length < sizeof(headerType) + NET_FRAME_LENGTH) {
		diag ("tarp rcv bad length %d", length);
		return TCV_DSP_DROP;
	}
	
	if (tarp_mType (msgBuf->msg_type) == 0) { // dummy ack from dst
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
			if (!tcvp_isdetached (rtrCache->pkt [i])) {
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
	if (guide_rtr (msgBuf) == 2 && 
		(i = findInRtr (msgBuf->snd, msgBuf->seq_no, NULL)) <
			rtrCacheSize) {
		if (msgBuf->hoc < rtrCache->hoc[i]) {
			// should we send back a dummy? Likely not, as our
			// retry will have a chance to clear the upstream...
			dbug_rtr ("%x-%u.%u resisted %u (%u) at %u",
					rtrCache->pkt[i], msgBuf->snd, 
					msgBuf->seq_no, msgBuf->hoc,
					rtrCache->hoc[i], i);
		} else {
			dbug_rtr ("%x-%u.%u unhooked-ack at %u %u (%x %u %u)",
                                        rtrCache->pkt[i], 
                                        msgBuf->snd, msgBuf->seq_no,
                                        i, rtrCache->fecnt +1,
					tcvp_gethook (rtrCache->pkt [i]),
					tcvp_isqueued (rtrCache->pkt [i]),
					tcvp_issettimer (rtrCache->pkt [i])
			);
			
// PG =========================================================================
			if (!tcvp_isdetached (rtrCache->pkt [i])) {
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
	}
#endif

	tarp_ctrl.ssignal = (net_get_rssi (buffer, length) >=
		tarp_ctrl.rssi_th) ?  YES : NO;

	dbug_lte ("%x %u %u rcv %u", msgBuf->msg_type, msgBuf->snd,
			msgBuf->seq_no, (word)seconds());

	dbug_rx ("%x %u ssig %u drop %u", msgBuf->msg_type, msgBuf->snd,
		tarp_ctrl.ssignal, tarp_drop_weak);

	if (!tarp_ctrl.ssignal) {
		dbg_8 (0x0600 | msgBuf->hoc + 1);
		if (tarp_drop_weak) {
			return TCV_DSP_DROP;
		}
	}

	tarp_ctrl.rcv++;

	if (*buffer == 0)  { // from unbound node

		dbug_rx ("%x %u from unbound %s", msgBuf->msg_type, msgBuf->snd,
			net_id == 0 || !msg_isNew(msgBuf->msg_type) ?  
				"drop" : "rcv");
		return net_id == 0 || !msg_isNew(msgBuf->msg_type) ?
			TCV_DSP_DROP : TCV_DSP_RCV;
	}

	if (msgBuf->snd == local_host) {
		// my own echo -- drop it
		dbug_rx ("%x %u drop echo", msgBuf->msg_type, msgBuf->snd);
		return TCV_DSP_DROP;
	}

	if (net_id == 0 && !msg_isBind (msgBuf->msg_type)) {

		dbug_rx ("%x %u drop no net_id", msgBuf->msg_type, msgBuf->snd);
		return TCV_DSP_DROP;
	}

	msgBuf->hoc++;

	if (msgBuf->weak) {
		dbg_8 (0x0700 | msgBuf->hoc);
		tarp_ctrl.ssignal = NO;
	} else {
		if (!tarp_ctrl.ssignal)
			msgBuf->weak = YES;
	}

#if TARP_RTR
	// ack duplicates as well - otherwise the 1st lost ack
	// invalidates retries
	if (msgBuf->rcv == local_host)
		ackForRtr (msgBuf, ses);
#endif

#if TARP_COMPRESS
	if (msgBuf->msg_type & TARP_OPTPATH_FLAG) {
		// Check transmit queue
		cmp_discard (msgBuf);
		// Clear the flag, so it is not forwarded
		msgBuf->msg_type ^= TARP_OPTPATH_FLAG;
	}
#endif
	if (tarp_level && !dd_fresh(msgBuf)) {  // check and update DD

		dbug_rx ("%x %u drop dup %u", msgBuf->msg_type, msgBuf->snd,
			msgBuf->seq_no);
		return TCV_DSP_DROP;    //duplicate
	}

	if (tarp_level > 1 && tarp_ctrl.ssignal)
		upd_spd (msgBuf);

	if (msgBuf->rcv == local_host) {
		dbug_rx ("%x %u rcv is me %u", msgBuf->msg_type, msgBuf->snd, 
			msgBuf->seq_no);
		return TCV_DSP_RCV;
	}

	if (msgBuf->rcv == 0) {
#if TARP_RTR
		// if we ever want bcast retried... for now, this is redundant
		ackForRtr (msgBuf, ses);
#endif
		if (tarp_fwd_on && !msgBuf->prox &&
				msgBuf->hoc < msgBuf->hco) {
			// Duplicate the packet, send the copy,
			// receive the original
			do_rx_dup (buffer, length,
				msg_isTrace (msgBuf->msg_type) ?
					tr_offset (msgBuf) : 0, *ses);
			tarp_ctrl.fwd++;
		} else {
			// Just receive
			dbug_rx ("%x %u bcast just rcv %u", msgBuf->msg_type, 
				msgBuf->snd, msgBuf->seq_no);
		}

		return TCV_DSP_RCV; // the original
	}

	if (msgBuf->hoc >= tarp_maxHops) {

		dbug_rx ("%x %u Max drop %d", msgBuf->msg_type, msgBuf->snd, 
			msgBuf->seq_no);
		return TCV_DSP_DROP;
	}

	if (tarp_fwd_on && !msgBuf->prox &&
		(tarp_level < 2 || check_spd (msgBuf) >= 0)) {

		tarp_ctrl.fwd++;
#if TARP_RTR
		// PG: Why? Doesn't normal FWD fulfill this goal? Oh, I see,
		// this is stronger, because the ACK is unconditional on the
		// number of hops.
		ackForRtr (msgBuf, ses);
#endif
		if (!msg_isTrace (msgBuf->msg_type)) {

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
			buffer[(length >>1) -1] = tarp_pxopts;
#endif
			dbug_rx ("%x %u xmit %u", msgBuf->msg_type, 
					msgBuf->snd, msgBuf->seq_no);
			return TCV_DSP_XMT;
		}

		// Trace
		do_rx_dup (buffer, length, tr_offset (msgBuf), *ses);
		// Forwarded a copy, drop the original
		return TCV_DSP_DROP;
		// PG: in Wlodek's version, the original was also DSP_XMT. I
		// think it was wrong, because the packet was retransmitted
		// with the same length and hoc++. That caused packet overflow
		// on next hop. Netting would crash on trace messages over a
		// notrivial number of hops.
	}

	dbug_rx ("%x %u (&) drop %u", msgBuf->msg_type, msgBuf->snd, 
		msgBuf->seq_no);
	return TCV_DSP_DROP;
}

static void setHco (headerType * msg) {

	word i;

	if (msg->hco != 0) // application decided
		return;
	if (tarp_level < 2 || msg->rcv == 0) {
		msg->hco = tarp_maxHops;
		return;
	}
	if (msg->rcv == master_host) {
		msg->hco = (spdCache->m_hop>>8) + tarp_ctrl.pp_widen;
		return;
	}

	if ((i = findInSpd(msg->rcv)) < spdCacheSize)
		msg->hco = (spdCache->en[i].hop >>8) +
			tarp_ctrl.pp_widen;

	else
		msg->hco = tarp_maxHops;
}

int tarp_tx (address buffer) {

	headerType * msgBuf = (headerType *)(buffer+1);
	int rc = (tarp_ctrl.pp_urg ? TCV_DSP_XMTU : TCV_DSP_XMT);

	tarp_ctrl.snd++;
	msgBuf->hoc = 0;
	msgBuf->weak = 0;
	setHco(msgBuf);
	if (++tarp_cyclingSeq & 0xFF00)
		tarp_cyclingSeq = 1;
	msgBuf->seq_no = tarp_cyclingSeq;
	msgBuf->snd = local_host;
	// clear per packet exceptions every time tarp_tx is called
	tarp_ctrl.pp_urg = 0;
	tarp_ctrl.pp_widen = 0;

	dbug_lte ("%x %u %u snd %u", msgBuf->msg_type, msgBuf->rcv,
		msgBuf->seq_no, (word)seconds());

	dbug_rx ("%x %u tx %u %u %u", msgBuf->msg_type, msgBuf->snd,
			msgBuf->rcv, msgBuf->hco, msgBuf->seq_no);

	return rc;
}

#if TARP_RTR
int tarp_xmt (address buffer) {

        headerType * msgBuf = (headerType *)(buffer+1);
	word i;

	// msg_null - ack at dst || irrelevant
	if (tarp_mType (msgBuf->msg_type) == 0 || guide_rtr (msgBuf) < 2)
		return TCV_DSP_DROP;


	if ((i = findInRtr (0, 0, buffer)) < rtrCacheSize) {

		if (++rtrCache->rcnt[i] > TARP_RTR) {
			rtrCache->fecnt++;
			dbug_rtr ("%x-%u.%u t%x r%u unhooked-rtr at %u %u",
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
			rtrCache->hoc[i] = in_header(buffer +1, hoc);
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

#undef _TARP_T_RX
#undef _TARP_T_LIGHT
#undef _TARP_T_RTR
