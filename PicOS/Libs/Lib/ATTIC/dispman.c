#include "sysio.h"
#include "dispman.h"
#include "storage.h"

//
// Display manager
//

#define	CCHAR	">"			// Menu pointer character

dobject_t	*DM_TOP = NULL,		// The top object displayed
		*DM_HEAD = NULL;	// The front of list

word		DM_STATUS = 0;		// Error status

#if 0
static dobject_t *CLIP = NULL;		// Clip object

static Boolean dm_clipout (dobject_t *o) {
//
// Check if the object is completely outside the clip area, i.e.,
// it doesn't have to be displayed at all
//
	return (	o -> XH < CLIP -> XL	||	// To the left
			o -> XL > CLIP -> XH	||	// To the right
			o -> YH < CLIP -> YL	||	// Below
			o -> YL > CLIP -> YH	  );	// Above
}

static Boolean dm_clipin (dobject_t *o) {
//
// Check if the object is completely inside the clip area, i.e.,
// nothing has to be checked as the object is being displayed
//
	return (
			o -> XL >= CLIP -> XL	&&
			o -> XH <= CLIP -> XH	&&
			o -> YL >= CLIP -> YL	&&
			o -> YH <= CLIP -> YH	  );
}
#endif	/* CLIP */

Boolean dm_displayed (const dobject_t *o) {
//
// Check if an object occurs on the list
//
	dobject_t *c;

	for (c = DM_HEAD; c != NULL; c = c->next)
		if (c == o)
			return YES;
	return NO;
}

static Boolean dm_lmatch (const char *lab, const char *str) {
//
// Substring within image label
//
	const char *s, *t;

	do {
		for (s = lab, t = str; *t != '\0'; t++, s++) {
			if (*s != *t)
				goto Next;
		}
		// Match
		return YES;
Next:
		if (*lab == '\0')
			return NO;
		lab++;
	} while (1);
}

static Boolean dm_covered (dobject_t *a, dobject_t *b) {
//
// Object a is covered by object b
//
	return (
	  a->XL >= b->XL && a->YL >= b->YL && a->XH <= b->XH && a->YH <= b->YH
	);
}

static dobject_t *dm_prev (dobject_t *q) {

	dobject_t *o;

	if (q == DM_HEAD)
		return NULL;

	for (o = DM_HEAD; o != NULL && o->next != q; o = o->next);
	return o;
}

static void dm_clear () {

	lcdg_set (0, 0, LCDG_MAXX, LCDG_MAXY);
	lcdg_setc (COLOR_BLACK, BNONE);
	lcdg_clear ();
}

static byte dm_mell (dmenu_t *m, word ln) {
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

static word dm_dimage (dimage_t *im) {
//
// Display image pointed to by the page address. Note: this assumes that all
// images are 12 bpp.
//
	lword ep;
	word pn, cw, cx, cy;
	img_sig_t *sig;

	if ((sig = (img_sig_t*) umalloc (sizeof (img_sig_t))) == NULL)
		// No memory
		return (DM_STATUS = DERR_MEM);

	ep = im->EPointer;

	ee_read (ep + IMG_PO_SIG, (byte*)sig, IMG_SIGSIZE);
	if (sig->np > IMG_MAXPPI) {
		// Something wrong with image format
FErr:
		ufree (sig);
		return (DM_STATUS = DERR_FMT);
	}

	if ((sig->x & 0x8000))
		// Make sure this is a 12 bpp image - just in case; we shouldn't
		// be using the 8 bpp format
		goto FErr;

	// Check if image parameters haven't changed
	if (sig->x != im->XH - im->XL + 1 ||
	    sig->y != im->YH - im->YL + 1   )
		goto FErr;

	// Set the area for rendering; note that the semantics of lcdg_set have
	// changed
	lcdg_set (im->XL, im->YL, im->XH, im->YH);

	// Initial offset on first page
	cw = 0;
	ep = pntopa (sig->ppt [pn = 0]);
	while (sig->nc) {
		if ((cw += CHUNKSIZE+2) > IMG_MAX_COFF) {
			pn++;
			ep = pntopa (sig->ppt [pn]);
			cw = 0;
		}
		// Render this chunk
		ee_read (ep + cw, (byte*)(&(sig->cn)), CHUNKSIZE + 2);
		// Starting pixel of the chunk
		cx = sig->cn * PIXPERCHK12;
		cy = cx / sig->x;
		cx = cx % sig->x;
#if 0
		if (CLIP) {
			// Clipping - do only those pixels that fall into the
			// rectangle
			dy = 0;
			dx = cx + PIXPERCHK12 - 1;
			while (dx >= sig->x) {
				dy++;
				dx -= sig->x;
			}
			// dy == y-span of the chunk
			// dx == last displayed x-coordinate (relative)
			st = cy + im->YL;
			if (st + dy < CLIP->YL || st > CLIP->YH)
				// y outside the rectangle
				goto Ignore;

			if (dy < 2) {
				// If we completely span a row, we must have
				// been clipped out earlier
				st = cx + im->XL;
				dx += im->XL;
				if (dy) {
					// Wrap around
					if (st > CLIP->XH && dx < CLIP->XL)
						goto Ignore;
				} else {
					if (dx < CLIP->XL || st > CLIP->XH)
						goto Ignore;
				}
			}
		}
#endif
		lcdg_render ((byte)cx, (byte)cy, sig->chunk, PIXPERCHK12);
Ignore:
		sig->nc--;
	}
	// Done
	ufree (sig);
	return (DM_STATUS = 0);
}

static word dm_dmenu (dmenu_t *dm) {
//
// Display a menu object
//
	word i, j, a;

	// This initialization is partially redundant the second and next time
	// around, but harmless
	if (lcdg_font (dm->Font))
		// No way to set font
		return (DM_STATUS = DERR_FNT);

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
	return (DM_STATUS = 0);
}

void dm_menu_d (dmenu_t *m) {
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

void dm_menu_u (dmenu_t *m) {
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

void dm_menu_l (dmenu_t *m) {
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

void dm_menu_r (dmenu_t *m) {
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

static word dm_dtext (dtext_t *dt) {
//
// Display a text
//
	word nl, sh;

	if (lcdg_font (dt->Font))
		// No way to set font
		return (DM_STATUS = DERR_FNT);

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

	return (DM_STATUS = 0);
}

word dm_dobject (dobject_t *o) {
//
// Display an object
//
	switch (o->Type) {

		case DOTYPE_IMAGE:

			return dm_dimage (((dimage_t*)o));

		case DOTYPE_MENU:

			return dm_dmenu (((dmenu_t*)o));

		case DOTYPE_TEXT:

			return dm_dtext (((dtext_t*)o));

	}
	// This cannot happen
	return 0;
}

word dm_dtop () {
//
// Re-display the top object
//
	if (DM_TOP == NULL) {
		dm_clear ();
		return 0;
	}

	return dm_dobject (DM_TOP);
}

dobject_t *dm_delete (dobject_t *o) {
//
// Delete the object
//
	dobject_t *c;

	if (o == NULL)
		// Delete top object by default
		o = DM_TOP;

	if (o == NULL)
		// No objects
		return NULL;

	// Find on the list
	for (c = DM_HEAD; c != NULL; c = c->next)
		if (c == o)
			break;

	if (!dm_displayed (o))
		// Not on the list: do nothing, just return the object
		return o;

	c = dm_prev (o);
	if (c != NULL)
		c->next = o->next;
	else
		DM_HEAD = o->next;
		
	if (o == DM_TOP) {
		// Deleting the top object
		DM_TOP = c;
		dm_dtop ();
	}

	return o;
}

word dm_refresh () {
//
// Redisplay all objects on the list
//
	dobject_t *c, *d;
	word st;

	// Check if should clear the screen
	for (c = DM_HEAD; c != NULL; c = c->next) {
		if (c->XL == 0 && c->YL == 0 && c->XH == LCDG_MAXX &&
		    c->YH == LCDG_MAXY)
			break;
	}

	if (c == NULL)
		dm_clear ();

	st = 0;
	for (c = DM_HEAD; c != NULL; c = c->next) {
		// Check if covered by a next object
		for (d = c->next; d != NULL; d = d->next)
			if (dm_covered (c, d))
				break;
		if (d == NULL) {
			if ((st = dm_dobject (c)) != 0)
				return st;
		}
	}
	return 0;
}

word dm_top (dobject_t *o) {
//
// Display a new object as top
//
	dobject_t *c;
	word st;

	if ((st = dm_dobject (o)) != 0)
		// Error
		return st;

	if (o == DM_TOP)
		return 0;

	// In case this is an object already on the list
	dm_delete (o);

	// Make it new top
	o->next = NULL;
	if (DM_TOP == NULL)
		DM_HEAD = DM_TOP = o;
	else {
		DM_TOP->next = o;
		DM_TOP = o;
	}

	return 0;
}

dobject_t *dm_newmenu (
			const char **ls,	// The array of lines
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
	dmenu_t *dm;
	word i;

	if (lcdg_font (fn)) {
		// No font
		DM_STATUS = DERR_FNT;
		return NULL;
	}

	if (lcdg_sett (x, y, w, h)) {
		// Bounding rectangle out of screen
		DM_STATUS = DERR_REC;
		return NULL;
	}

	dm = (dmenu_t*) umalloc (sizeof (dmenu_t));
	if (dm == NULL) {
		DM_STATUS = DERR_MEM;
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
	dm->Type = DOTYPE_MENU;

	lcdg_get (NULL, NULL, &(dm->XH), &(dm->YH));
	DM_STATUS = 0;
	return (dobject_t*) dm;
}

dobject_t *dm_newtext (const char *ln, byte fn, byte bg, byte fg,
						byte x, byte y, byte w) {
//
// Create a text object
//
	dtext_t *dt;
	word nc, h;

	if (lcdg_font (fn)) {
		// No font
		DM_STATUS = DERR_FNT;
		return NULL;
	}

	// Calculate the height
	nc = strlen (ln);
	for (h = 1; nc > w; nc -= w, h++);

	// Set the bounding rectangle
	if (lcdg_sett (x, y, w, h)) {
		// Bounding rectangle out of screen
		DM_STATUS = DERR_REC;
		return NULL;
	}

	dt = (dtext_t*) umalloc (sizeof (dtext_t));
	if (dt == NULL) {
		DM_STATUS = DERR_MEM;
		return NULL;
	}

	dt->Font = fn;
	dt->BG = bg;
	dt->FG = fg;
	dt->XL = x;
	dt->YL = y;
	dt->Width = w;
	dt->Height = h;
	dt->Type = DOTYPE_TEXT;
	dt->Line = ln;

	lcdg_get (NULL, NULL, &(dt->XH), &(dt->YH));
	DM_STATUS = 0;
	return (dobject_t*) dt;
}

dobject_t *dm_newimage (const char *lab, byte x, byte y) {
//
// New image object
//
	lword ep;
	{
		word pn, np, cw;
		char *ilab;

		if ((ilab = (char*) umalloc (IMG_MAX_LABEL+1)) == NULL) {
			DM_STATUS = DERR_MEM;
			return NULL;
		}

		// Total number of pages
		np = (word)(ee_size (NULL, NULL) >> IMG_PAGE_SHIFT);

		for (pn = 0; pn < np; pn++) {
			ep = pntopa (pn);
			ee_read (ep + IMG_PO_MAGIC, (byte*)(ilab), 2);
			if (*((word*)ilab) != 0x7f00)
				continue;
			// First page of an image
			ee_read (ep + IMG_PO_LABEL, (byte*)ilab, IMG_MAX_LABEL);
			// Sentinel - just in case
			ilab [IMG_MAX_LABEL] = '\0';
			if (dm_lmatch (ilab, lab)) {
				// Found
				ufree (ilab);
				goto Found;
			}
		}

		// Not found
		ufree (ilab);
		DM_STATUS = DERR_NFN;
		return NULL;
	}
Found:
	{
		img_sig_t *sig;
		dimage_t *di;
		word SX, SY;

		if ((sig = (img_sig_t*) umalloc (sizeof (img_sig_t))) == NULL) {
Mem:
			DM_STATUS = DERR_MEM;
			return NULL;
		}
		ee_read (ep + IMG_PO_SIG, (byte*)sig, IMG_SIGSIZE);
		if (sig->x & 0x8000) {
			DM_STATUS = DERR_FMT;
Fail:
			ufree (sig);
			return NULL;
		}
		if ((SX = sig->x + x - 1) > LCDG_MAXX ||
		    (SY = sig->y + y - 1) > LCDG_MAXY    ) {
			DM_STATUS = DERR_REC;
			goto Fail;
		}
		// Looks OK
		ufree (sig);

		di = (dimage_t*) umalloc (sizeof (dimage_t));
		if (di == NULL)
			goto Mem;

		di->XL = x;
		di->YL = y;

		di->XH = (byte)SX;
		di->YH = (byte)SY;

		di->Type = DOTYPE_IMAGE;

		di->EPointer = ep;

		DM_STATUS = 0;
		return (dobject_t*) di;
	}
}
