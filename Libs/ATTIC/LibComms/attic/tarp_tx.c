/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "tarp.h"
#include "msg_tarp.h"
#include "diag.h"

extern bool tarp_isProxy (word);

bool    tarp_urgent             = 0;
word	tarp_retry		= 0;
static char tarp_cyclingSeq	= 0;

static void setHco (headerType * msg) {
	if (tarp_level < 2 || tarp_isProxy (msg->msg_type)) {
		msg->hco = 0;
		return;
	}

	if (tarp_findInSpd(msg->rcv)) {
		msg->hco = spdCache->repos[spdCache->last].hco;
		if (!(tarp_options & tarp_strictSpdOpt)) {
			if (tarp_retry) {
				msg->hco += tarp_retry;
				tarp_retry = 0;
			}
		}

		net_diag (D_DEBUG,  "Hco %u for %lu", msg->hco, msg->rcv);

	} else {
		msg->hco = 0;
	}
}

int tarp_tx (address buffer) {
	headerType * msgBuf = (headerType *)(buffer+1);
	int rc = (tarp_urgent ? TCV_DSP_XMTU : TCV_DSP_XMT);

	tarp_count.snd++;
	msgBuf->hoc = 0;
	setHco(msgBuf);
	msgBuf->seq_no = ++tarp_cyclingSeq;
	msgBuf->snd = local_host;

	// clear retry and urgent every time tarp_tx is called (explicit, exceptional events)
	tarp_retry = 0;
	tarp_urgent = 0;
	return rc;
}
