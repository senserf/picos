#ifndef __nvram_c__
#define __nvram_c__

#include "nvram.h"

#if 0
void NVRAM::dump () {

	word cur;

	trace ("NVRAM at %s, list of chunks [%1d,%1d,%1d]:",
		TheStation->getSName (), tsize, esize, asize);

	for (cur = 0; cur < asize; cur++) {
		trace ("    Start %1d [%4x], length %1d [%4x]",
			chunks [cur] . start,
			chunks [cur] . start,
			chunks [cur] . length,
			chunks [cur] . length);
	}
}
#endif

NVRAM::NVRAM (word size, word psize) {

	word k;

	tsize = size;
	if (psize) {
		// Make sure this is a power of 2
		for (k = 1; k < psize; k *= 2);
		if (k != psize)
			excptn ("NVRAM: psize must be a power of 2");
		psize = ~(psize - 1);
	}

	pmask = psize;

	chunks = NULL;
	asize = esize = 0;
};

NVRAM::~NVRAM () {

	erase ();
};

void NVRAM::merge (byte *target, const byte *source, word length) {

	if (pmask) {
		// This is flash: writing 1 to 0 is void
		while (length--)
			*target++ &= *source++;
	} else {
		// This is EEPROM
		memcpy (target, source, length);
	}
}

void NVRAM::grow () {

	if (esize <= asize) {
		// Must grow the array
		esize = asize + 4;
		chunks = (nvram_chunk_t*) ((chunks == NULL) ?
		  malloc (esize * sizeof (nvram_chunk_t)) :
		    realloc (chunks, esize * sizeof (nvram_chunk_t)));
	}
}


void NVRAM::get (word adr, byte *buf, word len) {

	word cur, n;
	int off;

	cur = 0;

	Assert ((long) len + adr <= tsize, "NVRAM->get: %d + %d out of range",
		adr, len);

	while (len && cur < asize) {

		if (chunks [cur] . start <= adr) {
			off = adr - chunks [cur] . start;
			if (off < chunks [cur] . length) {
				if ((n = chunks [cur] . length - off) > len)
					n = len;
				memcpy (buf, chunks [cur] . ptr + off, n);
				len -= n;
				adr += n;
				buf += n;
			}
			cur++;
		} else {
			if ((n = chunks [cur] . start - adr) > len)
				n = len;

			adr += n;
			len -= n;

			while (n--)
				*buf++ = 0xff;
		}
	}

	while (len--)
		*buf++ = 0xff;
#if 0
	dump ();
#endif
}

void NVRAM::put (word adr, const byte *buf, word len) {

	word cur, nlm, olm, cus, nad, nle;
	byte *nc;

	Assert ((long) len + adr <= tsize, "NVRAM->put: %d + %d out of range",
		adr, len);

	for (cur = 0; cur < asize; cur++) {

		if ((olm = chunks [cur] . start + chunks [cur] . length) >= adr)
			// Mergeable
			break;
	}

	nlm = adr + len;
		
	if (cur == asize || chunks [cur] . start > nlm) {
		// Case 0: a brand new chunk inserted here
		grow ();
		for (cus = asize; cus > cur; cus--)
			chunks [cus] = chunks [cus-1];
		asize++;
		chunks [cur] . start = adr;
		chunks [cur] . length = len;
		chunks [cur] . ptr = (byte*) malloc (len);

		memcpy (chunks [cur] . ptr, buf, len);
		return;
	}

	if (nlm <= olm && adr >= chunks [cur] . start) {
		// Case 1: no need to grow or merge the previous segment
		merge (chunks [cur] . ptr + (adr - chunks [cur] . start),
			buf, len);
		return;
	}

	// Case 2: we have at least to grow it and possibly merge other chunks
	if ((nad = chunks [cur] . start) > adr)
		// This is the starting address
		nad = adr;

	cus = cur;
	if (nlm > olm) {
		// Check if should merge with other chunks
		for (cus++; cus < asize; cus++) {
			if (nlm < chunks [cus] . start)
				break;
		}
		cus--;
		olm = chunks [cus] . start + chunks [cus] . length;
	}
	if (nlm < olm)
		// This is the extent of the new chunk (LWA+1)
		nlm = olm;

	// The length
	nle = nlm - nad;

	// New chunk
	nc = (byte*) malloc (nle);

	// Now copy the stuff
	get (nad, nc, nle);

	// Replace the old chunks
	chunks [cur] . start = nad;
	chunks [cur] . length = nle;
	free (chunks [cur] . ptr);
	chunks [cur] . ptr = nc;

	if (cur != cus) {
		nlm = cur + 1;
		// Remove merged entries
		for (olm = nlm; olm <= cus; olm++)
			free (chunks [olm] . ptr);

		for (olm = cus + 1; olm < asize; olm++)
			chunks [nlm++] = chunks [olm];

		asize = nlm;
	}

	merge (chunks [cur] . ptr + (adr - nad), buf, len);
}

void NVRAM::erase () {

	word cur;

	if (chunks != NULL) {
		for (cur = 0; cur < asize; cur++)
			free (chunks [cur] . ptr);

		free (chunks);
		chunks = NULL;
	}
	asize = esize = 0;
}

void NVRAM::erase (word adrf) {

	word len, off, n, cur, cus;
	byte *nc;

	Assert (adrf < tsize, "NVRAM->erase: %d out of range", adrf);

	if (pmask == 0)
		excptn ("NVRAM->erase (adr): page size is zero");

	adrf &= pmask;				// From
	len = (~pmask) + 1;			// Length

	cur = 0;
	while (len && cur < asize) {
		if (chunks [cur] . start <= adrf) {
			off = adrf - chunks [cur] . start;
			if (off < chunks [cur] . length) {
				n = chunks [cur] . length - off;
				if (off == 0) {
					// From the beginning
					if (n <= len) {
						// The entire chunk goes
						len -= n;
						free (chunks [cur] . ptr);
						chunks [cur] . ptr = NULL;
						adrf += n;
						cur++;
						continue;
					}
					// Something is left: truncate front
					n -= len;
					nc = (byte*) malloc (n);
					memcpy (nc, chunks [cur] . ptr + len,
						n);
					free (chunks [cur] . ptr);
					chunks [cur] . ptr = nc;
					chunks [cur] . start += len;
					chunks [cur] . length = n;
					break;
				}
				if (n <= len) {
					// Truncate at the end
					nc = (byte*) malloc (off);
					memcpy (nc, chunks [cur] . ptr, off);
					free (chunks [cur] . ptr);
					chunks [cur] . ptr = nc;
					chunks [cur] . length = off;
					len -= n;
					adrf += n;
					cur++;
					continue;
				}
				grow ();

				for (cus = asize-1; cus > cur; cus--)
					chunks [cus+1] = chunks [cus];

				asize++;

				nc = (byte*) malloc (off);
				memcpy (nc, chunks [cur] . ptr, off);
				chunks [cur] . length = off;

				n -= len;
				chunks [cur+1] . ptr = (byte*) malloc (n);
				memcpy (chunks [cur+1] . ptr,
					chunks [cur] . ptr + off + len, n);
				chunks [cur+1] . length = n;
				chunks [cur+1] . start = chunks [cur] . start +
					off + len;

				free (chunks [cur] . ptr);
				chunks [cur] . ptr = nc;
				break;
			}
			cur++;
		} else {
			if ((n = chunks [cur] . start - adrf) > len)
				n = len;

			len -= n;
			adrf += n;
		}
	}

	// Final cleanup: remove empty chunks
	for (cur = cus = 0; cur < asize; cur++)
		if (chunks [cur] . ptr != NULL)
			chunks [cus++] = chunks [cur];
	asize = cus;
}

#endif
