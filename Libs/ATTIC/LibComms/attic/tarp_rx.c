/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "tarp.h"
#include "msg_tarp.h"
#include "diag.h"

extern bool tarp_isProxy (word);

// one stop to maintain the DD cache and remove duplicates (not fresh)
static bool fresh (headerType * buffer) {
	word i;

	for (i = 0; i < ddCacheSize; ++i) {
		if (ddCache[i].snd == buffer->snd) {
			if (ddCache[i].seq == buffer->seq_no &&
				seconds() < ddCache[i].timeStamp + tarp_freshTout) { // duplicate

				net_diag (D_DEBUG, "Drop dup (%lu %u)",
					buffer->snd, buffer->seq_no);

				ddCache[i].timeStamp = seconds(); // update timestamp
				return false;
			}

			// update seq and time
			net_diag (D_DEBUG, 
				"DD upd(%lu): seq(%d->%d) time(%lu->%lu)",
				ddCache[i].snd, ddCache[i].seq,
				buffer->seq_no, ddCache[i].timeStamp,
				seconds());

			ddCache[i].seq = buffer->seq_no;
			ddCache[i].timeStamp = seconds();
			return true; // fresh seq
		}
		if (ddCache[i].snd == 0) { // fresh sender

			net_diag (D_DEBUG,
				"DD ins: snd(%lu) seq(%u) time(%lu)",
				buffer->snd, buffer->seq_no, seconds());

			ddCache[i].snd = buffer->snd;
			ddCache[i].seq = buffer->seq_no;
			ddCache[i].timeStamp = seconds();
			return true;
		}
	}
	net_diag (D_SERIOUS, "fresh: ??");
	return true;
}

static void dumpSpdEntry (word i) {
	net_diag (D_INFO, " id(%lu) hco(%u) timeStamp(%lu)",
		spdCache->repos[i].id,
		spdCache->repos[i].hco,
		spdCache->repos[i].timeStamp);
}


static void dumpSpdCache () {
	int i;
	
	net_diag (D_INFO, "SPD Cache: last(%u) free(%u)",
		spdCache->last, spdCache->free);
	
	for (i = 0; i < spdCacheSize; i++) {
		if (spdCache->repos[i].id != 0)
			dumpSpdEntry(i);
	}
}

static void dumpMsg (headerType * buffer) {

	net_diag (D_INFO, "Msg(%x): seq(%u) snd(%lu) rcv(%lu) hoc(%u) hco(%u)", 
		buffer->msg_type, buffer->seq_no, buffer->snd,
		buffer->rcv, buffer->hoc, buffer->hco);

}

static void insertToSpdCache (lword who, word what) {

	spdCache->repos[spdCache->free].id = who;
	spdCache->repos[spdCache->free].hco = what;
	spdCache->repos[spdCache->free].timeStamp = seconds();
	spdCache->last = spdCache->free++;

	while (spdCache->repos[spdCache->free].id != 0 && 
		spdCache->free < spdCacheSize) {
		spdCache->free++;
	}

	if (spdCache->free >= spdCacheSize)
		net_diag (D_SERIOUS, "Serious: SPD full");
}

static void updSpdCache (lword who, word what) {
	if (tarp_findInSpd (who)) {
	// longer route or a break since the last one
		if (what < spdCache->repos[spdCache->last].hco ||
			seconds() > spdCache->repos[spdCache->last].timeStamp + tarp_freshTout) {
			spdCache->repos[spdCache->last].hco = what;
			// I'm not sure if the time update should be conditional
			spdCache->repos[spdCache->last].timeStamp = seconds();
		}
	} else {
		insertToSpdCache(who, what);
	}
	if (net_dl >= D_DEBUG)
		dumpSpdCache();
}

static bool checkSpdCache (headerType * msg) {
	// can't optimize bcast nor msg to myself
	if (msg->rcv == 0 || msg->rcv == local_host)
		return true;

	if (tarp_findInSpd(msg->rcv)) {

		net_diag (D_DEBUG, "Spd hit: %u ? %u + %u",
				msg->hco, msg->hoc,
				spdCache->repos[spdCache->last].hco);

		if (msg->hco == 0) { // update msg->hco

			net_diag (D_DEBUG, "Msg hco update (%lu)", msg->rcv);

			msg->hco =
				spdCache->repos[spdCache->last].hco + msg->hoc;
			return true;
		}

		return spdCache->repos[spdCache->last].hco + msg->hoc <= 
			msg->hco;
	}

	net_diag (D_INFO, "No spd  (%lu)", msg->rcv);

	return true;

}

int tarp_rx (address buffer, int length) {

	bool	optimal	= true;
	bool	schizo	= false;
	headerType * msgBuf = (headerType *)(buffer+1);

	tarp_count.rcv++;
	
	if (length == 0) { // nothing in the recv buffer... can it happen?
		net_diag (D_WARNING, "Zero length");
		return TCV_DSP_DROP;
	}
	if (tarp_isProxy (msgBuf->msg_type)) {
		net_diag (D_DEBUG, "Proxy (%lu, %u)", msgBuf->snd,
				msgBuf->seq_no);
		return TCV_DSP_RCV;
	}
	// demo cheat
	/* that was for meter demo -- 1 and 3 could not talked directly
	if (msgBuf->hoc == 0 && local_host + msgBuf->snd == 4) {
		net_diag (D_DEBUG, "Drop direct");
		return TCV_DSP_DROP;
	}
	*/
	if (msgBuf->snd == local_host) { // my own -- drop?
		net_diag (D_DEBUG, "Echo (%u, %u)", msgBuf->seq_no,
			msgBuf->hoc);
		return TCV_DSP_DROP;
	}
	if (tarp_level && !fresh(msgBuf))	 // check and update DD
		return TCV_DSP_DROP;	//duplicate
 
	if (tarp_level > 1) {
		// first, update spdCache
		updSpdCache(msgBuf->snd, msgBuf->hoc);
 
		// SPD check
		optimal = checkSpdCache(msgBuf);
	}

	// do we want to forward?
	if (optimal && 
		msgBuf->rcv != local_host) { // not mine nor bcast
		if (++msgBuf->hoc >= tarp_maxHops) {
			net_diag (D_DEBUG, "Hoc maxed dropped:");
			dumpMsg(msgBuf);

		} else {
			net_diag (D_DEBUG, "Forwarding:");
			dumpMsg(msgBuf);
			schizo = true;
			tarp_count.fwd++;
		}
	}  // done with forwarding

	if (msgBuf->rcv == 0 || 
		msgBuf->rcv == local_host) { // mine or bcast
		return (schizo ? -1 : TCV_DSP_RCV); // -1 is both XMT and RCV
	}
	return (schizo ? TCV_DSP_XMT : TCV_DSP_DROP);
}
