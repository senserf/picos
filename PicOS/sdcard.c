#include "kernel.h"
#include "sdcard.h"

#define	SD_DEBUG	0
#define	SD_CSD_DUMP	0

#define	SD_CMD_GOIDLE	(0x00 + 0x40)
#define SD_CMD_SOPCND	(0x01 + 0x40)
#define	SD_CMD_SNDCSD	(0x09 + 0x40)
#define	SD_CMD_BLKLEN	(0x10 + 0x40)
#define SD_CMD_RSBLCK	(0x11 + 0x40)
#define SD_CMD_WSBLCK	(0x18 + 0x40)
#define	SD_CMD_EFBLCK	(0x20 + 0x40)	// First block for erase
#define	SD_CMD_ELBLCK	(0x21 + 0x40)	// Last block for erase
#define	SD_CMD_ERASEB	(0x26 + 0x40)	// Erase blocks

#define SD_REP_IDLE	0x01

// This is our "logical" block size
#define	SD_BKSIZE	512
#define	SD_BNMASK	0xfffffe00
#define	SD_BOMASK	0x01ff

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

#if SD_USE_UART

// ============================================================================
// SPI mode ===================================================================
// ============================================================================

static byte get_byte () {

	// Send a dummy byte of zeros; we are the master so we have to
	// keep the clock ticking

	while (!sd_tx_ready);
	sd_put (0xff);
	while (!sd_rx_ready);
	return sd_get;
}

static void put_byte (byte b) {

	byte s;
	while (!sd_tx_ready);
	sd_put (b);
	while (!sd_rx_ready);
	s = sd_get;
}

#else

// ============================================================================
// Direct (pin) mode ==========================================================
// ============================================================================

static byte get_byte () {

	register int i;
	register byte b;

	for (b = 0, i = 0; i < 8; i++) {
		b <<= 1;
		sd_clkh;
		if (sd_inp)
			b |= 1;
		sd_clkl;
	}

	return b;
}

static void put_byte (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			sd_outh;
		else
			sd_outl;
		sd_clkh;
		sd_clkl;
		b <<= 1;
	}
	// This is the default state of DO
	sd_outh;
}

// ============================================================================
// ============================================================================
// ============================================================================

#endif

static byte sd_skn (word tm) {
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

static byte sd_skp (word n) {
//
// Skip n bytes
//
	byte r;
	while (n--)
		r = get_byte ();
	return r;
}

static byte sd_cmd (byte cmd, lword par) {
//
// Send a short-response command
//
#if SD_DEBUG
	byte rc;
#endif
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
#if SD_DEBUG
	rc = sd_skn (12);
	diag ("C = %x R %x", cmd, rc);
	return rc;
#else
	// Response
	return sd_skn (12);
#endif
}

// ============================================================================
#if	SD_CSD_DUMP

void static sd_outcsd () {

	int i;

	for (i = 0; i < 16; i += 2)
		diag ("%x <- %d-%d", ((word)sd_buf [i]) << 8 | sd_buf [i+1],
			127 - (i << 3), 112 - (i << 3));
}
#endif
// ============================================================================



static void sd_getsize () {
//
// Read card size
//
	lword s;
	word i, cnt, w;
	byte c;

	// Now for some black magic. I am not sure if this is going to work for
	// all possible card types. Apparently, there is no way to determine
	// the actual size, but to go through these incantations
	for (sd_siz = 0, i = 0; ; i++) {
		// Try to get two identical values in a row
		if (sd_cmd (SD_CMD_SNDCSD, 0) == 0) {
			if (sd_skn (32) == 0xff)
				goto Bad;
			// Read the CSD into the buffer
			for (w = 0; w < 17; w++)
				sd_buf [w] = get_byte ();
#if	SD_CSD_DUMP
			sd_outcsd ();
#endif
			// The sixth byte contains "block" size exponent ...
			if ((c = sd_buf [5] & 0xf) > 11)
				// Max block size is 2K
				goto Bad;

			// First multiplier
			s =  ((word)(sd_buf [6] & 0x3) << 10) |
			    (((word)(sd_buf [7])) << 2) |
			           ((sd_buf [8] & 0xc0) >> 6) + 1;


			// Second multiplier
			w = 4 << ( ((sd_buf  [9] & 0x3 ) << 1) |
				   ((sd_buf [10] & 0x80) >> 7) );

			// My understanding of the documentation is that this
			// "block" has nothing to do with the "sector/block"
			// size used to define the semantics of subsequent
			// operations, e.g., the erase sector size is always
			// 512 bytes

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
		if (i == 8192)
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
	put_byte (0xfe);

	for (wc = 0; wc < SD_BKSIZE; wc++)
		put_byte (sd_buf [wc]);

	// Dummy CRC for now
	put_byte (0xff); put_byte (0xff);

	MARK_CLEAN;

	for (wc = 0; wc != 0x7fff; wc++) {
		// Wait while busy
		if (get_byte () == 0xff)
			return 0;
	}
	return SDERR_TIME;
}
		
word sd_open () {

	word i;

	// This makes CS output for card detection; note that in principle we
	// can detect the card present by checking the 50K pullup on CS

	sd_bring_up;

	sd_bkn = LWNONE;

	if (sd_buf == NULL) {
		// Allocate buffer
		if ((sd_buf = (byte*) umalloc (SD_BKSIZE)) == NULL) {
			i = SDERR_NOMEM;
			goto ErrM;
		}
	}

	// Clock warmup with select up (a bit of black magic)
	sd_skp (32);

	// Select
	sd_start;

	if ((i = sd_sleep ()) != 0) {
Err:
		sd_stop;
		ufree (sd_buf);
		sd_buf = NULL;
ErrM:
		sd_bring_down;
		return i;
	}

	// Set block size (is it really needed?)
	if (sd_cmd (SD_CMD_BLKLEN, SD_BKSIZE)) {
		i = SDERR_NOBLK;
		goto Err;
	}

	sd_getsize ();
	sd_stop;

	if (sd_siz == 0) {
		i = SDERR_UNSUP;
		goto Err;
	}

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
		bo = ((word)offset) & SD_BOMASK;	// Block offset

		if ((ln = SD_BKSIZE - bo) > length)
			// To read in this turn
			ln = length;

		if (ba != sd_bkn) {

			// Not in current block
			if (ba >= sd_siz)
				return SDERR_RANGE;

			sd_start;

			if ((wc = sd_synk ()) != 0) {
ERet:
				if (IS_ACTIVE)
					DEACTIVATE;

				sd_stop;
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
				if (get_byte () == 0xfe)
					break;
				if (wc == 0x7fff) {
					wc = SDERR_TIME;
					goto ERet;
				}
			}

			// Cache the block
			for (wc = 0; wc < SD_BKSIZE; wc++)
				sd_buf [wc] = get_byte ();

			// Read (and ignore for now) the CRC
			get_byte (); get_byte ();

			// Most reads will be much shorter than 512 bytes
			DEACTIVATE;

			// Deselect
			sd_stop;

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
		bo = ((word)offset) & SD_BOMASK;	// Block offset
		if ((ln = SD_BKSIZE - bo) > length)
			// To read in this turn
			ln = length;

		if (ba != sd_bkn) {

			// Not in current block
			if (ba >= sd_siz)
				return SDERR_RANGE;

			sd_start;
			if ((wc = sd_synk ()) != 0) {
ERet:
				if (IS_ACTIVE)
					DEACTIVATE;
				sd_stop;
				return wc;
			}

#if SD_NO_REREAD_ON_NEW_BLOCK_WRITE

			if (bo == 0) {
				// New block - no reread, just zero it out
				sd_stop;
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
					if (get_byte () == 0xfe)
						break;
					if (wc == 0x7fff) {
						wc = SDERR_TIME;
						goto ERet;
					}
				}
				for (wc = 0; wc < SD_BKSIZE; wc++)
					sd_buf [wc] = get_byte ();

				// Read (and ignore for now) the CRC
				get_byte (); get_byte ();

				DEACTIVATE;

				// Deselect
				sd_stop;
			}
			sd_bkn = ba;
		}
		MARK_DIRTY;
            	memcpy (sd_buf + bo, buffer, ln);
		length -= ln;
		offset += ln;
		buffer += ln;
	}

	return 0;
}

word sd_erase (lword from, lword upto) {

	lword ba;
	word wc;

	if (upto == 0)
		// This is LWA+1
		upto = sd_siz;
	else
		upto++;

	if (upto > sd_siz) {
ERange:
		wc = SDERR_RANGE;
		goto ERet;
	}

	if (from >= upto)
		goto ERange;

	sd_start;

	if ((wc = sd_synk ()))
		// Empty the buffers
		goto ERet;

	// Invalidate the cache
	sd_bkn = LWNONE;

	// This would be the first block number
	ba = from & SD_BNMASK;
	if ((wc = ((word)from) & (word)SD_BOMASK))
		// ... if it weren't partial
		ba++;


	// Any full blocks at all?
	if (ba < (upto & SD_BNMASK)) {

		// OK, erase blocks ... from:
		MARK_ACTIVE;
		if (sd_cmd (SD_CMD_EFBLCK, ba)) {
ESBad:
			wc = SDERR_NOBLK;
			goto ERet;
		}

		// ... to:
		if (sd_cmd (SD_CMD_ELBLCK, (upto & SD_BNMASK) - 1))
			goto ESBad;

		// ... do it
		if (sd_cmd (SD_CMD_ERASEB, 0))
			goto ESBad;

		// Wait until done (perhaps we need a state for this); not
		// really, it works amazingly fast, even for the entire card
		while (get_byte () != 0xff);
		// Need a timeout !!!! Let's check how long it takes in the
		// worst case, probably eons! Nah, next to nothing (a short
		// timeout will do).
		// Hold on: some cards take more than others, e.g., while
		// Lexar erases itself completely in a fraction of a second,
		// Verbatim takes ca. 10 seconds.
	}

	if (IS_ACTIVE)
		DEACTIVATE;
	sd_stop;

	// Now the marginals (handled as write); they are just in case and
	// should be discouraged
	wc = 0;
	while ((((word)from) & SD_BOMASK)) {
		if ((wc = sd_write (from, (const byte*)(&wc), 1)))
			return wc;
		from++;
	}

	while ((((word)upto) & SD_BOMASK)) {
		if ((wc = sd_write (upto - 1, (const byte*)(&wc), 1)))
			return wc;
		upto--;
	}

	return 0;

ERet:
	if (IS_ACTIVE)
		DEACTIVATE;

	sd_stop;
	return wc;
}
	
void sd_close () {

	if (sd_buf != NULL) {
		sd_start;
		sd_synk ();
		sd_sleep ();
		sd_stop;
		ufree (sd_buf);
		sd_buf = NULL;
		MARK_CLEAN;
		sd_bkn = LWNONE;
	}
	sd_bring_down;
}

void sd_panic () {
//
// For emergency shutdown
//
	if (sd_buf != NULL) {
		sd_start;
		sd_synk ();
		sd_stop;
	}
}

void sd_idle () {
//
// Assume idle state (user level)
//
	// Clock warmup (a bit of black magic)
	sd_skp (32);

	// Select
	sd_start;

	sd_sleep ();

	sd_stop;
}

word sd_sync () {
//
// Flush the cached buffer (user level)
//
	word wc;

	if (sd_buf == NULL)
		return 0;

	sd_start;
	wc = sd_synk ();
	if (IS_ACTIVE)
		DEACTIVATE;
	sd_stop;

	return wc;
}

#ifdef	RESET_ON_KEY_PRESSED

void sd_init_erase () {

	if (sd_open () == 0)
		sd_erase (0, 0);
	sd_close ();
}

#endif
