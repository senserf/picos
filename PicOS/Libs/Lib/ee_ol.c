#include "sysio.h"
#include "ee_ol.h"

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// EEPROM object loader
//
// Object layout:
//			size [ bytes ] nrel [ rel ]
//                       w      b*      w      w*
//
// Each rel == word number (must be word-aligned) OR 0x8000, if the address
// is a string pointer. The relocated addresses and strings are verified to
// fall into the object.
//

byte ee_load (lword addr, void **op) {

	word nb, nr, rl, ta;
	address ap;

	if (ee_read (addr, (byte*)&nb, 2) || nb > 0x3fff || nb == 0)
		return EE_LOADERR_GARBAGE;

	if ((*op = (void*)(ap = umalloc (nb))) == NULL)
		return EE_LOADERR_NOMEM;

	// Read in the object
	addr += 2;
	if (ee_read (addr, (byte*)ap, nb)) {
Garbage:
		ufree (*op);
		return EE_LOADERR_GARBAGE;
	}

	addr += nb;

	// This is followed by the list of relocation points
	if (ee_read (addr, (byte*)&nr, 2) || nr > (nb >> 1))
		goto Garbage;

	while (nr--) {
		addr += 2;
		if (ee_read (addr, (byte*)(&rl), 2))
			goto Garbage;

		if ((ta = rl & 0x7fff) >= (nb >> 1))
			goto Garbage;

		// Relocate
		ta = ap [ta] += (word) ap;

		// Check if the relocated address is in range
		if (ta < (word) ap || ta >= (word) ap + nb)
			goto Garbage;

		if ((rl & 0x8000) != 0) {
			// Validate string
			rl = (word) ap + nb;
			while ( *((char*)ta) != '\0' ) {
				if (++ta >= rl)
					goto Garbage;
			}
		}
	}
	return 0;
}
