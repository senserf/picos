#ifndef __pg_lcdg_images_h__
#define __pg_lcdg_images_h__

#include "sysio.h"
#include "lcdg_n6100p.h"
#include "oep.h"

//+++ "lcdg_images.c"

#include "lcdg_images_params.h"
#include "lcdg_images_types.h"

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
