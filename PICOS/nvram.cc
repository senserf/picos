#ifndef __nvram_c__
#define __nvram_c__

#include "nvram.h"

#if 0
void NVRAM::dump () {

	lword cur;

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

NVRAM::NVRAM (lword size, lword psize, FLAGS tp, double *bounds) {

	lword k;

	tsize = size;
	if (psize) {
		// Make sure this is a power of 2
		for (k = 1; k < psize; k *= 2);
		if (k != psize)
			excptn ("NVRAM: psize must be a power of 2");
		// Turn it into a mask
		psize = ~(psize - 1);
	}

	pmask = psize;

	chunks = NULL;
	asize = esize = 0;

	TP = tp & ~NVRAM_FLAG_WEHANG;

	ftimes = NULL;

	if (bounds != NULL) {

		for (k = 0; k < EP_N_BOUNDS; k++)
			if (bounds [k] != 0.0)
				break;

		if (k < EP_N_BOUNDS) {
			// There are bounds
			ftimes = new nvram_timing_t;
			ftimes->UnHang = TIME_0;
			memcpy (ftimes->Bounds, bounds,
				EP_N_BOUNDS * sizeof (double));
		}
	}
};

NVRAM::~NVRAM () {

	erase (WNONE, 0, 0);
	if (ftimes)
		delete ftimes;
};

void NVRAM::merge (byte *target, const byte *source, lword length) {

	if ((TP & NVRAM_TYPE_NOOVER)) {
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
		esize = asize + 8;
		chunks = (nvram_chunk_t*) ((chunks == NULL) ?
		  malloc (esize * sizeof (nvram_chunk_t)) :
		    realloc (chunks, esize * sizeof (nvram_chunk_t)));
	}
}


word NVRAM::get (lword adr, byte *buf, lword len) {

	lword cur, n;
	int off;

	cur = 0;

	if (adr >= tsize || adr + len > tsize)
		return 1;

	if (len == 0)
		return 0;

	TP &= ~NVRAM_FLAG_UNSNCD;

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
	return 0;
}

word NVRAM::put (word st, lword adr, const byte *buf, lword len) {

	TIME del;
	lword cur, nlm, olm, cus, nad, nle;
	byte *nc;

	if (adr >= tsize || adr + len > tsize)
		return 1;

	if (len == 0)
		return 0;

	TP |= NVRAM_FLAG_UNSNCD;

	if (st != WNONE && ftimes != NULL) {
		// This is a somewhat crude model of timing
		if ((TP & NVRAM_FLAG_WEHANG) == 0) {
			// Set up the timer
			del = etuToItu ((double) len *
			  dRndUniform (ftimes->Bounds [0], ftimes->Bounds [1]));
			if (del != TIME_0) {
				TP |= NVRAM_FLAG_WEHANG;
				ftimes -> UnHang = Time + del;
				Timer->wait (del, st);
				sleep;
			}
		} else {
			if (ftimes->UnHang <= Time) {
				TP &= ~NVRAM_FLAG_WEHANG;
			} else {
				Timer->wait (ftimes->UnHang - Time, st);
				sleep;
			}
		}
	}

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
		return 0;
	}

	if (nlm <= olm && adr >= chunks [cur] . start) {
		// Case 1: no need to grow or merge the previous segment
		merge (chunks [cur] . ptr + (adr - chunks [cur] . start),
			buf, len);
		return 0;
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
	return 0;
}

word NVRAM::erase (word st, lword adrf, lword len) {

	lword off, n, cur, cus;
	TIME del;
	byte *nc;

	// len is 'upto' at this point
	if (adrf >= tsize)
		return 1;
	if (len >= tsize || len == 0)
		len = tsize - 1;
	else if (len < adrf)
		return 1;

	if (pmask != 0 && (TP & NVRAM_TYPE_ERPAGE) != 0) {
		// Erase applies to pages
		adrf &=  pmask;
		len |= ~pmask;
	}

	// Turn this into actual length
	len = len - adrf + 1;

	if (st != WNONE && ftimes != NULL) {
		if ((TP & NVRAM_FLAG_WEHANG) == 0) {
			// Set up the timer
			del = etuToItu ((double) len *
			  dRndUniform (ftimes->Bounds [2], ftimes->Bounds [3]));
			if (del != TIME_0) {
				TP |= NVRAM_FLAG_WEHANG;
				ftimes -> UnHang = Time + del;
				Timer->wait (del, st);
				sleep;
			}
		} else {
			if (ftimes->UnHang <= Time) {
				TP &= ~NVRAM_FLAG_WEHANG;
			} else {
				Timer->wait (ftimes->UnHang - Time, st);
				sleep;
			}
		}
	}

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
	return 0;
}

word NVRAM::sync (word st) {

	TIME del;

	if (st == WNONE || ftimes == NULL || (TP & NVRAM_FLAG_UNSNCD) == 0)
		return 0;

	if ((TP & NVRAM_FLAG_WEHANG) == 0) {
		// Set up the timer
		del = etuToItu (dRndUniform (ftimes->Bounds [4],
			ftimes->Bounds [5]));
		if (del != TIME_0) {
			TP |= NVRAM_FLAG_WEHANG;
			ftimes -> UnHang = Time + del;
			Timer->wait (del, st);
			sleep;
		}
	} else {
		if (ftimes->UnHang <= Time) {
			TP &= ~NVRAM_FLAG_WEHANG;
		} else {
			Timer->wait (ftimes->UnHang - Time, st);
			sleep;
		}
	}

	return 0;
}
	
#endif
