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

static byte *bbuf = NULL;	// Buffer pointer
static byte bbdi = NO;		// Dirty flag
static lword bblk = LWNONE;	// Block number stored in the buffer
static lword msize;		// Size

static void put_byte (byte b) {

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

static byte get_byte () {

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

static byte skip_null (word tm) {
//
// Skip NULL bytes
//
	word i;
	byte r;

	for (i = 0; i < tm; i++)
		if ((r = get_byte ()) != 0xff)
			break;

	return r;
}

static byte skip (word n) {
//
// Skip n bytes
//
	byte r;
	while (n--)
		r = get_byte ();
	return r;
}

static byte cmds (byte cmd, lword par) {
//
// Send a short-response command
//
	// A delay to "warm up" the clock
	put_byte (0xff);
	// put_byte (0xff);
	// put_byte (0xff);

	// cmd has 0x40 or'red into it
	put_byte (cmd);
	put_byte ((byte)(par >> 24));
	put_byte ((byte)(par >> 16));
	put_byte ((byte)(par >>  8));
	put_byte ((byte)(par      ));
	// There is one command that always requires a checksum; luckily it
	// is static
	if (cmd == SD_CMD_GOIDLE)
		put_byte (0x95);

	// Response
	return skip_null (12);
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
	for (msize = 0, i = 0; ; i++) {
		// Try to get two identical values in a row
		if ((b = cmds (SD_CMD_SNDCSD, 0)) == 0) {
			if (skip_null (32) == 0xff)
				goto Bad;
			// The sixth byte contains "block" size exponent ...
			c = skip (6);
			// ... here it is
			c &= 0xf;

			w = (word) (get_byte () & 0x3) << 10;
			w |= ((word) get_byte ()) << 2;
			w |= (get_byte () & 0xc0) >> 6;
			// First multiplier
			w++;
			s = w;

			// The second multiplier
			b = (get_byte () & 0x3) << 1;
			b |= (get_byte () & 0x80) >> 7;
			w = 4 << b;

			// Get the remaining 4 bytes + CRC
			skip (6);

			if (c > 11)
				// Max block size is 2K
				goto Bad;

			s *= w;

			// Multiply by block size
			s <<= c;

			if (msize == 0 || s != msize)
				msize = s;
			else
				break;
		}
Bad:
		if (i == 16) {
			msize = 0;
			return;
		}
	}

	// Truncate to blocks
	msize &= SD_BNMASK;
}
		
word sd_open () {

	word i;

	// This makes CS output for card detection; note that in principle we
	// can detect the card present by checking the 50K pullup on CS
	// sd_ini_regs;

	if (bbuf == NULL) {
		// Allocate buffer
		if ((bbuf = (byte*) umalloc (SD_BKSIZE)) == NULL)
			return SDERR_NOMEM;
	}

	bblk = LWNONE;

	// A bit of warmup (black magic required by some cards)
	skip (32);

	// Select
	sd_csl;

	for (i = 0; ; i++) {

		// Set the card to IDLE SPI mode
		if (cmds (SD_CMD_GOIDLE, 0) == SD_REP_IDLE)
			break;

		if (i == 128) {
			// No card
			i = SDERR_NOCARD;
Err:
			sd_csh;
			ufree (bbuf);
			bbuf = NULL;
			return i;
		}
	}

	for (i = 0; ; i++) {
		if ((cmds (SD_CMD_SOPCND, 0) & SD_REP_IDLE) == 0)
			break;
		if (i == 1024) {
			i = SDERR_TIME;
			goto Err;
		}
	}

	// Set block size (is it really needed?)
	if (cmds (SD_CMD_BLKLEN, SD_BKSIZE)) {
		i = SDERR_NOBLK;
		goto Err;
	}

	sd_getsize ();
	sd_csh;

	if (msize == 0)
		return SDERR_UNSUP;

	return 0;
}

lword sd_size () { return msize; }

word sd_read (lword offset, byte *buffer, word length) {

	word wc, ln, bo;
	lword ba;

	if (bbuf == NULL)
		return SDERR_NOCARD;

	while (length) {

		ba = offset & SD_BNMASK;		// Block address
		bo = (word) (offset & SD_BOMASK);	// Block offset
		if ((ln = SD_BKSIZE - bo) > length)
			// To read in this turn
			ln = length;

		if (ba != bblk) {
			// Not in current block
			if (ba >= msize)
				return SDERR_RANGE;
			if ((wc = sd_sync ()) != 0)
				return wc;

			sd_csl;

			if (cmds (SD_CMD_RSBLCK, ba)) {
				// Error
				sd_csh;
				return SDERR_NOBLK;
			}

			// Wait for the block (should we go through a state?)
			for (wc = 0; ; wc++) {
				if (get_byte () == 0xfe)
					break;
				if (wc == 0x7fff) {
					sd_csh;
					return SDERR_TIME;
				}
			}

			// Cache the block
			for (wc = 0; wc < SD_BKSIZE; wc++)
				bbuf [wc] = get_byte ();

			// Read (and ignore for now) the CRC
			get_byte (); get_byte ();

			// Deselect
			sd_csl;

			bblk = ba;
			bbdi = NO;
		}

            	memcpy (buffer, bbuf + bo, ln);
		length -= ln;
		offset += ln;
		buffer += ln;
	}

	return 0;
}

word sd_write (lword offset, const byte *buffer, word length) {

	word wc, ln, bo;
	lword ba;

	if (bbuf == NULL)
		return SDERR_NOCARD;

	while (length) {

		ba = offset & SD_BNMASK;		// Block address
		bo = (word) (offset & SD_BOMASK);	// Block offset
		if ((ln = SD_BKSIZE - bo) > length)
			// To read in this turn
			ln = length;

		if (ba != bblk) {

			// Not in current block
			if (ba >= msize)
				return SDERR_RANGE;
			if ((wc = sd_sync ()) != 0)
				return wc;

#if SD_NO_REREAD_ON_NEW_BLOCK_WRITE

			if (bo == 0) {
				// New block - no reread, just zero it out
				bzero (bbuf, SD_BKSIZE);
			} else
#endif
			{
				// Read the block in
				sd_csl;
				if (cmds (SD_CMD_RSBLCK, ba)) {
					// Error
					sd_csh;
					return SDERR_NOBLK;
				}

				// Wait for the block
				for (wc = 0; ; wc++) {
					if (get_byte () == 0xfe)
						break;
					if (wc == 0x7fff) {
						sd_csh;
						return SDERR_TIME;
					}
				}
				for (wc = 0; wc < SD_BKSIZE; wc++)
					bbuf [wc] = get_byte ();
				// Read (and ignore for now) the CRC
				get_byte (); get_byte ();
				// Deselect
				sd_csl;
			}
			bblk = ba;
			bbdi = YES;
		}

            	memcpy (bbuf + bo, buffer, ln);
		length -= ln;
		offset += ln;
		buffer += ln;
	}

	return 0;
}

word sd_sync () {
//
// Flush the cached buffer
//
	word wc;

	if (bbuf == NULL || bbdi == NO)
		return 0;

	sd_csl;

	if (cmds (SD_CMD_WSBLCK, bblk)) {
		// Failed
		sd_csh;
		return SDERR_NOBLK;
	}

	// Start byte
	put_byte (0xfe);

	for (wc = 0; wc < SD_BKSIZE; wc++)
		put_byte (bbuf [wc]);

	// Dummy CRC for now
	put_byte (0xff); put_byte (0xff);

	bbdi = NO;
	for (wc = 0; wc != 0x7fff; wc++) {
		// Wait while busy
		if (get_byte () == 0xff) {
			sd_csh;
			return 0;
		}
	}

	sd_csh;
	return SDERR_TIME;
}

void sd_close () {

	if (bbuf != NULL) {
		sd_sync ();
		ufree (bbuf);
		bbuf = NULL;
		bbdi = NO;
		bblk = LWNONE;
	}
}
