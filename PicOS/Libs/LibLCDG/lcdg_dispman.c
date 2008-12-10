#include "sysio.h"
#include "lcdg_dispman.h"

//
// Display manager
//

#define	CCHAR	">"			// Menu pointer character

lcdg_dm_obj_t	*LCDG_DM_TOP = NULL,	// The top object displayed
		*LCDG_DM_HEAD = NULL;	// The front of list

byte		LCDG_DM_STATUS = 0;	// Error status

static word	wp_handle = 0;		// Wallpaper image handle

Boolean lcdg_dm_shown (const lcdg_dm_obj_t *o) {
//
// Check if an object occurs on the list
//
	lcdg_dm_obj_t *c;

	for (c = LCDG_DM_HEAD; c != NULL; c = c->next)
		if (c == o)
			return YES;
	return NO;
}

static Boolean dm_covered (lcdg_dm_obj_t *a, lcdg_dm_obj_t *b) {
//
// Object a is covered by object b
//
	return (
	  a->XL >= b->XL && a->YL >= b->YL && a->XH <= b->XH && a->YH <= b->YH
	);
}

static lcdg_dm_obj_t *dm_prev (lcdg_dm_obj_t *q) {

	lcdg_dm_obj_t *o;

	if (q == LCDG_DM_HEAD)
		return NULL;

	for (o = LCDG_DM_HEAD; o != NULL && o->next != q; o = o->next);
	return o;
}

static void dm_clear () {

	if (wp_handle == 0) {
		// Look up the wallpaper image
		wp_handle = lcdg_im_find ((const byte*)"wallpaper", 10, WNONE);
	}

	if (wp_handle != WNONE) {
		lcdg_im_disp (wp_handle, 0, 0);
	} else {
		// Just erase to background
		lcdg_set (0, 0, LCDG_MAXX, LCDG_MAXY);
		lcdg_setc (COLOR_BLACK, BNONE);
		lcdg_clear ();
	}
}

static byte dm_mell (lcdg_dm_men_t *m, word ln) {
//
// Return effective line length + 1, i.e., the first column index behind the
// last displayed character
//
	word r;

	r = strlen (m->Lines [ln]);
	if (r <= m->SH)
		return 1;

	r = r - m->SH + 1;
	if (r > m->Width)
		r = m->Width;
	return (byte) r;
}

static byte dm_dimage (lcdg_dm_img_t *im) {
//
// Display image pointed to by the page address. Note: this assumes that all
// images are 12 bpp.
//
	lcdg_im_hdr_t *sig;

	if ((sig = (lcdg_im_hdr_t*) umalloc (sizeof (lcdg_im_hdr_t))) == NULL)
		return (LCDG_DM_STATUS = LCDG_DMERR_NOMEM);

	if ((LCDG_DM_STATUS = lcdg_im_hdr (im->EPointer, sig)) != 0) {
FErr:
		ufree (sig);
		return LCDG_DM_STATUS;
	}

	// Verify if the image dimensions haven't changed
	if (sig->X != im->XH - im->XL + 1 ||
	    sig->Y != im->YH - im->YL + 1   ) {
		LCDG_DM_STATUS = LCDG_DMERR_GARBAGE;
		goto FErr;
	}

	// Now do it
	ufree (sig);
	LCDG_DM_STATUS = lcdg_im_disp (im->EPointer, im->XL, im->YL);
	return LCDG_DM_STATUS;
}

static byte dm_dmenu (lcdg_dm_men_t *dm) {
//
// Display a menu object
//
	word i, j, a;

	// This initialization is partially redundant the second and next time
	// around, but harmless
	if (lcdg_font (dm->Font))
		// No way to set font
		return (LCDG_DM_STATUS = LCDG_DMERR_NOFONT);

	lcdg_setc (dm->BG, dm->FG);
	lcdg_set (dm->XL, dm->YL, dm->XH, dm->YH);

	for (i = 0; i < dm->Height; i++) {
		// Actual line number
		j = i + dm->FL;
		if (j >= dm->NL)
			// Done with the lines
			break;
		if (j == dm->SE)
			// The cursor
			lcdg_wl (CCHAR, 0, 0, i);
		else
			lcdg_ec (0, i, 1);

		lcdg_wl (dm->Lines [j], dm->SH, 1, i);

		if ((a = dm_mell (dm, j)) < dm->Width)
			// Need to clear to the end of line
			lcdg_ec (a, i, dm->Width - a);
	}

	// Empty lines to the end
	while (i < dm->Height) {
		lcdg_ec (0, i, dm->Width);
		i++;
	}
	return (LCDG_DM_STATUS = 0);
}

byte lcdg_dm_update (lcdg_dm_men_t *dm, word ln) {
//
// Update (re-display) line ln in menu dm
//
	word i, a;

	// Check if anything to do
	if ((lcdg_dm_obj_t*)dm != LCDG_DM_TOP ||
		ln < dm->FL || (i = ln - dm->FL) >= dm->Height)
			goto Done;

	if (lcdg_font (dm->Font))
		return (LCDG_DM_STATUS = LCDG_DMERR_NOFONT);

	lcdg_setc (dm->BG, dm->FG);
	lcdg_set (dm->XL, dm->YL, dm->XH, dm->YH);

	if (ln == dm->SE)
		// The cursor
		lcdg_wl (CCHAR, 0, 0, i);
	else
		lcdg_ec (0, i, 1);

	lcdg_wl (dm->Lines [ln], dm->SH, 1, i);

	if ((a = dm_mell (dm, ln)) < dm->Width)
		// Need to clear to the end of line
		lcdg_ec (a, i, dm->Width - a);
Done:
	return (LCDG_DM_STATUS = 0);
}

void lcdg_dm_menu_d (lcdg_dm_men_t *m) {
//
// Select next item down the list
//
	word i, j, d;
	byte a, b;

	if (m->SE >= m->NL - 1)
		// Cursor at the last line, do nothing
		return;

	// Relative to the window
	a = (byte) (m->SE - m->FL);
	// Cursor must be erased whatever comes next ...
	lcdg_ec (0, a, 1);
	// ... and advanced
	m->SE++;

	if (a < m->Height - 1) {
		// Just advance the cursor
		lcdg_wl (CCHAR, 0, 0, a+1);
		return;
	}

	// At the bottom (and there are more lines)

	// Previous FL
	d = m->FL;
	// Advance
	m->FL += m->Height;
	if (m->FL + m->Height > m->NL)
		// Less than a window-full of lines left
		m->FL = m->NL - m->Height;

	// The actual advancement
	d = m->FL - d;

	for (i = 0; i < m->Height; i++) {
		// New line index
		j = i + m->FL;
		lcdg_wl (m->Lines [j], m->SH, 1, i);
		// Check how much to clean after the line
		if ((a = dm_mell (m, j)) >= m->Width)
			// No need to clear
			continue;
		if (a >= (b = dm_mell (m, j-d)))
			// No need to clear
			continue;
		// Number of characters to clear
		b -= a;
		lcdg_ec (a, i, b);
	}

	// Display the cursor
	lcdg_wl (CCHAR, 0, 0, m->SE - m->FL);
}

void lcdg_dm_menu_u (lcdg_dm_men_t *m) {
//
// Select next item up the list
//
	word i, j, d;
	byte a, b;

	if (m->SE == 0)
		// Cursor at the first line
		return;

	// Relative to the window
	a = (byte) (m->SE - m->FL);
	// Cursor must be erased whatever comes next ...
	lcdg_ec (0, a, 1);
	// ... and advanced
	m->SE--;

	if (a > 0) {
		// Just advance the cursor
		lcdg_wl (CCHAR, 0, 0, a-1);
		return;
	}

	// At the top

	// Previous FL
	d = m->FL;
	// Advance
	if (m->FL < m->Height)
		m->FL = 0;
	else
		m->FL -= m->Height;
	d -= m->FL;

	for (i = 0; i < m->Height; i++) {
		// New line index
		j = i + m->FL;
		lcdg_wl (m->Lines [j], m->SH, 1, i);
		// Check how much to clean after the line
		if ((a = dm_mell (m, j)) >= m->Width)
			// No need to clear
			continue;
		if (a >= (b = dm_mell (m, j+d)))
			// No need to clear
			continue;
		// Number of characters to clear
		b -= a;
		lcdg_ec (a, i, b);
	}

	// Display the cursor
	lcdg_wl (CCHAR, 0, 0, m->SE - m->FL);
}

void lcdg_dm_menu_l (lcdg_dm_men_t *m) {
//
// Scroll left
//
	word i, j, w, sl;

	// Calculate the maximum line length over displayed lines
	for (w = i = 0; i < m->Height; i++) {
		j = i + m->FL;
		if (j >= m->NL)
			break;
		if ((sl = strlen (m->Lines [j])) > w)
			w = sl;
	}
		
	if (m->SH + m->Width - 1 >= w)
		// No need to scroll any further
		return;
	m->SH++;

	for (i = 0; i < m->Height; i++) {
		j = i + m->FL;
		if (j >= m->NL)
			return;
		lcdg_wl (m->Lines [j], m->SH, 1, i);
		// Should a character be erased
		j = dm_mell (m, j);
		if (j < m->Width)
			lcdg_ec (j, i, 1);
	}
}

void lcdg_dm_menu_r (lcdg_dm_men_t *m) {
//
// Scroll right
//
	word i, j;

	if (m->SH == 0)
		return;
	m->SH--;

	for (i = 0; i < m->Height; i++) {
		j = i + m->FL;
		if (j >= m->NL)
			return;
		lcdg_wl (m->Lines [j], m->SH, 1, i);
	}
}

static byte dm_dtext (lcdg_dm_tex_t *dt) {
//
// Display a text
//
	word nl, sh;

	if (lcdg_font (dt->Font))
		// No way to set font
		return (LCDG_DM_STATUS = LCDG_DMERR_NOFONT);

	lcdg_setc (dt->BG, dt->FG);
	lcdg_set (dt->XL, dt->YL, dt->XH, dt->YH);

	nl = sh = 0;
	while (1) {
		lcdg_wl (dt->Line, sh, 0, nl);
		if (++nl == dt->Height)
			break;
		sh += dt->Width;
	}

	// The residual of last line
	if ((sh = strlen (dt->Line + sh)) < dt->Width)
		lcdg_ec (sh, nl - 1, dt->Width - sh);

	return (LCDG_DM_STATUS = 0);
}

byte lcdg_dm_display (lcdg_dm_obj_t *o) {
//
// Display an object
//
	switch (o->Type) {

		case LCDG_DMTYPE_IMAGE:

			return dm_dimage (((lcdg_dm_img_t*)o));

		case LCDG_DMTYPE_MENU:

			return dm_dmenu (((lcdg_dm_men_t*)o));

		case LCDG_DMTYPE_TEXT:

			return dm_dtext (((lcdg_dm_tex_t*)o));

	}
	// This cannot happen
	return 0;
}

byte lcdg_dm_dtop () {
//
// Re-display the top object
//
	if (LCDG_DM_TOP == NULL) {
		dm_clear ();
		return 0;
	}

	return lcdg_dm_display (LCDG_DM_TOP);
}

lcdg_dm_obj_t *lcdg_dm_remove (lcdg_dm_obj_t *o) {
//
// Delete the object
//
	lcdg_dm_obj_t *c;

	if (o == NULL)
		// Delete top object by default
		o = LCDG_DM_TOP;

	if (o == NULL)
		// No objects
		return NULL;

	// Find on the list
	for (c = LCDG_DM_HEAD; c != NULL; c = c->next)
		if (c == o)
			break;

	if (!lcdg_dm_shown (o))
		// Not on the list: do nothing, just return the object
		return o;

	c = dm_prev (o);
	if (c != NULL)
		c->next = o->next;
	else
		LCDG_DM_HEAD = o->next;
		
	if (o == LCDG_DM_TOP) {
		// Deleting the top object
		LCDG_DM_TOP = c;
		lcdg_dm_dtop ();
	}

	return o;
}

byte lcdg_dm_refresh () {
//
// Redisplay all objects on the list
//
	lcdg_dm_obj_t *c, *d;
	byte st;

	// Check if should clear the screen
	for (c = LCDG_DM_HEAD; c != NULL; c = c->next) {
		if (c->XL == 0 && c->YL == 0 && c->XH == LCDG_MAXX &&
		    c->YH == LCDG_MAXY)
			break;
	}

	if (c == NULL)
		dm_clear ();

	st = 0;
	for (c = LCDG_DM_HEAD; c != NULL; c = c->next) {
		// Check if covered by a next object
		for (d = c->next; d != NULL; d = d->next)
			if (dm_covered (c, d))
				break;
		if (d == NULL) {
			if ((st = lcdg_dm_display (c)) != 0)
				return st;
		}
	}
	return 0;
}

byte lcdg_dm_newtop (lcdg_dm_obj_t *o) {
//
// Display a new object as top
//
	byte st;

	if ((st = lcdg_dm_display (o)) != 0)
		// Error
		return st;

	if (o == LCDG_DM_TOP)
		return 0;

	// In case this is an object already on the list
	lcdg_dm_remove (o);

	// Make it new top
	o->next = NULL;
	if (LCDG_DM_TOP == NULL)
		LCDG_DM_HEAD = LCDG_DM_TOP = o;
	else {
		LCDG_DM_TOP->next = o;
		LCDG_DM_TOP = o;
	}

	return 0;
}

lcdg_dm_obj_t *lcdg_dm_newmenu (
			char **ls,		// The array of lines
			word nl,		// The number of lines
			byte fn,		// Font number
			byte bg,		// Background color
			byte fg,		// Foreground color
			byte x,			// Coordinates: left top corner
			byte y,
			byte w,			// Width (characters)
			byte h			// Height (characters)
									) {
//
// Create a new menu
//
	lcdg_dm_men_t *dm;

	if (lcdg_font (fn)) {
		// No font
		LCDG_DM_STATUS = LCDG_DMERR_NOFONT;
		return NULL;
	}

	if (lcdg_sett (x, y, w, h)) {
		// Bounding rectangle out of screen
		LCDG_DM_STATUS = LCDG_DMERR_RECT;
		return NULL;
	}

	dm = (lcdg_dm_men_t*) umalloc (sizeof (lcdg_dm_men_t));
	if (dm == NULL) {
		LCDG_DM_STATUS = LCDG_DMERR_NOMEM;
		return NULL;
	}

	dm->Font = fn;
	dm->BG = bg;
	dm->FG = fg;
	dm->Lines = ls;
	dm->NL = nl;
	dm->XL = x;
	dm->YL = y;
	dm->Width = w;
	dm->Height = h;
	dm->FL = dm->SH = dm->SE = 0;
	dm->Type = LCDG_DMTYPE_MENU;

	lcdg_get (NULL, NULL, &(dm->XH), &(dm->YH));
	LCDG_DM_STATUS = 0;
	return (lcdg_dm_obj_t*) dm;
}

char **lcdg_dm_asa (word n) {
//
// Allocates an array of strings
//
	char **res;

	if ((res = (char**) umalloc (sizeof (char*) * n)) == NULL) {
		// No memory
		LCDG_DM_STATUS = LCDG_DMERR_NOMEM;
		return NULL;
	}

	while (n)
		// This is needed in case we have an emergency cleanup
		res [--n] = NULL;

	return res;
}

void lcdg_dm_csa (char **lines, word n) {
//
// Cleans up a string array
//
	while (n) {
		n--;
		if (lines [n] != NULL)
			ufree (lines [n]);
	}

	ufree (lines);
}

lcdg_dm_obj_t *lcdg_dm_newtext (char *ln, byte fn, byte bg, byte fg,
						byte x, byte y, byte w) {
//
// Create a text object
//
	lcdg_dm_tex_t *dt;
	word nc, h;

	if (lcdg_font (fn)) {
		// No font
		LCDG_DM_STATUS = LCDG_DMERR_NOFONT;
		return NULL;
	}

	// Calculate the height
	nc = strlen (ln);
	for (h = 1; nc > w; nc -= w, h++);

	// Set the bounding rectangle
	if (lcdg_sett (x, y, w, h)) {
		// Bounding rectangle out of screen
		LCDG_DM_STATUS = LCDG_DMERR_RECT;
		return NULL;
	}

	dt = (lcdg_dm_tex_t*) umalloc (sizeof (lcdg_dm_tex_t));
	if (dt == NULL) {
		LCDG_DM_STATUS = LCDG_DMERR_NOMEM;
		return NULL;
	}

	dt->Font = fn;
	dt->BG = bg;
	dt->FG = fg;
	dt->XL = x;
	dt->YL = y;
	dt->Width = w;
	dt->Height = h;
	dt->Type = LCDG_DMTYPE_TEXT;
	dt->Line = ln;

	lcdg_get (NULL, NULL, &(dt->XH), &(dt->YH));
	LCDG_DM_STATUS = 0;
	return (lcdg_dm_obj_t*) dt;
}

lcdg_dm_obj_t *lcdg_dm_newimage (word pn, byte x, byte y) {
//
// New image object: the first argument is the image handle (EEPROM page
// number)
//
	word cz;
	byte xh, yh;
	void *ptr;

#define	sig	((lcdg_im_hdr_t*)ptr)
#define	dip	((lcdg_dm_img_t*)ptr)

	if ((sig = (lcdg_im_hdr_t*) umalloc (sizeof (lcdg_im_hdr_t))) == NULL) {
Mem:
		LCDG_DM_STATUS = LCDG_DMERR_NOMEM;
		return NULL;
	}

	if ((LCDG_DM_STATUS = lcdg_im_hdr (pn, sig)) != 0) {
		ufree (sig);
		return NULL;
	}

	// Sanitize origin coordinates
	if ((cz = (word)x + sig->X) > LCDG_MAXX+1)
		x -= (byte)(cz - (LCDG_MAXX+1));
	if ((cz = (byte)y + sig->Y) > LCDG_MAXY+1)
		y -= (byte)(cz - (LCDG_MAXY+1));

	xh = x + sig->X - 1;
	yh = y + sig->Y - 1;

	ufree (sig);

	dip = (lcdg_dm_img_t*) umalloc (sizeof (lcdg_dm_img_t));
	if (dip == NULL)
		goto Mem;

	dip->XL = x;
	dip->YL = y;

	dip->XH = xh;
	dip->YH = yh;

	dip->Type = LCDG_DMTYPE_IMAGE;

	dip->EPointer = pn;

	LCDG_DM_STATUS = 0;
	return (lcdg_dm_obj_t*) dip;

#undef sig
#undef dip
}
