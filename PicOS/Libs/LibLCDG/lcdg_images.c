#include "sysio.h"
#include "lcdg_images.h"
#include "oep.h"

#define	IMG_PAGE_SHIFT	13		// 2^13 = 8192
#define	IMG_PAGE_MASK	((1<<IMG_PAGE_SHIFT)-1)

// Convert page number to page address
#define	pntopa(a)	(((lword)(a)) << IMG_PAGE_SHIFT)

// Maximum offset of a chunk origin from beginning of page
#define	IMG_MAX_COFF	((LCDG_IM_CHUNKSPP-1) * LCDG_IM_CHUNKLEN)
#define	IMG_MAXPPI	5			// Max pages per image

// Number of pixels covered by one chunk
#define	PIXPERCHUNK	((OEP_CHUNKLEN * 8) / 12)

// Convert the number of chunks to the number of pages; note that n is the
// number of chunks - 1
#define	chunks_to_pages(n)	(((n) + LCDG_IM_CHUNKSPP) / LCDG_IM_CHUNKSPP)

// ============================================================================
//
// First chunk layout (of the first page):
//
// Bytes 00-01:	 0x7f00 (a magic of sorts)
// Bytes 02-03:  the number of pages, including this one
// Bytes 04-13:  5 words -> page numbers (starting with this page number)
// Bytes 14-15:	 the total number of chunks in the image
// Bytes 16-17:  X/Y-dimensions in pixels
// Bytes 18-55:  The label, 38 bytes fixed length
//
// ============================================================================

typedef	struct {
//
// Image signature: bytes 00-17 of the first chunk + (occasionally) a bit
// extra
//
	word	np;	// Number of pages
	word	ppt [IMG_MAXPPI];
	word	nc;	// Number of chunks
	byte	x, y;	// X-size/Y-size
	// --- label follows

	// --- extras (if the image has to be displayed)
	word	cn;
	byte	chunk [OEP_CHUNKLEN];

} img_sig_t;

// The size of the mandatory part
#define	IMG_SIGSIZE	((3 + IMG_MAXPPI) * 2)

// Offsets into the first chunk

#define	IMG_PO_MAGIC	0
#define	IMG_PO_SIG	2		// Signature offset
#define	IMG_PO_NPAGES	2
#define	IMG_PO_PPT	4
#define	IMG_PO_NCHUNKS	14		// Total number of chunks
#define	IMG_PO_HDR	16		// Image header
#define	IMG_PO_X	16
#define	IMG_PO_Y	17
#define	IMG_PO_LABEL	18

// ============================================================================
// ============================================================================

// These can be used to bound the range of EEPROM pages for storing images,
// i.e., to implement a kind of "partition". For now, we assume that the
// first page of EEPROM is reserved for fonts, and the "partition" extends
// from page 1 to the last EEPROM page. The bounds are set with lcdg_im_init.

static	word	img_first_page,		// First page
		img_last_page;		// Last page + 1

static const word eraw = 0xffff;	// Marks a free page

void lcdg_im_init (word fp, word np) {
//
// Initialize things: fp = first page of EEPROM, np = number of pages (0 stands
// for no limit
//
	img_last_page = (word)(ee_size (NULL, NULL) >> IMG_PAGE_SHIFT);
	if ((img_first_page = fp) >= img_last_page)
		syserror (EREQPAR, "im_init");

	if (np != 0 && (np += img_first_page) < img_last_page)
		img_last_page = np;

	// FIXME: put in here consistency checks, or (maybe) provide a separate
	// function (later)
}

static Boolean hdrtst (word pn) {
//
// Check if the page number points to an image header
//
	if (pn < img_first_page || pn >= img_last_page)
		return YES;

	ee_read (pntopa (pn) + IMG_PO_MAGIC, (byte*) (&pn), 2);
	if (pn != 0x7f00)
		return YES;

	return NO;
}

static Boolean isfree (word pn) {
//
// Check if the page is free
//
	ee_read (pntopa (pn) + IMG_PO_MAGIC, (byte*) (&pn), 2);
	return (pn == eraw);
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
	}

	if (pn == WNONE)
		pn = img_first_page;
	else
		pn++;

	for ( ; pn < img_last_page; pn++) {
		ep = pntopa (pn);
		ee_read (ep + IMG_PO_MAGIC, (byte*)(&cw), 2);
		if (cw == 0x7f00) {
			// This is an image
			if (len == 0)
				// Return immediately, no need to free
				// anything
				return pn;
			// Match the label at the beginning
			ee_read (ep + IMG_PO_LABEL, lab, len);
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

	ee_read (pntopa (pn) + IMG_PO_HDR, (byte*) sig, sizeof (lcdg_im_hdr_t));
	return 0;
}

byte lcdg_im_disp (word pn, byte x, byte y) {
//
// Display the image
//
	lword ep;
	word cw, cx, cy;
	img_sig_t *sig;

	if (hdrtst (pn))
		return LCDG_IMGERR_HANDLE;

	if ((sig = (img_sig_t*) umalloc (sizeof (img_sig_t))) == NULL)
		// No memory
		return LCDG_IMGERR_NOMEM;

	ep = pntopa (pn);
	ee_read (ep + IMG_PO_SIG, (byte*)sig, IMG_SIGSIZE);

	// Sanity checks
	if (sig->np > IMG_MAXPPI || sig->nc > LCDG_IM_MAXCHUNKS ||
	    sig->x > LCDG_MAXX+1 || sig->y > LCDG_MAXY+1) {
		ufree (sig);
		return LCDG_IMGERR_GARBAGE;
	}

	// Set the frame
	simrec (x, y, sig->x, sig->y);

	// Initial offset on first page
	cw = pn = 0;
	while (sig->nc) {
		if ((cw += LCDG_IM_CHUNKLEN) > IMG_MAX_COFF) {
			// Next page
			cx = sig->ppt [++pn];
			if (cx < img_first_page || cx >= img_last_page)
				// Abort
				break;
			ep = pntopa (cx);
			cw = 0;
		}
		// Render current chunk
		ee_read (ep + cw, (byte*)(&(sig->cn)), LCDG_IM_CHUNKLEN);
		cx = sig->cn * PIXPERCHUNK;
		cy = cx / sig->x;
		cx = cx % sig->x;
		lcdg_render (cx, cy, sig->chunk, PIXPERCHUNK);
		sig->nc--;
	}

	ufree (sig);
	return 0;
}

word lcdg_im_free () {
//
// Returns the number of free pages
//
	word pn, wc;

	for (wc = 0, pn = img_first_page; pn < img_last_page; pn++)
		if (isfree (pn))
			wc++;

	return wc;
}

byte lcdg_im_purge (word pn) {
//
// Delete the image pointed to by pn
//
	word cw;
	img_sig_t *sig;

	if (pn == WNONE) {
		// All
		return ee_erase (WNONE, pntopa (img_first_page),
		    pntopa (img_last_page) - 1) ? LCDG_IMGERR_FAIL : 0;
	}

	if (hdrtst (pn))
		return LCDG_IMGERR_HANDLE;

	if ((sig = (img_sig_t*) umalloc (IMG_SIGSIZE)) == NULL)
		return LCDG_IMGERR_NOMEM;

	ee_read (pntopa (pn) + IMG_PO_SIG, (byte*)sig, IMG_SIGSIZE);
	if (sig->ppt [0] != pn) {
Garbage:
		ufree (sig);
		return LCDG_IMGERR_GARBAGE;
	}

	for (cw = 0; cw < sig->np; cw++) {
		pn = sig->ppt [cw];
		if (pn < img_first_page || pn >= img_last_page)
			goto Garbage;
		ee_write (WNONE, pntopa (pn) + IMG_PO_MAGIC, (byte*)(&eraw), 2);
	}

	ufree (sig);
	ee_sync (WNONE);
	return 0;
}

// ============================================================================
// OEP functions ==============================================================
// ============================================================================

static void	*DHook = NULL;				// OEP structure pointer

typedef struct {
//
// Transaction structure for sending an image
//

// -----------------
// Signature + label
// -----------------

	word		ppt [IMG_MAXPPI];	// Page numbers
// -----------------
	word		cmap [0];		// Chunk map

} img_s_t;

#define	IMSData		((img_s_t*)DHook)
#define	imsd_ppt	(IMSData->ppt)
#define	imsd_cmap	(IMSData->cmap)

// ============================================================================

typedef struct {
//
// Transaction structure for receiving an image
//
	lword		cpp;	// Chunk pointer to write to eeprom
// ----------------
	word		np;			// Number or pages
	word		ppt [IMG_MAXPPI];	// Page numbers
// ----------------
	word		cp;	// Current page index
	word		wid;	// Image width (needed if also displaying)
	Boolean		rdy,	// eeprom ready flag
			shw;	// Display while receiving
} img_r_t;

#define	IMRData		((img_r_t*)DHook)
#define	imrd_cpp	(IMRData->cpp)
#define	imrd_cp		(IMRData->cp)
#define	imrd_wid	(IMRData->wid)
#define	imrd_rdy	(IMRData->rdy)
#define	imrd_np		(IMRData->np)
#define	imrd_ppt	(IMRData->ppt)
#define	imrd_shw	(IMRData->shw)

static void chunk_out (word state, address pay, word chn) {
//
// Chunk handler for the sender: fills the payload with the specified chunk
//
	word pn, cn;

	// Chunk coordinates
	cn = imsd_cmap [chn];
	pn = cn >> IMG_PAGE_SHIFT;
	cn &= IMG_PAGE_MASK;
	ee_read (pntopa (imsd_ppt [pn]) + cn + 2, (byte*)pay, OEP_CHUNKLEN);
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
	ep = pntopa (pn);
	ee_read (ep + IMG_PO_NCHUNKS, (byte*)(&nc), 2);
	if (nc == 0 || nc > LCDG_IM_MAXCHUNKS)
		return LCDG_IMGERR_GARBAGE;

	// Allocate the structure
	if ((DHook = (void*) umalloc (sizeof (img_s_t) + (nc << 1))) == NULL)
		return LCDG_IMGERR_NOMEM;

	// Read in the signature
	ee_read (ep + IMG_PO_PPT, (byte*)(&(imsd_ppt [0])), (IMG_MAXPPI+1)*2);

	// Build the chunk map
	for (cw = 0; cw < nc; cw++)
		// Preinit for verifying completeness
		imsd_cmap [cw] = WNONE;

	ws = LCDG_IM_CHUNKLEN;
	// Page number index in PPT
	pn = 0;
	for (cw = 0; cw < nc; cw++) {
		if (ws > IMG_MAX_COFF) {
			pn++;
			ws = imsd_ppt [pn];
			if (ws < img_first_page || ws >= img_last_page) {
				// Garbage
Garbage:
				ufree (DHook);
				return LCDG_IMGERR_GARBAGE;
			}
			ep = pntopa (ws);
			ws = 0;
		}

		// Just the chunk number
		ee_read (ep + ws, (byte*)(&cn), 2);

		if (cn >= nc || imsd_cmap [cn] != WNONE)
			// Bad image
			goto Garbage;

		imsd_cmap [cn] = (pn << IMG_PAGE_SHIFT) | ws;
		ws += LCDG_IM_CHUNKLEN;
	}
	// Check for missing chunks in CMAP
	for (cw = 0; cw < nc; cw++)
		if (imsd_cmap [cw] == WNONE)
			goto Garbage;

	// Now start the OEP part; note that nc gives the total number
	// of chunks - 1, as the header chunk doesn't count (i.e., it is
	// handled "behind the scences"

	if ((ws = oep_snd (ylid, yrqn, nc, chunk_out)) != 0) {
		ufree (DHook);
		return ws;
	}

	return 0;
}

// ============================================================================

static void chunk_in (word state, address pay, word chn) {
//
// Chunk handler for the receiver
//
	word cs, rc;

	if (imrd_rdy) {

		if (imrd_shw) {
			// Displaying
			cs = chn * PIXPERCHUNK;
			// Is there a way to avoid division? Have to calculate
			// starting row and column
			rc = cs / imrd_wid;
			cs = cs % imrd_wid;
			lcdg_render ((byte)cs, (byte)rc, (byte*)pay,
				PIXPERCHUNK);
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
	if ((word)(imrd_cpp & IMG_PAGE_MASK) >= IMG_MAX_COFF) {
		if (++imrd_cp < imrd_np)
			imrd_cpp = pntopa (imrd_ppt [imrd_cp]);
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
	nc = (((word)(hdr->X) * (word)(hdr->Y)) + PIXPERCHUNK-1) / PIXPERCHUNK;

	if ((DHook = (void*)umalloc (sizeof (img_r_t))) == NULL)
		// No memory
		return LCDG_IMGERR_NOMEM;

	// The number of pages required
	np = chunks_to_pages (nc);

	// Find free pages
	for (imrd_np = 0, pn = img_first_page; pn < img_last_page; pn++) {
		if (isfree (pn)) {
			imrd_ppt [imrd_np++] = pn;
			if (imrd_np == np)
				// All we need
				goto GotThem;
		}
	}

	// Not enough memory
	ufree (DHook);
	return LCDG_IMGERR_NOSPACE;
	
GotThem:
	// Intialize the out pointer
	imrd_cpp = pntopa (imrd_ppt [imrd_cp = 0]);

	// Start the first page
	np = 0x7f00;
	ee_write (WNONE, imrd_cpp + IMG_PO_MAGIC, (byte*)(&np), 2);

	// First part of the signature
	ee_write (WNONE, imrd_cpp + IMG_PO_SIG, (byte*)(&imrd_np),
		(IMG_MAXPPI + 1) * 2);

	// The number of chunks
	ee_write (WNONE, imrd_cpp + IMG_PO_NCHUNKS, (byte*)(&nc), 2);

	// Second part of the signature
	ee_write (WNONE, imrd_cpp + IMG_PO_X, (byte*)hdr,
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

	if ((np = oep_rcv (nc, chunk_in)) != 0) {
		ufree (DHook);
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
				ee_write (WNONE, pntopa (imrd_ppt [cw]) +
					IMG_PO_MAGIC, (byte*)(&eraw), 2);
		}
		ee_sync (WNONE);
	}
	ufree (DHook);
}
