#ifndef __pg_lcdg_imahes_h__
#define __pg_lcdg_imahes_h__

#include "sysio.h"
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

// Errors (kind of compatible with OEP status values)
#define	LCDG_IMGERR_HANDLE	32
#define	LCDG_IMGERR_NOMEM	OEP_STATUS_NOMEM
#define	LCDG_IMGERR_GARBAGE	33
#define	LCDG_IMGERR_NOSPACE	34
#define	LCDG_IMGERR_FAIL	35

typedef	struct {
	byte X, Y;
	// These are arbitrarily bytes of fixed length
	byte Label [LCDG_IM_LABLEN];
}  lcdg_im_hdr_t;

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
