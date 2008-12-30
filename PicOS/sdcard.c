#include "kernel.h"
#include "sdcard.h"

#define	SD_CMD_GOIDLE	(0x00 + 0x40)
#define SD_CMD_SOPCND	(0x01 + 0x40)
#define	SD_CMD_SNDCSD	(0x09 + 0x40)
#define	SD_CMD_BLKLEN	(0x10 + 0x40)
#define SD_CMD_RSBLCK	(0x11 + 0x40)
#define SD_CMD_WSBLCK	(0x18 + 0x40)

#define SD_REP_IDLE	0x01

#define	SD_BKSIZE	512
#define	SD_BNMASK	0xfffffe00
#define	SD_BOMASK	0x000001ff

#ifndef	SD_NO_REREAD_ON_NEW_BLOCK_WRITE
#define	SD_NO_REREAD_ON_NEW_BLOCK_WRITE	0
#endif

#ifndef	SD_KEEP_IDLE
#define	SD_KEEP_IDLE	0
#endif

static byte *sd_buf = NULL;	// Buffer pointer
static byte sd_bdf = NO;	// Flags

#define	MARK_DIRTY		sd_bdf = YES
#define	MARK_CLEAN		sd_bdf = NO
#define	IS_DIRTY		sd_bdf
#define	IS_CLEAN		(sd_bdf == NO)

#if	SD_KEEP_IDLE
static byte sd_act = NO;
#define	MARK_ACTIVE		sd_act = YES
#define	MARK_IDLE		sd_act = NO
#define	IS_ACTIVE		sd_act
#define	IS_IDLE			(sd_act == NO)
#define	DEACTIVATE		sd_sleep ()
#else
#define	MARK_ACTIVE		CNOP
#define	MARK_IDLE		CNOP
#define	IS_ACTIVE		NO
#define	IS_IDLE			YES
#define	DEACTIVATE		CNOP
#endif

static lword sd_bkn = LWNONE;	// Block number stored in the buffer
static lword sd_siz;		// Size

static void sd_put (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			sd_doh;
		else
			sd_dol;
		sd_clkh;
		sd_clkl;
		b <<= 1;
	}
	// This is the default state of DO
	sd_doh;
}

static byte sd_get () {

	register int i;
	register byte b;

	for (b = 0, i = 0; i < 8; i++) {
		b <<= 1;
		sd_clkh;
		if (sd_di)
			b |= 1;
		sd_clkl;
	}

	return b;
}

static byte sd_skn (word tm) {
//
// Skip NULL bytes
//
	word i;
	byte r;

	for (i = 0; i < tm; i++)
		if ((r = sd_get ()) != 0xff)
			break;

	return r;
}

static byte sd_skp (word n) {
//
// Skip n bytes
//
	byte r;
	while (n--)
		r = sd_get ();
	return r;
}

static byte sd_cmd (byte cmd, lword par) {
//
// Send a short-response command
//
	// A delay to "warm up" the clock
	sd_put (0xff);
	// sd_put (0xff);
	// sd_put (0xff);

	// cmd has 0x40 or'red into it
	sd_put (cmd);
	sd_put ((byte)(par >> 24));
	sd_put ((byte)(par >> 16));
	sd_put ((byte)(par >>  8));
	sd_put ((byte)(par      ));
	// There is one command that always requires a checksum; luckily it
	// is static
	if (cmd == SD_CMD_GOIDLE)
		sd_put (0x95);

	// Response
	return sd_skn (12);
}

static void sd_getsize () {
//
// Read card size
//
	lword s;
	word i, cnt, w;
	byte b, c;

	// Now for some black magic. I am not sure if this is going to work for
	// all possible card types. Apparently, there is no way to determine
	// the actual size, but to go through these incantations
	for (sd_siz = 0, i = 0; ; i++) {
		// Try to get two identical values in a row
		if ((b = sd_cmd (SD_CMD_SNDCSD, 0)) == 0) {
			if (sd_skn (32) == 0xff)
				goto Bad;
			// The sixth byte contains "block" size exponent ...
			c = sd_skp (6);
			// ... here it is
			c &= 0xf;

			w = (word) (sd_get () & 0x3) << 10;
			w |= ((word) sd_get ()) << 2;
			w |= (sd_get () & 0xc0) >> 6;
			// First multiplier
			w++;
			s = w;

			// The second multiplier
			b = (sd_get () & 0x3) << 1;
			b |= (sd_get () & 0x80) >> 7;
			w = 4 << b;

			// Get the remaining 4 bytes + CRC
			sd_skp (6);

			if (c > 11)
				// Max block size is 2K
				goto Bad;

			s *= w;

			// Multiply by block size
			s <<= c;

			if (sd_siz == 0 || s != sd_siz)
				sd_siz = s;
			else
				break;
		}
Bad:
		if (i == 16) {
			sd_siz = 0;
			return;
		}
	}

	// Truncate to blocks
	sd_siz &= SD_BNMASK;
}

static word sd_sleep () {
//
// Assume idle state
//
	word i;

	for (i = 0; ; i++) {

		if (sd_cmd (SD_CMD_GOIDLE, 0) == SD_REP_IDLE)
			break;

		if (i == 128)
			return SDERR_NOCARD;
	}

	MARK_IDLE;

	for (i = 0; ; i++) {
		if ((sd_cmd (SD_CMD_SOPCND, 0) & SD_REP_IDLE) == 0)
			break;
		if (i == 1024)
			return SDERR_TIME;
	}
	return 0;
}

static word sd_synk () {
//
// Flush the cached buffer (internal)
//
	word wc;

	if (IS_CLEAN)
		// Current buffer not dirty
		return 0;

	MARK_ACTIVE;

	if (sd_cmd (SD_CMD_WSBLCK, sd_bkn))
		// Failed
		return SDERR_NOBLK;

	// Start byte
	sd_put (0xfe);

	for (wc = 0; wc < SD_BKSIZE; wc++)
		sd_put (sd_buf [wc]);

	// Dummy CRC for now
	sd_put (0xff); sd_put (0xff);

	MARK_CLEAN;

	for (wc = 0; wc != 0x7fff; wc++) {
		// Wait while busy
		if (sd_get () == 0xff)
			return 0;
	}
	return SDERR_TIME;
}
		
word sd_open () {

	word i;

	// This makes CS output for card detection; note that in principle we
	// can detect the card present by checking the 50K pullup on CS
	// sd_ini_regs;

	sd_bkn = LWNONE;

	if (sd_buf == NULL) {
		// Allocate buffer
		if ((sd_buf = (byte*) umalloc (SD_BKSIZE)) == NULL)
			return SDERR_NOMEM;
	}

	// Clock warmup with select up (a bit of black magic)
	sd_skp (32);

	// Select
	sd_csl;

	if ((i = sd_sleep ()) != 0) {
Err:
		sd_csh;
		ufree (sd_buf);
		sd_buf = NULL;
		return i;
	}

	// Set block size (is it really needed?)
	if (sd_cmd (SD_CMD_BLKLEN, SD_BKSIZE)) {
		i = SDERR_NOBLK;
		goto Err;
	}

	sd_getsize ();
	sd_csh;

	if (sd_siz == 0)
		return SDERR_UNSUP;

	return 0;
}

lword sd_size () { return sd_siz; }

word sd_read (lword offset, byte *buffer, word length) {

	lword ba;
	word wc, ln, bo;

	if (sd_buf == NULL)
		return SDERR_NOCARD;

	while (length) {

		ba = offset & SD_BNMASK;		// Block address
		bo = (word) (offset & SD_BOMASK);	// Block offset

		if ((ln = SD_BKSIZE - bo) > length)
			// To read in this turn
			ln = length;

		if (ba != sd_bkn) {

			// Not in current block
			if (ba >= sd_siz)
				return SDERR_RANGE;

			sd_csl;

			if ((wc = sd_synk ()) != 0) {
ERet:
				if (IS_ACTIVE)
					DEACTIVATE;

				sd_csh;
				return wc;
			}

			// Reading moves the card to active state
			MARK_ACTIVE;

			if (sd_cmd (SD_CMD_RSBLCK, ba)) {
				wc = SDERR_NOBLK;
				goto ERet;
			}

			// Wait for the block
			for (wc = 0; ; wc++) {
				if (sd_get () == 0xfe)
					break;
				if (wc == 0x7fff) {
					wc = SDERR_TIME;
					goto ERet;
				}
			}

			// Cache the block
			for (wc = 0; wc < SD_BKSIZE; wc++)
				sd_buf [wc] = sd_get ();

			// Read (and ignore for now) the CRC
			sd_get (); sd_get ();

			// Most reads will be much shorter than 512 bytes
			DEACTIVATE;

			// Deselect
			sd_csh;

			sd_bkn = ba;
			MARK_CLEAN;
		}

            	memcpy (buffer, sd_buf + bo, ln);
		length -= ln;
		offset += ln;
		buffer += ln;
	}
	return 0;
}

word sd_write (lword offset, const byte *buffer, word length) {

	word wc, ln, bo;
	lword ba;

	if (sd_buf == NULL)
		return SDERR_NOCARD;

	while (length) {

		ba = offset & SD_BNMASK;		// Block address
		bo = (word) (offset & SD_BOMASK);	// Block offset
		if ((ln = SD_BKSIZE - bo) > length)
			// To read in this turn
			ln = length;

		if (ba != sd_bkn) {

			// Not in current block
			if (ba >= sd_siz)
				return SDERR_RANGE;

			sd_csl;
			if ((wc = sd_synk ()) != 0) {
ERet:
				if (IS_ACTIVE)
					DEACTIVATE;
				sd_csh;
				return wc;
			}

#if SD_NO_REREAD_ON_NEW_BLOCK_WRITE

			if (bo == 0) {
				// New block - no reread, just zero it out
				sd_csh;
				bzero (sd_buf, SD_BKSIZE);
			} else
#endif
			{
				// Read the block in
				MARK_ACTIVE;

				if (sd_cmd (SD_CMD_RSBLCK, ba)) {
					// Error
					wc = SDERR_NOBLK;
					goto ERet;
				}

				// Wait for the block
				for (wc = 0; ; wc++) {
					if (sd_get () == 0xfe)
						break;
					if (wc == 0x7fff) {
						wc = SDERR_TIME;
						goto ERet;
					}
				}
				for (wc = 0; wc < SD_BKSIZE; wc++)
					sd_buf [wc] = sd_get ();

				// Read (and ignore for now) the CRC
				sd_get (); sd_get ();

				DEACTIVATE;

				// Deselect
				sd_csh;
			}
			sd_bkn = ba;
			MARK_DIRTY;
		}

            	memcpy (sd_buf + bo, buffer, ln);
		length -= ln;
		offset += ln;
		buffer += ln;
	}

	return 0;
}

void sd_close () {

	if (sd_buf != NULL) {
		sd_synk ();
		sd_sleep ();
		ufree (sd_buf);
		sd_buf = NULL;
		MARK_CLEAN;
		sd_bkn = LWNONE;
	}
}

void sd_idle () {
//
// Assume idle state (user level)
//
	// Clock warmup (a bit of black magic)
	sd_skp (32);

	// Select
	sd_csl;

	sd_sleep ();

	sd_csh;
}

word sd_sync () {
//
// Flush the cached buffer (user level)
//
	word wc;

	if (sd_buf == NULL)
		return 0;

	sd_csl;
	wc = sd_synk ();
	if (IS_ACTIVE)
		DEACTIVATE;
	sd_csh;

	return wc;
}
