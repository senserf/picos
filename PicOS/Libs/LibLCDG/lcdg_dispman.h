#ifndef	__lcdg_dispman_h__
#define	__lcdg_dispman_h__

#include "lcdg_images.h"
#include "lcdg_n6100p.h"
#include "oep.h"

//+++ "lcdg_dispman.c"

#include "lcdg_dispman_params.h"
#include "lcdg_dispman_types.h"

#ifdef __SMURPH__
// ============================================================================

#define	LCDG_DM_TOP	_dap (LCDG_DM_TOP)
#define	LCDG_DM_HEAD	_dap (LCDG_DM_HEAD)
#define	LCDG_DM_STATUS	_dap (LCDG_DM_STATUS)

#else
// ============================================================================

extern lcdg_dm_obj_t	*LCDG_DM_TOP, *LCDG_DM_HEAD;
extern byte		LCDG_DM_STATUS;

#endif
// ============================================================================

//+++
Boolean lcdg_dm_shown (const lcdg_dm_obj_t*);
void lcdg_dm_menu_d (lcdg_dm_men_t*);
void lcdg_dm_menu_u (lcdg_dm_men_t*);
void lcdg_dm_menu_l (lcdg_dm_men_t*);
void lcdg_dm_menu_r (lcdg_dm_men_t*);
byte lcdg_dm_display (lcdg_dm_obj_t*);
byte lcdg_dm_dtop (void);
// Update i-th line of the top menu
byte lcdg_dm_update (lcdg_dm_men_t*, word);
lcdg_dm_obj_t *lcdg_dm_remove (lcdg_dm_obj_t*);
byte lcdg_dm_refresh (void);
byte lcdg_dm_newtop (lcdg_dm_obj_t*);
void lcdg_dm_free (lcdg_dm_obj_t*);

#if LCDG_DM_EE_CREATORS
#include "ee_ol.h"
lcdg_dm_obj_t *lcdg_dm_newmenu_e (lword);
lcdg_dm_obj_t *lcdg_dm_newtext_e (lword);
lcdg_dm_obj_t *lcdg_dm_newimage_e (lword);
#endif

#if LCDG_DM_SOFT_CREATORS
#if LCDG_DM_SOFT_CREATORS == 1
lcdg_dm_obj_t *lcdg_dm_newmenu (char**, word, byte*);
lcdg_dm_obj_t *lcdg_dm_newtext (char*, byte*);
lcdg_dm_obj_t *lcdg_dm_newimage (word, byte*);
#else
lcdg_dm_obj_t *lcdg_dm_newmenu (char**,
			word, byte, byte, byte, byte, byte, byte, byte);
lcdg_dm_obj_t *lcdg_dm_newtext (char*,
			byte, byte, byte, byte, byte, byte);
lcdg_dm_obj_t *lcdg_dm_newimage (word, byte, byte);
#endif /* LCDG_DM_SOFT_CREATORS */
#endif /* LCDG_DM_SOFT_CREATORS */

char **lcdg_dm_asa (word);
void lcdg_dm_csa (char**, word);

#define	lcdg_dm_menu_c(m)	(((lcdg_dm_men_t*)(m))->SE)
#define	lcdg_dm_menu_s(m)	(((lcdg_dm_men_t*)(m))->NL)

#endif
