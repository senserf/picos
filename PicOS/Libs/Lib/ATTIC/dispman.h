#ifndef	__dispman_h__
#define	__dispman_h__

// ============================================================================
// ========================= Picture-related stuff ============================
// ============================================================================

//+++ "dispman.c"

#define	CHUNKSIZE	54

#define	IMG_PAGE_SIZE	8192
#define	IMG_PAGE_SHIFT	13
#define	IMG_PAGE_MASK	((1<<IMG_PAGE_SHIFT)-1)

#define	pntopa(a)	(((lword)(a)) << IMG_PAGE_SHIFT)

#define	IMG_MAXCHUNKS	720
#define	IMG_CHK_PAGE	146		// Chunks per page
#define	IMG_MAX		128
#define	IMG_MAXPPI	5		// Max pages per image (hard limit)
					// Max chunk offset
#define	IMG_MAX_COFF	((IMG_CHK_PAGE-1)*(CHUNKSIZE+2))

#define	PIXPERCHK8	CHUNKSIZE
#define	PIXPERCHK12	((CHUNKSIZE*8)/12)

// First chunk layout (in EEPROM) =============================================
//
// Bytes 0-1:		0x7f00
// Bytes 2-3:		Image number
//
// Bytes 4-5:           The total number of chunks in file
// Bytes 6-7:		The number of pages (including this one)
// Bytes 8-11:		GEOMETRY, two words (X and Y), X is or'red with 0x8000
//			if the image is 8bpp
// Bytes 12-21:		Page numbers of the remaining pages
// Bytes 22-55: 	The label
//
// ============================================================================

typedef	struct {

	word	nc;	// Number of chunks
	word	x;	// X
	word	y;	// Y
	word	np;	// Number of pages
	word	ppt [IMG_MAXPPI];

	// --- optional ---
	word	cn;
	byte	chunk [CHUNKSIZE];

} img_sig_t;

#define	IMG_SIGSIZE	((4 + IMG_MAXPPI) * 2)

#define	IMG_PO_MAGIC	0
#define	IMG_PO_NUMBER	2		// W number (WNONE == OWN)
#define	IMG_PO_SIG	4		// Signature
#define	IMG_PO_NCHUNKS	4
#define	IMG_PO_X	6
#define	IMG_PO_LABEL	22

#define	IMG_MAX_LABEL	(CHUNKSIZE+2-IMG_PO_LABEL)

// ============================================================================
// ========================= Display manager structures =======================
// ============================================================================

struct dobject_s;

#define	DERR_MEM	1
#define	DERR_FMT	2
#define	DERR_NFN	3
#define	DERR_FNT	4
#define	DERR_REC	5

#define	DOTYPE_IMAGE	0
#define	DOTYPE_MENU	1
#define	DOTYPE_TEXT	2

#define	DOBJECT_STUFF	\
	byte			XL, YL, XH, YH; \
	struct dobject_s	*next; \
	byte			Type

struct dobject_s {
//
// Displayable object: this is the minimum that every object must have
//
	DOBJECT_STUFF;

};

typedef	struct dobject_s dobject_t;

typedef struct {
//
// Image
//
	DOBJECT_STUFF;

	lword	EPointer;	// Pointer to EEPROM page

} dimage_t;

typedef struct {
//
// Menu
//
	DOBJECT_STUFF;

	byte Font;		// Font ID
	byte BG, FG;		// Colors
	const char **Lines;	// This must be allocated by the praxis
	word NL;		// Number of lines
	byte Width, Height;	// In characters
	word FL,		// First line displayed
	     SH,                // Shift (horizontal scroll)
	     SE;		// Selected line

} dmenu_t;

typedef struct {
//
// Static text
//
	DOBJECT_STUFF;

	byte Font;
	byte BG, FG;		// Colors
	const char *Line;	// This must be allocated by the praxis
	byte Width, Height;	// In characters
	

} dtext_t;

extern dobject_t	*DM_TOP, *DM_HEAD;
extern word		DM_STATUS;

Boolean dm_displayed (const dobject_t*);
void dm_menu_d (dmenu_t*);
void dm_menu_u (dmenu_t*);
void dm_menu_l (dmenu_t*);
void dm_menu_r (dmenu_t*);
word dm_dobject (dobject_t*);
word dm_dtop (void);
dobject_t *dm_delete (dobject_t*);
word dm_refresh (void);
word dm_top (dobject_t*);
dobject_t *dm_newmenu (const char**,
				word, byte, byte, byte, byte, byte, byte, byte);
dobject_t *dm_newtext (const char*, byte, byte, byte, byte, byte, byte);
dobject_t *dm_newimage (const char*, byte, byte);

#endif
