#ifndef	__applib_h__
#define	__applib_h__

#include "lcdg_dispman.h"
#include "sealists.h"

//+++ "applib.c"

typedef struct {
	word	id;
	word	ts;
	word	gr :8; // :2 needed now
	word	st :8; // :4 needed
} nbh_t;

typedef struct {
	word	li 	:8; // :7 enough
	word	spare 	:8;
	nbh_t  *mm;
} nbh_menu_t;

typedef struct {
	char	*buf;
	word	 len; // len, rssi could be :8, but need addresses or doubles
	word	 rss;
} rf_rcv_t;

#define BEAC_FREQ	5

// 'what' in handle_nbh()
#define NBH_AUDIT	1
#define NBH_FIND	2
#define NBH_INIT	3
int handle_nbh (word what, word id);

#define AD_GOT		1
#define AD_RPLY		2
#define AD_CANC		3
int handle_ad (word what, word plod);
// hellish kludge - not sure about the advertisers
#define AD_OLSO_ID	4
#define AD_COMBAT_ID	5

void init_glo ();
void process_incoming ();

// updates c0 (status) or c1 (group) (never both)
#define ULSEL_C0	1
#define ULSEL_C1	2
void update_line (word, word, word, word);

// handshake status
#define HS_SILE		0
#define HS_BEAC		1
#define HS_ISND		2
#define HS_IRCV		3
#define HS_NOSY		4
#define HS_SYMM		5
#define HS_MATC		6
#define HS_DECL		7
#define HS_IDEC		8

// codes for the status char (menu line[0], after the cursor >)
#define MLI0_INIT	'.'
#define MLI0_INNH	'!'
#define MLI0_GONE	'x'
#define MLI0_NOSY	'0'
#define MLI0_SYMM	'2'
#define MLI0_ISND	'1'
#define MLI0_IRCV	'm'
#define MLI0_MATC	'M'
#define MLI0_DECL	'-'
#define MLI0_IDEC	'~'

// groups
#define GR_YE		0
#define GR_NO		1
#define GR_IY		2
#define GR_IN		3

// codes for the group char (menu line[1]), so far +/- with 'ignore'
#define MLI1_YE		'+'
#define MLI1_NO		'-'
#define MLI1_IY		'I'
#define MLI1_IN		'i'

// top_flag
#define TOP_HIER	0
#define TOP_DATA	1
#define TOP_AD		2

extern nbh_menu_t 	nbh_menu;
extern lcdg_dm_men_t   *lcd_menu;
extern rf_rcv_t		rf_rcv, ad_rcv;
extern word		top_flag;
extern char *		ad_buf;
extern sea_rec_t *	curr_rec;
#endif
