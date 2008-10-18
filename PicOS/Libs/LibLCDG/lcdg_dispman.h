#ifndef	__lcdg_dispman_h__
#define	__lcdg_dispman_h__

#include "oep.h"
#include "lcdg_images.h"

//+++ "lcdg_dispman.c"

struct lcdg_dm_obj_s;

#define	LCDG_DMTYPE_IMAGE	0
#define	LCDG_DMTYPE_MENU	1
#define	LCDG_DMTYPE_TEXT	2

// Errors (compatible with OEP)
#define	LCDG_DMERR_NOMEM	OEP_STATUS_NOMEM
#define	LCDG_DMERR_GARBAGE	LCDG_IMGERR_GARBAGE
#define	LCDG_DMERR_NOFONT	40
#define	LCDG_DMERR_RECT		41

#define	LCDG_DOBJECT_STUFF	\
	byte			XL, YL, XH, YH; \
	struct lcdg_dm_obj_s	*next; \
	address			Extras; \
	byte			Type

struct lcdg_dm_obj_s {
//
// Displayable object: this is the minimum that every object must have
//
	LCDG_DOBJECT_STUFF;

};

typedef	struct lcdg_dm_obj_s lcdg_dm_obj_t;

typedef struct {
//
// Image
//
	LCDG_DOBJECT_STUFF;
	word	EPointer;	// Pointer to EEPROM page

} lcdg_dm_img_t;

typedef struct {
//
// Menu
//
	LCDG_DOBJECT_STUFF;

	byte Font;		// Font ID
	byte BG, FG;		// Colors
	const char **Lines;	// This must be allocated by the praxis
	word NL;		// Number of lines
	byte Width, Height;	// In characters
	word FL,		// First line displayed
	     SH,                // Shift (horizontal scroll)
	     SE;		// Selected line

} lcdg_dm_men_t;

typedef struct {
//
// Static text
//
	LCDG_DOBJECT_STUFF;

	byte Font;
	byte BG, FG;		// Colors
	const char *Line;	// This must be allocated by the praxis
	byte Width, Height;	// In characters
	

} lcdg_dm_tex_t;

extern lcdg_dm_obj_t	*LCDG_DM_TOP, *LCDG_DM_HEAD;
extern byte		LCDG_DM_STATUS;

Boolean lcdg_dm_shown (const lcdg_dm_obj_t*);
void lcdg_dm_menu_d (lcdg_dm_men_t*);
void lcdg_dm_menu_u (lcdg_dm_men_t*);
void lcdg_dm_menu_l (lcdg_dm_men_t*);
void lcdg_dm_menu_r (lcdg_dm_men_t*);
byte lcdg_dm_display (lcdg_dm_obj_t*);
byte lcdg_dm_dtop (void);
lcdg_dm_obj_t *lcdg_dm_remove (lcdg_dm_obj_t*);
byte lcdg_dm_refresh (void);
byte lcdg_dm_newtop (lcdg_dm_obj_t*);
lcdg_dm_obj_t *lcdg_dm_newmenu (const char**,
			word, byte, byte, byte, byte, byte, byte, byte);
lcdg_dm_obj_t *lcdg_dm_newtext (const char*,
			byte, byte, byte, byte, byte, byte);
lcdg_dm_obj_t *lcdg_dm_newimage (word, byte, byte);
char **lcdg_dm_asa (word);
void lcdg_dm_csa (char**, word);

#define	lcdg_dm_menu_c(m)	(((lcdg_dm_men_t*)(m))->SE)
#define	lcdg_dm_menu_s(m)	(((lcdg_dm_men_t*)(m))->NL)

#endif
