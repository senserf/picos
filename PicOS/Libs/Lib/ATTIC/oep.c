#include "sysio.h"
#include "tcvphys.h"
#include "tcvplug.h"
#include "oep.h"

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// Object exchange protocol
//

#define	OEP_MAXTRIES	8
#define	OEP_LONGTRIES	12
#define	OEP_REPDELAY	1024
#define	OEP_NSTOPS	3
#define	OEP_STOPDELAY	24
#define	OEP_CHKDELAY	1

// FIXME: make (some of) them settable?

#define	PT_GO		0x11
#define	PT_CHUNK	0x12


// ============================================================================

typedef struct {
//
// List of chunks to send (as requested by the recipient)
//
	word	list [OEP_MAXCHUNKS];
	word	p, n, nc;

} croster_t;

// ============================================================================

typedef struct {
//
// The tally of received chunks
//
	word	n,		// Total number of chunks
		f;		// First missing
	byte	cstat [0];	// Bitmap of chunks

} ctally_t;

// Calculates cstat size given the number of chunks
#define	ctally_ctsize(nc)	(((nc)+7) >> 3)

// Checks whether a given chunk number is present
#define	ctally_present(ct,cn)	((ct)->cstat [(cn)>>3] & (1 << ((cn)&7)))
#define	ctally_missing(ct,cn)	(!ctally_present (ct, cn))

// Update the first missing pointer
#define ctally_update(ct)	do { while ((ct)->f < (ct)->n && \
					ctally_present (ct, (ct)->f)) \
						(ct)->f++; } while (0)

// ============================================================================

typedef struct {
//
// Receiver structure
//
	word oli;	// Old network ID, must be the first item
	word scr;
	oep_chf_t fun;
	ctally_t cta;

} rcv_data_t;

#define	RD_cta	(RDATA->cta)
#define	RD_scr	(RDATA->scr)
#define RD_fun	(RDATA->fun)

// ============================================================================

typedef struct {
//
// Sender structure
//
	word oli;	// Old network ID, must be the first item
	croster_t ros;
	word scr, ylid, nch;
	oep_chf_t fun;
	byte yrqn;

} snd_data_t;

#define	SD_ros	(SDATA->ros)
#define	SD_scr	(SDATA->scr)
#define	SD_ylid	(SDATA->ylid)
#define	SD_yrqn	(SDATA->yrqn)
#define	SD_fun	(SDATA->fun)
#define	SD_nch	(SDATA->nch)

#define	OLD_SID	(SDATA->oli)

// ============================================================================

word  OEP_MLID;				// My Link ID

static void *PDATA = NULL;		// Internal data hook for the protocol

#define	SDATA	((snd_data_t*)PDATA)
#define RDATA	((rcv_data_t*)PDATA)

static address	packet;

#define	PF_RLID	(packet [0])		// Requestor ID
#define	PF_CMD	(((byte*)packet)[2])	// Command code
#define	PF_RQN	(((byte*)packet)[3])	// Request number
#define	PF_PAY	(packet+2)		// Payload pointer
#define	PF_CKNO (packet [2])		// Chunk number (CHUNK)

static int	SID = -1;		// Session ID
static byte	Retries, PLUG;
static byte	SDesc [TCV_MAX_PHYS];

byte		OEP_Status = OEP_STATUS_NOINIT,
		OEP_RQN = 1,
		OEP_PHY = 0,
		OEP_LastOp = 0;		// OEP_STATUS_SND/OEP_STATUS_RCV

// ============================================================================
// The plugin =================================================================
// ============================================================================

static int tcv_ope_oep (int, int, va_list);
static int tcv_clo_oep (int, int);
static int tcv_rcv_oep (int, address, int, int*, tcvadp_t*);
static int tcv_frm_oep (address, int, tcvadp_t*);
static int tcv_out_oep (address);
static int tcv_xmt_oep (address);

const tcvplug_t plug_oep =
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
 * down, it will drain the queue and set SID to -1
 */
	if (SID < 0)
		return TCV_DSP_PASS;

	*ses = SID;
	bounds->head = bounds->tail = 0;

	return TCV_DSP_RCV;
}

static int tcv_frm_oep (address p, int phy, tcvadp_t *bounds) {

	bounds->head = 0;
	bounds->tail = 2;	// We don't care about CRC
	return 2;
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

void ctally_init (ctally_t *ct, word nc) {
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

word ctally_fill (ctally_t *ct, address p) {
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

Boolean ctally_add (ctally_t *ct, word cn) {

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

Boolean ctally_full (ctally_t *ct) {

	ctally_update (ct);

	return (ct->f >= ct->n);
}

word ctally_absent (ctally_t *ct) {
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

word croster_init (croster_t *cr, address payl, word ne) {
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

word croster_next (croster_t *cr) {
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

procname (oep_handler);
static word swaplid (word);

static void quit () {

	if (PDATA != NULL) {
		// Do this first as OLD_SID is in PDATA!
		swaplid (OLD_SID);
		// Deallocate our data
		ufree (PDATA);
		PDATA = NULL;
	}

	if (SID >= 0) {
		while ((packet = tcv_rnp (WNONE, SID)) != NULL)
			// Get rid of all queued packets we have claimed
			tcv_endp (packet);
		// Switch the plugin off
		SID = -1;
	}

	trigger (OEP_EVENT);

	// This must be the last statement, as there is no return
	killall (oep_handler);
}

static void oepr_outp (word rep, byte cmd, word len) {
//
// Create an outgoing packet for RCV
//
	if ((packet = tcv_wnp (rep, SID, OEP_PKHDRLEN + len)) != NULL) {
		PF_RLID = OEP_MLID;
		PF_RQN = OEP_RQN;
		PF_CMD = cmd;
	}
}

static void oepx_outp (word rep, byte cmd, word len) {
//
// Create an outgoing packet for SEND
//
	if ((packet = tcv_wnp (rep, SID, OEP_PKHDRLEN + len)) != NULL) {
		PF_RLID = SD_ylid;
		PF_RQN = SD_yrqn;
		PF_CMD = cmd;
	}
}

thread (oep_handler)

#define	OEP_START		0
#define	OEP_RCV_SGO		1
#define	OEP_RCV_SGO_LP		2
#define	OEP_RCV_SGO_SN		3
#define	OEP_RCV_SGO_WR		4
#define	OEP_RCV_SCK		5
#define	OEP_RCV_CHK_MC		6
#define	OEP_RCV_END		7
#define	OEP_RCV_END_LP		8
#define	OEP_SEND_CHK_LP		9
#define	OEP_SEND_CHK_AP		10
#define	OEP_SEND_CHK_SN		11
#define	OEP_SEND_EOR_LP		12
#define	OEP_SEND_EOR_EC		13
#define	OEP_SEND_EOR_WG		14

    entry (OEP_START)

	if (OEP_Status == OEP_STATUS_SND) {
		Retries = OEP_LONGTRIES - 1;
		proceed (OEP_SEND_EOR_EC);
	}

	if (OEP_Status != OEP_STATUS_RCV)
		// A precaution
		quit ();
		
// ============================================================================
// ============================================================================

    entry (OEP_RCV_SGO)		// Prepare a GO request =======================

	// GO packet length
	if ((RD_scr = ctally_fill (&RD_cta, NULL)) == 0)
		// No more chunks expected
		proceed (OEP_RCV_END);

	Retries = OEP_MAXTRIES;

    entry (OEP_RCV_SGO_LP)	// GO send loop ===============================

	if (Retries == 0)
		// Chunk reception timeout
		goto Tmout;

	Retries--;

    entry (OEP_RCV_SGO_SN)	// Send one GO packet =========================

	oepr_outp (OEP_RCV_SGO_SN, PT_GO, RD_scr);
	ctally_fill (&RD_cta, PF_PAY);
	tcv_endp (packet);

    entry (OEP_RCV_SGO_WR)	// Wait for a reply to GO =====================

	delay (OEP_REPDELAY, OEP_RCV_SGO_LP);
	packet = tcv_rnp (OEP_RCV_SGO_WR, SID);
	if (PF_RLID != OEP_MLID && PF_RQN != OEP_RQN && PF_CMD != PT_CHUNK) {
		tcv_endp (packet);
		proceed (OEP_RCV_SGO_WR);
	}
RChunk:
	// A chunk packet arriving
	if ((RD_scr = tcv_left (packet) - OEP_PKHDRLEN) != OEP_PAYLEN) {
		// No chunk content
		tcv_endp (packet);
		if (RD_scr != 0)
			// Packet screwup, but assume it was a chunk, so
			// expect more
			proceed (OEP_RCV_CHK_MC);
		// Empty chunk, end of round
		proceed (OEP_RCV_SGO);
	}

	if (ctally_add (&RD_cta, PF_CKNO))
		// The chunk has already been received, ignore it
		goto IgnoreChunk;

    entry (OEP_RCV_SCK)		// Store the chunk ============================

	RD_fun (OEP_RCV_SCK, PF_PAY+1, PF_CKNO);

IgnoreChunk:
	tcv_endp (packet);

    entry (OEP_RCV_CHK_MC)	// Wait for more chunks in the round ==========

	// Timeout == end of round
	delay (OEP_REPDELAY, OEP_RCV_SGO);
	packet = tcv_rnp (OEP_RCV_CHK_MC, SID);
	if (PF_RLID != OEP_MLID && PF_RQN != OEP_RQN && PF_CMD != PT_CHUNK) {
		tcv_endp (packet);
		proceed (OEP_RCV_CHK_MC);
	}
	goto RChunk;

    entry (OEP_RCV_END)		// All chunks received ========================

	Retries = OEP_NSTOPS;

    entry (OEP_RCV_END_LP)	// Send stop packets ==========================

	oepr_outp (OEP_RCV_END_LP, PT_GO, 0);
	tcv_endp (packet);

	if (--Retries) {
		// More
		delay (OEP_STOPDELAY, OEP_RCV_END_LP);
		release;
	}

	// Done
	OEP_Status = OEP_STATUS_DONE;
	quit ();

// ============================================================================
// ============================================================================

    entry (OEP_SEND_CHK_LP)	// Find next chunk to send ====================

	// This is not idempotent, as it advances the roster to the next chunk,
	// so we need a special state for it

	// The number of next outgoing chunk
	if ((SD_scr = croster_next (&SD_ros)) == WNONE) {
		// No more, send an empty chunk
		Retries = OEP_MAXTRIES - 1;
		proceed (OEP_SEND_EOR_EC);
	}

    entry (OEP_SEND_CHK_AP)	// Acquire an output packet ===================

	oepx_outp (OEP_SEND_CHK_AP, PT_CHUNK, OEP_CHUNKSIZE);
	PF_CKNO = SD_scr;

    entry (OEP_SEND_CHK_SN)	// Load and send the chunk ====================

	SD_fun (OEP_SEND_CHK_SN, PF_PAY+1, SD_scr);
	tcv_endp (packet);

	delay (OEP_CHKDELAY, OEP_SEND_CHK_LP);
	release;

    entry (OEP_SEND_EOR_LP)	// End of round loop ==========================

	if (Retries == 0) {
		// Done or broken, we shall never know
Tmout:
		OEP_Status = OEP_STATUS_TMOUT;
		quit ();
	}

	Retries--;

    entry (OEP_SEND_EOR_EC)	// Send empty chunk ===========================

	oepx_outp (OEP_SEND_EOR_EC, PT_CHUNK, 0);
	tcv_endp (packet);

    entry (OEP_SEND_EOR_WG)	// Wait for a GO packet =======================

	delay (OEP_REPDELAY, OEP_SEND_EOR_LP);
	packet = tcv_rnp (OEP_SEND_EOR_WG, SID);
	if (PF_RLID != SD_ylid || PF_RQN != SD_yrqn || PF_CMD != PT_GO) {
		// Ignore
IgnoreRost:
		tcv_endp (packet);
		proceed (OEP_SEND_EOR_WG);
	}
	if ((SD_scr = tcv_left (packet)) < OEP_PKHDRLEN)
		// Packet screwup
		goto IgnoreRost;

	if ((SD_scr -= OEP_PKHDRLEN) < 2) {
		// Empty GO
		tcv_endp (packet);
		OEP_Status = OEP_STATUS_DONE;
		quit ();
	}
	// Unpack the chunk list
	if (croster_init (&SD_ros, PF_PAY, SD_scr))
		// Error in the chunk list, ignore the packet
		goto IgnoreRost;

	tcv_endp (packet);
	// And start the next round
	proceed (OEP_SEND_CHK_LP);

endthread

// ============================================================================

Boolean oep_init () {
//
// Initialize
//
	if (SID >= 0)
		// Something in progress
		return NO;

	if (OEP_Status == OEP_STATUS_NOINIT) {
		// First time
		for (PLUG = 0; PLUG < TCV_MAX_PHYS; PLUG++)
			SDesc [PLUG] = BNONE;
		// Install the plugin in the first free location
		for (PLUG = 0; PLUG < TCV_MAX_PLUGS; PLUG++) {
			if (tcv_plug ((int)PLUG, &plug_oep) == 0) {
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

	if (SDesc [OEP_PHY] == BNONE) {
		// Open a session for this phy
		if ((sid = tcv_open (WNONE, OEP_PHY, (int) PLUG)) == ERROR)
			syserror (ERESOURCE, "oep_setsess");
		SDesc [OEP_PHY] = (byte) sid;
	}

	if (runthread (oep_handler) == 0)
		return OEP_STATUS_NOPROC;

	SID = (int) SDesc [OEP_PHY];
	return 0;
}

static word swaplid (word lid) {
//
// Swaps the network ID
//
	word oli;

	oli = tcv_control (SID, PHYSOPT_GETSID, NULL);
	tcv_control (SID, PHYSOPT_SETSID, &lid);
	return oli;
}

byte oep_rcv (word nch, oep_chf_t fun) {
//
// Sender
//
	byte st;

	if (OEP_Status >= 64)
		return OEP_Status;

	sysassert (SID < 0, "oep_rcv");

	if ((st = setsess ()) != 0) {
Quit:
		quit ();
		return st;
	}

	// Create the chunk tally
	if ((PDATA = (void*) umalloc (sizeof (rcv_data_t) +
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

	sysassert (SID < 0, "oep_snd");

	if ((st = setsess ()) != 0) {
Quit:
		quit ();
		return st;
	}

	if ((PDATA = (void*) umalloc (sizeof (snd_data_t))) == NULL) {
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
