/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __oep_c__
#define	__oep_c__

#include "sysio.h"
#include "tcvphys.h"
#include "tcvplug.h"


//
// Object exchange protocol
//

#include "oep.h"
#include "oep_types.h"

#define	OEP_MAXTRIES	8
#define	OEP_LONGTRIES	12
#define	OEP_REPDELAY	1024
#define	OEP_NSTOPS	3
#define	OEP_STOPDELAY	24
#define	OEP_CHKDELAY	1

// FIXME: make (some of) them settable?

#define	PT_GO		0x11
#define	PT_CHUNK	0x12

fsm oep_handler;

// Calculates cstat size given the number of chunks
#define	ctally_ctsize(nc)	(((nc)+7) >> 3)

// Checks whether a given chunk number is present
#define	ctally_present(ct,cn)	((ct)->cstat [(cn)>>3] & (1 << ((cn)&7)))
#define	ctally_missing(ct,cn)	(!ctally_present (ct, cn))

// Update the first missing pointer
#define ctally_update(ct)	do { while ((ct)->f < (ct)->n && \
					ctally_present (ct, (ct)->f)) \
						(ct)->f++; } while (0)

#define	RD_cta	(RDATA->cta)
#define	RD_scr	(RDATA->scr)
#define RD_fun	(RDATA->fun)

#define	SD_ros	(SDATA->ros)
#define	SD_scr	(SDATA->scr)
#define	SD_ylid	(SDATA->ylid)
#define	SD_yrqn	(SDATA->yrqn)
#define	SD_fun	(SDATA->fun)
#define	SD_nch	(SDATA->nch)

#define	OLD_SID	(SDATA->oli)

byte 	OEP_Status 		= OEP_STATUS_NOINIT,
	OEP_LastOp		= 0,
	OEP_RQN			= 1,
	OEP_PHY			= 0;

word	OEP_MLID;

static void *oep_pdata		= NULL;

static address oep_pkt;

static int oep_sid = -1;

static byte	oep_retries,
		oep_plug;
static byte	oep_sdesc [TCV_MAX_PHYS];


#define	SDATA	((oep_snd_data_t*)oep_pdata)
#define RDATA	((oep_rcv_data_t*)oep_pdata)

#define	PF_RLID	(oep_pkt [0])		// Requestor ID
#define	PF_CMD	(((byte*)oep_pkt)[2])	// Command code
#define	PF_RQN	(((byte*)oep_pkt)[3])	// Request number
#define	PF_PAY	(oep_pkt+2)		// Payload pointer
#define	PF_CKNO (oep_pkt [2])		// Chunk number (CHUNK)

// ============================================================================
// The plugin =================================================================
// ============================================================================

static int tcv_ope_oep (int, int, va_list);
static int tcv_clo_oep (int, int);
static int tcv_rcv_oep (int, address, int, int*, tcvadp_t*);
static int tcv_frm_oep (address, tcvadp_t*);
static int tcv_out_oep (address);
static int tcv_xmt_oep (address);

trueconst tcvplug_t plug_oep =
		{ tcv_ope_oep, tcv_clo_oep, tcv_rcv_oep, tcv_frm_oep,
			tcv_out_oep, tcv_xmt_oep, NULL, 0x0085 /* Id */ };

static int tcv_ope_oep (int phy, int fd, va_list plid) {
/*
 * We are allowed to have one descriptor per PHY, and everything was
 * verified before we have been plugged in
 */
	return 0;
}

static int tcv_clo_oep (int phy, int fd) {
/*
 * We are never closed, this is void
 */
	return 0;
}

static int tcv_rcv_oep (int phy, address packet, int len, int *ses,
							tcvadp_t *bounds) {
/*
 * We claim all packets for as long as we are active; when the process goes
 * down, it will drain the queue and set oep_sid to -1
 */
	if (oep_sid < 0)
		return TCV_DSP_PASS;

	*ses = oep_sid;
	bounds->head = bounds->tail = 0;

	return TCV_DSP_RCV;
}

static int tcv_frm_oep (address p, tcvadp_t *bounds) {

	bounds->head = 0;
	bounds->tail = 2;	// We don't care about CRC
	return 0;
}

static int tcv_out_oep (address p) {

	return TCV_DSP_XMT;

}

static int tcv_xmt_oep (address p) {

	return TCV_DSP_DROP;
}

// ============================================================================
// Chunk tally functions ======================================================
// ============================================================================

static void ctally_init (oep_ctally_t *ct, word nc) {
//
// Initializes the chunk tally
//
	ct->n = nc;
	ct->f = 0;

	// The number of bytes
	nc = (nc + 7) >> 3;

	while (nc)
		ct->cstat [--nc] = 0;
}

static word ctally_fill (oep_ctally_t *ct, address p) {
//
// Fills the requested chunk list (if p != NULL), returns the length of the
// chunk list in bytes
//
	word cl, cn, cm;

	ctally_update (ct);

	if (ct->f >= ct->n)
		return 0;

	cn = ct->f;
	cl = 0;

	while (1) {
		// If we are here, cn is missing; check the next one
		if (cn+1 == ct->n || ctally_present (ct, cn+1)) {
			// No range
			if (p)
				*p++ = cn;
			cl++;
			if (cl == OEP_MAXCHUNKS)
				return OEP_MAXCHUNKS * 2;
			// Skip the current chunk
			cn++;
		} else {
			// A range
			if (cl == OEP_MAXCHUNKS-1) {
				// The last one to fill, range won't fit
				if (p)
					*p = cn;
				return OEP_MAXCHUNKS * 2;
			}
			// cn+1 is missing as well, keep scanning the range
			cm = cn + 2;
			while (cm < ct->n && ctally_missing (ct, cm))
				cm++;

			if (p) {
				*p++ = cm-1;
				*p++ = cn;
			}
			cl += 2;
			cn = cm;
		}
		// Locate next missing
		do {
			cn++;
			if (cn >= ct->n)
				return cl << 1;

		} while (ctally_present (ct, cn));
	}
}

static Boolean ctally_add (oep_ctally_t *ct, word cn) {

	word b;

	if (cn >= ct->n)
		// Sanity test: make it appear as present, so it won't
		// be added
		return YES;

	b = (1 << (cn & 7));
	cn >>= 3;
	if ((ct->cstat [cn] & b))
		return YES;
	ct->cstat [cn] |= (byte) b;
	return NO;
}

#if 0	/* For debugging ? */

static Boolean ctally_full (oep_ctally_t *ct) {

	ctally_update (ct);

	return (ct->f >= ct->n);
}

static word ctally_absent (oep_ctally_t *ct) {
//
// Count the missing chunks (for debugging)
//
	word wc, cn;

	for (cn = wc = 0; wc < ct->n; wc++)
		if (ctally_missing (ct, wc))
			cn++;

	return cn;
}

#endif	/* 0 */
	
// ============================================================================
// Chunk roster functions =====================================================
// ============================================================================

static word croster_init (oep_croster_t *cr, address payl, word ne) {
//
// Read the chunk list from the payload and initialize for processing
//
	word i;

	if (ne > OEP_MAXCHUNKS*2)
		// For sanity
		ne = OEP_MAXCHUNKS*2;

	memcpy ((byte*)(cr->list), (byte*)payl, ne);
	cr->n = (ne >> 1);
	cr->p = 0;

	// Sanity check
	for (i = 0; i < cr->n; i++)
		if (cr->list [i] >= cr->nc)
			return 1;
	return 0;
}

static word croster_next (oep_croster_t *cr) {
//
// Advance to next chunk
//
	word ci, cn;

	if (cr->p >= cr->n)
		// No more chunks
		return WNONE;

	if (cr->p == cr->n - 1 || cr->list [cr->p] < cr->list [cr->p+1]) {
		// This means: the last entry in the list or the next entry is
		// bigger than the current one, i.e., a single chunk indicated
		// by this entry
		return cr->list [(cr->p)++];
	}
	// A range: keep incrementing the second entry until it equals first

	cn = cr->list [ci = cr->p+1];
	if (cn == cr->list [cr->p])
		// The end of range
		cr->p += 2;
	else
		// Increment
		cr->list [ci] ++;
	return cn;
}

// ============================================================================

static word swaplid (word);

static void quit () {

	if (oep_pdata != NULL) {
		// Do this first as OLD_SID is in oep_pdata!
		swaplid (OLD_SID);
		// Deallocate our data
		ufree (oep_pdata);
		oep_pdata = NULL;
	}

	if (oep_sid >= 0) {
		while ((oep_pkt = tcv_rnp (WNONE, oep_sid)) != NULL)
			// Get rid of all queued packets we have claimed
			tcv_endp (oep_pkt);
		// Switch the plugin off
		oep_sid = -1;
	}

	trigger (OEP_EVENT);

	killall (oep_handler);
#ifdef __SMURPH__
	// Note: for VUEE, we require release
	release;
#endif
}

static void oepr_outp (word rep, byte cmd, word len) {
//
// Create an outgoing packet for RCV
//
	if ((oep_pkt = tcv_wnp (rep, oep_sid, OEP_PKHDRLEN + len)) != NULL) {
		PF_RLID = OEP_MLID;
		PF_RQN = OEP_RQN;
		PF_CMD = cmd;
	}
}

static void oepx_outp (word rep, byte cmd, word len) {
//
// Create an outgoing packet for SEND
//
	if ((oep_pkt = tcv_wnp (rep, oep_sid, OEP_PKHDRLEN + len)) != NULL) {
		PF_RLID = SD_ylid;
		PF_RQN = SD_yrqn;
		PF_CMD = cmd;
	}
}

fsm oep_handler {

    state OEP_START:

	if (OEP_Status == OEP_STATUS_SND) {
		oep_retries = OEP_LONGTRIES - 1;
		proceed OEP_SEND_EOR_EC;
	}

	if (OEP_Status != OEP_STATUS_RCV)
		// A precaution
		quit ();
		
// ============================================================================
// ============================================================================

    state OEP_RCV_SGO:		// Prepare a GO request =======================

	// GO packet length
	if ((RD_scr = ctally_fill (&RD_cta, NULL)) == 0)
		// No more chunks expected
		proceed OEP_RCV_END;

	oep_retries = OEP_MAXTRIES;

    state OEP_RCV_SGO_LP:	// GO send loop ===============================

	if (oep_retries == 0)
		// Chunk reception timeout
		goto Tmout;

	oep_retries--;

    state OEP_RCV_SGO_SN:	// Send one GO packet =========================

	oepr_outp (OEP_RCV_SGO_SN, PT_GO, RD_scr);
	ctally_fill (&RD_cta, PF_PAY);
	tcv_endp (oep_pkt);

    state OEP_RCV_SGO_WR:	// Wait for a reply to GO =====================

	delay (OEP_REPDELAY, OEP_RCV_SGO_LP);
	oep_pkt = tcv_rnp (OEP_RCV_SGO_WR, oep_sid);
	if (PF_RLID != OEP_MLID && PF_RQN != OEP_RQN && PF_CMD != PT_CHUNK) {
		tcv_endp (oep_pkt);
		proceed OEP_RCV_SGO_WR;
	}
RChunk:
	// A chunk packet arriving
	if ((RD_scr = tcv_left (oep_pkt) - OEP_PKHDRLEN) != OEP_PAYLEN) {
		// No chunk content
		tcv_endp (oep_pkt);
		if (RD_scr != 0)
			// Packet screwup, but assume it was a chunk, so
			// expect more
			proceed OEP_RCV_CHK_MC;
		// Empty chunk, end of round
		proceed OEP_RCV_SGO;
	}

	if (ctally_add (&RD_cta, PF_CKNO))
		// The chunk has already been received, ignore it
		goto IgnoreChunk;

    state OEP_RCV_SCK:		// Store the chunk ============================

	RD_fun (OEP_RCV_SCK, PF_PAY+1, PF_CKNO);

IgnoreChunk:
	tcv_endp (oep_pkt);

    state OEP_RCV_CHK_MC:	// Wait for more chunks in the round ==========

	// Timeout == end of round
	delay (OEP_REPDELAY, OEP_RCV_SGO);
	oep_pkt = tcv_rnp (OEP_RCV_CHK_MC, oep_sid);
	if (PF_RLID != OEP_MLID && PF_RQN != OEP_RQN && PF_CMD != PT_CHUNK) {
		tcv_endp (oep_pkt);
		proceed OEP_RCV_CHK_MC;
	}
	goto RChunk;

    state OEP_RCV_END:		// All chunks received ========================

	oep_retries = OEP_NSTOPS;

    state OEP_RCV_END_LP:	// Send stop packets ==========================

	oepr_outp (OEP_RCV_END_LP, PT_GO, 0);
	tcv_endp (oep_pkt);

	if (--oep_retries) {
		// More
		delay (OEP_STOPDELAY, OEP_RCV_END_LP);
		release;
	}

	// Done
	OEP_Status = OEP_STATUS_DONE;
	quit ();

// ============================================================================
// ============================================================================

    state OEP_SEND_CHK_LP:	// Find next chunk to send ====================

	// This is not idempotent, as it advances the roster to the next chunk,
	// so we need a special state for it

	// The number of next outgoing chunk
	if ((SD_scr = croster_next (&SD_ros)) == WNONE) {
		// No more, send an empty chunk
		oep_retries = OEP_MAXTRIES - 1;
		proceed OEP_SEND_EOR_EC;
	}

    state OEP_SEND_CHK_AP:	// Acquire an output packet ===================

	oepx_outp (OEP_SEND_CHK_AP, PT_CHUNK, OEP_CHUNKSIZE);
	PF_CKNO = SD_scr;

    state OEP_SEND_CHK_SN:	// Load and send the chunk ====================

	SD_fun (OEP_SEND_CHK_SN, PF_PAY+1, SD_scr);
	tcv_endp (oep_pkt);

	delay (OEP_CHKDELAY, OEP_SEND_CHK_LP);
	release;

    state OEP_SEND_EOR_LP:	// End of round loop ==========================

	if (oep_retries == 0) {
		// Done or broken, we shall never know
Tmout:
		OEP_Status = OEP_STATUS_TMOUT;
		quit ();
	}

	oep_retries--;

    state OEP_SEND_EOR_EC:	// Send empty chunk ===========================

	oepx_outp (OEP_SEND_EOR_EC, PT_CHUNK, 0);
	tcv_endp (oep_pkt);

    state OEP_SEND_EOR_WG:	// Wait for a GO packet =======================

	delay (OEP_REPDELAY, OEP_SEND_EOR_LP);
	oep_pkt = tcv_rnp (OEP_SEND_EOR_WG, oep_sid);
	if (PF_RLID != SD_ylid || PF_RQN != SD_yrqn || PF_CMD != PT_GO) {
		// Ignore
IgnoreRost:
		tcv_endp (oep_pkt);
		proceed OEP_SEND_EOR_WG;
	}
	if ((SD_scr = tcv_left (oep_pkt)) < OEP_PKHDRLEN)
		// Packet screwup
		goto IgnoreRost;

	if ((SD_scr -= OEP_PKHDRLEN) < 2) {
		// Empty GO
		tcv_endp (oep_pkt);
		OEP_Status = OEP_STATUS_DONE;
		quit ();
	}
	// Unpack the chunk list
	if (croster_init (&SD_ros, PF_PAY, SD_scr))
		// Error in the chunk list, ignore the packet
		goto IgnoreRost;

	tcv_endp (oep_pkt);
	// And start the next round
	proceed OEP_SEND_CHK_LP;
}

// ============================================================================

Boolean oep_init () {
//
// Initialize
//
	if (oep_sid >= 0)
		// Something in progress
		return NO;

	if (OEP_Status == OEP_STATUS_NOINIT) {
		// First time
		for (oep_plug = 0; oep_plug < TCV_MAX_PHYS; oep_plug++)
			oep_sdesc [oep_plug] = BNONE;
		// Install the plugin in the first free location
		for (oep_plug = 0; oep_plug < TCV_MAX_PLUGS; oep_plug++) {
			if (tcv_plug ((int)oep_plug, &plug_oep) == 0) {
				// Installed successfully
				OEP_Status = OEP_STATUS_DONE;
				goto IDone;
			}
		}
		// Failed
		return NO;
	}
IDone:
	return YES;
}

static byte setsess () {
//
// Set session Id for a transaction
//
	int sid;

	if (oep_sdesc [OEP_PHY] == BNONE) {
		// Open a session for this phy
		if ((sid = tcv_open (WNONE, OEP_PHY, (int) oep_plug)) == ERROR)
			syserror (ERESOURCE, "oep_setsess");
		oep_sdesc [OEP_PHY] = (byte) sid;
	}

	if (runfsm oep_handler == 0)
		return OEP_STATUS_NOPROC;

	oep_sid = (int) oep_sdesc [OEP_PHY];
	return 0;
}

static word swaplid (word lid) {
//
// Swaps the network ID
//
	word oli;

	oli = tcv_control (oep_sid, PHYSOPT_GETSID, NULL);
	tcv_control (oep_sid, PHYSOPT_SETSID, &lid);
	return oli;
}

byte oep_rcv (word nch, oep_chf_t fun) {
//
// Sender
//
	byte st;

	if (OEP_Status >= 64)
		return OEP_Status;

	sysassert (oep_sid < 0, "oep_rcv");

	if ((st = setsess ()) != 0) {
Quit:
		quit ();
		return st;
	}

	// Create the chunk tally
	if ((oep_pdata = (void*) umalloc (sizeof (oep_rcv_data_t) +
	    ctally_ctsize (nch))) == NULL) {
		st = OEP_STATUS_NOMEM;
		goto Quit;
	}
	OLD_SID = swaplid (0xFFFF);
	RD_fun = fun;
	ctally_init (&RD_cta, nch);
	OEP_LastOp = OEP_Status = OEP_STATUS_RCV;
	// Success
	return 0;
}

byte oep_snd (word ylid, byte yrqn, word nch, oep_chf_t fun) {
//
// Sender
//
	byte st;

	if (OEP_Status >= 64)
		return OEP_Status;

	sysassert (oep_sid < 0, "oep_snd");

	if ((st = setsess ()) != 0) {
Quit:
		quit ();
		return st;
	}

	if ((oep_pdata = (void*) umalloc (sizeof (oep_snd_data_t))) == NULL) {
		st = OEP_STATUS_NOMEM;
		goto Quit;
	}
	// This one is used for sanity checks
	SD_ros.nc = nch;
	OLD_SID = swaplid (0xFFFF);
	SD_ylid = ylid;
	SD_yrqn = yrqn;
	// Not sure if this is needed here (but we know it, and the caller
	// needs it, so let us keep it here just in case - one more sanity
	// check)
	SD_nch  = nch;
	SD_fun = fun;
	OEP_LastOp = OEP_Status = OEP_STATUS_SND;
	return 0;
}

byte oep_wait (word state) {

	if (oep_busy ()) {
		when (OEP_EVENT, state);
		release;
	}

	return OEP_Status;
}

#endif
