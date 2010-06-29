#ifndef __lcdg_images_c__
#define	__lcdg_images_c__

#include "sysio.h"
#include "lcdg_images.h"
#include "oep.h"
#include "storage.h"

#include "lcdg_images_types.h"

#define	LCDG_IM_PAGE_SHIFT	13		// 2^13 = 8192
#define	LCDG_IM_PAGE_MASK	((1<<LCDG_IM_PAGE_SHIFT)-1)

// Convert page number to page address
#define	lcdg_im_pntopa(a)	(((lword)(a)) << LCDG_IM_PAGE_SHIFT)

// Maximum offset of a chunk origin from beginning of page
#define	LCDG_IM_MAX_COFF	((LCDG_IM_CHUNKSPP-1) * LCDG_IM_CHUNKLEN)

// Number of pixels covered by one chunk
#define	LCDG_PIXPERCHK		((OEP_CHUNKLEN * 8) / 12)

// Convert the number of chunks to the number of pages; note that n is the
// number of chunks - 1
#define	lcdg_im_chtop(n)	(((n) + LCDG_IM_CHUNKSPP) / LCDG_IM_CHUNKSPP)

// ============================================================================

#ifdef	__SMURPH__

#define	lcdg_im_fpg	_dap (lcdg_im_fpg)
#define	lcdg_im_lpg	_dap (lcdg_im_lpg)
#define	lcdg_im_dhk	_dap (lcdg_im_dhk)

// ============================================================================

#else

#include "lcdg_images_node_data.h"

#endif

// ============================================================================

static const word lcdg_im_eraw = 0xffff;	// Marks a free page

void lcdg_im_init (word fp, word np) {
//
// Initialize things: fp = first page of EEPROM, np = number of pages (0 stands
// for no limit
//
	lcdg_im_lpg = (word)(ee_size (NULL, NULL) >> LCDG_IM_PAGE_SHIFT);
	if ((lcdg_im_fpg = fp) >= lcdg_im_lpg)
		syserror (EREQPAR, "im_init");

	if (np != 0 && (np += lcdg_im_fpg) < lcdg_im_lpg)
		lcdg_im_lpg = np;

	// FIXME: put in here consistency checks, or (maybe) provide a separate
	// function (later)
}

static Boolean hdrtst (word pn) {
//
// Check if the page number points to an image header
//
	if (pn < lcdg_im_fpg || pn >= lcdg_im_lpg)
		return YES;

	ee_read (lcdg_im_pntopa (pn) + LCDG_IM_PO_MAGIC, (byte*) (&pn), 2);
	if (pn != 0x7f00)
		return YES;

	return NO;
}

static Boolean isfree (word pn) {
//
// Check if the page is free
//
	ee_read (lcdg_im_pntopa (pn) + LCDG_IM_PO_MAGIC, (byte*) (&pn), 2);
	return (pn == lcdg_im_eraw);
}

static void simrec (byte xo, byte yo, byte xp, byte yp) {
//
// Set picture frame possibly adjusting the origin, so the picture fits
//
	word cz;

	// Sanitize origin coordinates
	if ((cz = (word)xo + xp) > LCDG_MAXX+1)
		xo -= (byte)(cz - (LCDG_MAXX+1));
	if ((cz = (byte)yo + yp) > LCDG_MAXY+1)
		yo -= (byte)(cz - (LCDG_MAXY+1));

	lcdg_set (xo, yo, xp + xo - 1, yp + yo - 1);
}

word lcdg_im_find (const byte *lbl, byte len, word pn) {
//
// Find the specified image by its label, pn is the starting page number
// (for continuing search); note that len can be zero, which means "next image"
//
	lword ep;
	word cw;
	byte *lab;

	if (len) {
		if (len > LCDG_IM_LABLEN)
			len = LCDG_IM_LABLEN;
		// Allocate only len bytes
		if ((lab = (byte*) umalloc (len)) == NULL)
			// FIXME: there is no way to tell this from not found
			return WNONE;
	} else
		// Not needed, but ... just in case
		lab = NULL;

	if (pn == WNONE)
		pn = lcdg_im_fpg;
	else
		pn++;

	for ( ; pn < lcdg_im_lpg; pn++) {
		ep = lcdg_im_pntopa (pn);
		ee_read (ep + LCDG_IM_PO_MAGIC, (byte*)(&cw), 2);
		if (cw == 0x7f00) {
			// This is an image
			if (len == 0)
				// Return immediately, no need to free
				// anything
				return pn;
			// Match the label at the beginning
			ee_read (ep + LCDG_IM_PO_LABEL, lab, len);
			for (cw = 0; cw < len; cw++)
				if (lab [cw] != lbl [cw])
					goto Next;

			// We have a match
			ufree (lab);
			return pn;
		}
Next:		CNOP;
	}
	// Not found
	if (len)
		ufree (lab);
	return WNONE;
}

byte lcdg_im_hdr (word pn, lcdg_im_hdr_t *sig) {
//
// Return the current image's header
//
	if (hdrtst (pn))
		return LCDG_IMGERR_HANDLE;

	ee_read (lcdg_im_pntopa (pn) + LCDG_IM_PO_HDR, (byte*) sig,
		sizeof (lcdg_im_hdr_t));
	return 0;
}

byte lcdg_im_disp (word pn, byte x, byte y) {
//
// Display the image
//
	lword ep;
	word cw, cx, cy;
	lcdg_im_sig_t *sig;

	if (hdrtst (pn))
		return LCDG_IMGERR_HANDLE;

	if ((sig = (lcdg_im_sig_t*) umalloc (sizeof (lcdg_im_sig_t))) == NULL)
		// No memory
		return LCDG_IMGERR_NOMEM;

	ep = lcdg_im_pntopa (pn);
	ee_read (ep + LCDG_IM_PO_SIG, (byte*)sig, LCDG_IM_SIGSIZE);

	// Sanity checks
	if (sig->np > LCDG_IM_MAXPPI || sig->nc > LCDG_IM_MAXCHUNKS ||
	    sig->x > LCDG_MAXX+1 || sig->y > LCDG_MAXY+1) {
		ufree (sig);
		return LCDG_IMGERR_GARBAGE;
	}

	// Set the frame
	simrec (x, y, sig->x, sig->y);

	// Initial offset on first page
	cw = pn = 0;
	while (sig->nc) {
		if ((cw += LCDG_IM_CHUNKLEN) > LCDG_IM_MAX_COFF) {
			// Next page
			cx = sig->ppt [++pn];
			if (cx < lcdg_im_fpg || cx >= lcdg_im_lpg)
				// Abort
				break;
			ep = lcdg_im_pntopa (cx);
			cw = 0;
		}
		// Render current chunk
		ee_read (ep + cw, (byte*)(&(sig->cn)), LCDG_IM_CHUNKLEN);
		cx = sig->cn * LCDG_PIXPERCHK;
		cy = cx / sig->x;
		cx = cx % sig->x;
		lcdg_render (cx, cy, sig->chunk, LCDG_PIXPERCHK);
		sig->nc--;
	}

	ufree (sig);
	lcdg_end ();
	return 0;
}

word lcdg_im_free () {
//
// Returns the number of free pages
//
	word pn, wc;

	for (wc = 0, pn = lcdg_im_fpg; pn < lcdg_im_lpg; pn++)
		if (isfree (pn))
			wc++;

	return wc;
}

byte lcdg_im_purge (word pn) {
//
// Delete the image pointed to by pn
//
	word cw;
	lcdg_im_sig_t *sig;

	if (pn == WNONE) {
		// All
		return ee_erase (WNONE, lcdg_im_pntopa (lcdg_im_fpg),
		    lcdg_im_pntopa (lcdg_im_lpg) - 1) ? LCDG_IMGERR_FAIL : 0;
	}

	if (hdrtst (pn))
		return LCDG_IMGERR_HANDLE;

	if ((sig = (lcdg_im_sig_t*) umalloc (LCDG_IM_SIGSIZE)) == NULL)
		return LCDG_IMGERR_NOMEM;

	ee_read (lcdg_im_pntopa (pn) + LCDG_IM_PO_SIG, (byte*)sig,
		LCDG_IM_SIGSIZE);

	if (sig->ppt [0] != pn) {
Garbage:
		ufree (sig);
		return LCDG_IMGERR_GARBAGE;
	}

	for (cw = 0; cw < sig->np; cw++) {
		pn = sig->ppt [cw];
		if (pn < lcdg_im_fpg || pn >= lcdg_im_lpg)
			goto Garbage;
		ee_write (WNONE, lcdg_im_pntopa (pn) + LCDG_IM_PO_MAGIC,
			(byte*)(&lcdg_im_eraw), 2);
	}

	ufree (sig);
	ee_sync (WNONE);
	return 0;
}

// ============================================================================
// OEP functions ==============================================================
// ============================================================================

#define	IMSData		((img_s_t*)lcdg_im_dhk)
#define	imsd_ppt	(IMSData->ppt)
#define	imsd_cmap	(IMSData->cmap)

// ============================================================================

#define	IMRData		((img_r_t*)lcdg_im_dhk)
#define	imrd_cpp	(IMRData->cpp)
#define	imrd_cp		(IMRData->cp)
#define	imrd_wid	(IMRData->wid)
#define	imrd_rdy	(IMRData->rdy)
#define	imrd_np		(IMRData->np)
#define	imrd_ppt	(IMRData->ppt)
#define	imrd_shw	(IMRData->shw)

static void lcdg_im_chunk_out (word state, address pay, word chn) {
//
// Chunk handler for the sender: fills the payload with the specified chunk
//
	word pn, cn;

	// Chunk coordinates
	cn = imsd_cmap [chn];
	pn = cn >> LCDG_IM_PAGE_SHIFT;
	cn &= LCDG_IM_PAGE_SHIFT;
	ee_read (lcdg_im_pntopa (imsd_ppt [pn]) + cn + 2, (byte*)pay,
		OEP_CHUNKLEN);
}

byte oep_im_snd (word ylid, byte yrqn, word pn) {
//
// Send the image over OEP
//
	lword ep;
	word cw, ws, cn, nc;

	if (hdrtst (pn))
		return LCDG_IMGERR_HANDLE;

	// Need to know the number of chunks first
	ep = lcdg_im_pntopa (pn);
	ee_read (ep + LCDG_IM_PO_NCHUNKS, (byte*)(&nc), 2);
	if (nc == 0 || nc > LCDG_IM_MAXCHUNKS)
		return LCDG_IMGERR_GARBAGE;

	// Allocate the structure
	if ((lcdg_im_dhk = (void*) umalloc (sizeof (img_s_t) + (nc << 1))) ==
	    NULL)
		return LCDG_IMGERR_NOMEM;

	// Read in the signature
	ee_read (ep + LCDG_IM_PO_PPT, (byte*)(&(imsd_ppt [0])),
		(LCDG_IM_MAXPPI+1)*2);

	// Build the chunk map
	for (cw = 0; cw < nc; cw++)
		// Preinit for verifying completeness
		imsd_cmap [cw] = WNONE;

	ws = LCDG_IM_CHUNKLEN;
	// Page number index in PPT
	pn = 0;
	for (cw = 0; cw < nc; cw++) {
		if (ws > LCDG_IM_MAX_COFF) {
			pn++;
			ws = imsd_ppt [pn];
			if (ws < lcdg_im_fpg || ws >= lcdg_im_lpg) {
				// Garbage
Garbage:
				ufree (lcdg_im_dhk);
				return LCDG_IMGERR_GARBAGE;
			}
			ep = lcdg_im_pntopa (ws);
			ws = 0;
		}

		// Just the chunk number
		ee_read (ep + ws, (byte*)(&cn), 2);

		if (cn >= nc || imsd_cmap [cn] != WNONE)
			// Bad image
			goto Garbage;

		imsd_cmap [cn] = (pn << LCDG_IM_PAGE_SHIFT) | ws;
		ws += LCDG_IM_CHUNKLEN;
	}
	// Check for missing chunks in CMAP
	for (cw = 0; cw < nc; cw++)
		if (imsd_cmap [cw] == WNONE)
			goto Garbage;

	// Now start the OEP part; note that nc gives the total number
	// of chunks - 1, as the header chunk doesn't count (i.e., it is
	// handled "behind the scenes"

	if ((ws = oep_snd (ylid, yrqn, nc, lcdg_im_chunk_out)) != 0) {
		ufree (lcdg_im_dhk);
		return ws;
	}

	return 0;
}

// ============================================================================

static void lcdg_im_chunk_in (word state, address pay, word chn) {
//
// Chunk handler for the receiver
//
	word cs, rc;

	if (imrd_rdy) {

		if (imrd_shw) {
			// Displaying
			cs = chn * LCDG_PIXPERCHK;
			// Is there a way to avoid division? Have to calculate
			// starting row and column
			rc = cs / imrd_wid;
			cs = cs % imrd_wid;
			lcdg_render ((byte)cs, (byte)rc, (byte*)pay,
				LCDG_PIXPERCHK);
		}

		// Prepare for possible blocking
		imrd_rdy = NO;
	}

	// We play a trick nowing that pay is a packet fragment, and that
	// it includes the chunk number in front of it
	ee_write (state, imrd_cpp, (byte*)(pay-1), LCDG_IM_CHUNKLEN);
	// If we succeed
	imrd_rdy = YES;

	// Advance the pointer
	if ((word)(imrd_cpp & LCDG_IM_PAGE_SHIFT) >= LCDG_IM_MAX_COFF) {
		if (++imrd_cp < imrd_np)
			imrd_cpp = lcdg_im_pntopa (imrd_ppt [imrd_cp]);
	} else {
		imrd_cpp += LCDG_IM_CHUNKLEN;
	}
}

byte oep_im_rcv (const lcdg_im_hdr_t *hdr, byte xd, byte yd) {
//
// Receive the image over OEP
//
	word np, nc, pn;

	if (hdr->X == 0 || hdr->X > LCDG_MAXX+1 || hdr->Y == 0 ||
	    hdr->Y > LCDG_MAXY+1)
		return LCDG_IMGERR_GARBAGE;

	// Calculate the number of chunks (-1, the header chunk doesn't count)
	nc = (((word)(hdr->X) * (word)(hdr->Y)) +
		LCDG_PIXPERCHK-1) / LCDG_PIXPERCHK;

	if ((lcdg_im_dhk = (void*)umalloc (sizeof (img_r_t))) == NULL)
		// No memory
		return LCDG_IMGERR_NOMEM;

	// The number of pages required
	np = lcdg_im_chtop (nc);

	// Find free pages
	for (imrd_np = 0, pn = lcdg_im_fpg; pn < lcdg_im_lpg; pn++) {
		if (isfree (pn)) {
			imrd_ppt [imrd_np++] = pn;
			if (imrd_np == np)
				// All we need
				goto GotThem;
		}
	}

	// Not enough memory
	ufree (lcdg_im_dhk);
	return LCDG_IMGERR_NOSPACE;
	
GotThem:
	// Intialize the out pointer
	imrd_cpp = lcdg_im_pntopa (imrd_ppt [imrd_cp = 0]);

	// Start the first page
	np = 0x7f00;
	ee_write (WNONE, imrd_cpp + LCDG_IM_PO_MAGIC, (byte*)(&np), 2);

	// First part of the signature
	ee_write (WNONE, imrd_cpp + LCDG_IM_PO_SIG, (byte*)(&imrd_np),
		(LCDG_IM_MAXPPI + 1) * 2);

	// The number of chunks
	ee_write (WNONE, imrd_cpp + LCDG_IM_PO_NCHUNKS, (byte*)(&nc), 2);

	// Second part of the signature
	ee_write (WNONE, imrd_cpp + LCDG_IM_PO_X, (byte*)hdr,
		sizeof (lcdg_im_hdr_t));

	// Point to the first actual chunk
	imrd_cpp += LCDG_IM_CHUNKLEN;

	// Need this only if xd != BNONE
	imrd_wid = hdr->X;

	if (xd != BNONE) {
		// Display while receiving
		imrd_shw = YES;
		simrec (xd, yd, hdr->X, hdr->Y);
	} else
		imrd_shw = NO;

	imrd_rdy = YES;

	// Now for the OEP part

	if ((np = oep_rcv (nc, lcdg_im_chunk_in)) != 0) {
		ufree (lcdg_im_dhk);
		return np;
	}
	return 0;
}

void oep_im_cleanup () {
//
// Deallocate data (to be called by the praxis when the transaction is over -
// one way or the other)
//
	word cw;

	if (oep_lastop () == OEP_STATUS_RCV) {
		if (oep_status () != OEP_STATUS_DONE) {
			// Need to remove the partially received image
			for (cw = 0; cw < imrd_np; cw++)
				ee_write (WNONE, lcdg_im_pntopa (imrd_ppt [cw])
					+ LCDG_IM_PO_MAGIC,
						(byte*)(&lcdg_im_eraw), 2);
		}
#ifdef __SMURPH__
		// This should be optimized out anyway (in PicOS), but can we
		// be sure?
		else if (imrd_shw) {
			lcdg_end ();
		}
#endif
		ee_sync (WNONE);
	}
	ufree (lcdg_im_dhk);
}

// Make sure these will not get in the way of other moduled that may follow this
// one in VUEE's combined source

#undef	IMSData		
#undef	imsd_ppt	
#undef	imsd_cmap	

#undef	IMRData		
#undef	imrd_cpp	
#undef	imrd_cp		
#undef	imrd_wid	
#undef	imrd_rdy	
#undef	imrd_np		
#undef	imrd_ppt	
#undef	imrd_shw	

#endif
