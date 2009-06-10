#ifndef	__lcdg_dispman_types_h__
#define	__lcdg_dispman_types_h__

#include "lcdg_dispman_params.h"

struct lcdg_dm_obj_s;

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
	char **Lines;		// This must be allocated by the praxis
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
	char *Line;		// This must be allocated by the praxis
	byte Width, Height;	// In characters
	

} lcdg_dm_tex_t;

#endif
