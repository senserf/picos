/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_lcdg_images_h__
#define __pg_lcdg_images_h__

#include "sysio.h"
#include "lcdg_n6100p.h"
#include "oep.h"

//+++ "lcdg_images.c"

#define	LCDG_IM_PAGESIZE	8192
// Max chunks per image
#define	LCDG_IM_MAXCHUNKS	720
// Image chunk length (includes the chunk number, which has to be stored)
#define	LCDG_IM_CHUNKLEN	(OEP_CHUNKLEN + 2)
// Chunks per page (they never cross page boundaries)
#define	LCDG_IM_CHUNKSPP	(LCDG_IM_PAGESIZE / LCDG_IM_CHUNKLEN)
// Label length
#define	LCDG_IM_LABLEN		(LCDG_IM_CHUNKLEN - 18)
// Max pages per image
#define	LCDG_IM_MAXPPI	5

// Errors (kind of compatible with OEP status values)
#define	LCDG_IMGERR_HANDLE	32
#define	LCDG_IMGERR_NOMEM	OEP_STATUS_NOMEM
#define	LCDG_IMGERR_GARBAGE	33
#define	LCDG_IMGERR_NOSPACE	34
#define	LCDG_IMGERR_FAIL	35

// ===========================================================================

typedef	struct {
	byte X, Y;
	// These are arbitrarily bytes of fixed length
	byte Label [LCDG_IM_LABLEN];
}  lcdg_im_hdr_t;

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
	word	ppt [LCDG_IM_MAXPPI];
	word	nc;	// Number of chunks
	byte	x, y;	// X-size/Y-size
	// --- label follows

	// --- extras (if the image has to be displayed)
	word	cn;
	byte	chunk [OEP_CHUNKLEN];

} lcdg_im_sig_t;

// The size of the mandatory part
#define	LCDG_IM_SIGSIZE	((3 + LCDG_IM_MAXPPI) * 2)

// Offsets into the first chunk

#define	LCDG_IM_PO_MAGIC	0
#define	LCDG_IM_PO_SIG		2		// Signature offset
#define	LCDG_IM_PO_NPAGES	2
#define	LCDG_IM_PO_PPT		4
#define	LCDG_IM_PO_NCHUNKS	14		// Total number of chunks
#define	LCDG_IM_PO_HDR		16		// Image header
#define	LCDG_IM_PO_X		16
#define	LCDG_IM_PO_Y		17
#define	LCDG_IM_PO_LABEL	18

typedef struct {
//
// Transaction structure for sending an image over OEP
//

// -----------------
// Signature + label
// -----------------
	word		ppt [LCDG_IM_MAXPPI];	// Page numbers
// -----------------
	word		cmap [0];		// Chunk map

} img_s_t;

typedef struct {
//
// Transaction structure for receiving an image over OEP
//
	lword		cpp;	// Chunk pointer to write to eeprom
// ----------------
	word		np;			// Number or pages
	word		ppt [LCDG_IM_MAXPPI];	// Page numbers
// ----------------
	word		cp;	// Current page index
	word		wid;	// Image width (needed if also displaying)
	Boolean		rdy,	// eeprom ready flag
			shw;	// Display while receiving
} img_r_t;

// ============================================================================

void	lcdg_im_init (word, word);
word	lcdg_im_find (const byte*, byte, word);
byte	lcdg_im_hdr (word, lcdg_im_hdr_t*);
byte	lcdg_im_disp (word, byte, byte);
word	lcdg_im_free ();
byte	lcdg_im_purge (word);
byte	oep_im_snd (word, byte, word);
byte	oep_im_rcv (const lcdg_im_hdr_t*, byte, byte);
void	oep_im_cleanup ();

#endif
