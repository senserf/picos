#ifndef __lcdg_images_types_h__
#define	__lcdg_images_types_h__

#include "lcdg_images_params.h"

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


#endif
