/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "ee_ol.h"
#include "storage.h"


//
// EEPROM object loader
//
// Object layout:
//			size [ bytes ] nrel [ rel ]
//                       w      b*      w      w*
//
//			rel & 0x3fff => byte number within the object
//			rel & 0xC000 =>
//					0 - address
//					1 - string address
//					2 - word
//					3 - lword
//					

#ifdef	__SMURPH__

//
// This is something truly messy, which we require for VUEE. Note that
// the whole concept of an EEPROM object loader is strongly coupled to
// the architecture of MSP430, assuming 16-bit pointers and little endian-
// ness. To make such objects appear right in the memory of some unknown
// host running this program, we have to do some creative repacking and
// then hope that thinks will turn out just fine.
//
// The address size on the host. We assume that addresses (+ endianness) are
// the only source of problems, i.e.,
//
//	word	takes two bytes (2-aligned in structs) on our host
//	lword	takes four bytes (4-aligned)
//	
// An address takes 4 or perhaps 8 bytes (determined by ADRS), which are
// respectively aligned in structs. The host can be big endian (in principle).
//
// Note that endianness issues are a big problem, and this "solution" is only
// an illustration that doesn't really solve anything. In a nuthsell, VUEE is
// not going to work on big endian machines if the praxis touches anything
// little endian (like preloaded EEPROM, or OSS running on little-endian
// machines).
//

#define	ADRS		(sizeof (void*))

// This is how to align an address (without using a mask)
#define	AALIGN(a)	do { \
				if ((a) % ADRS) \
					(a) += ADRS - ((a) % ADRS); \
			} while (0)

byte ee_load (lword addr, void **op) {
//
	lword	lwd;

	word	nb,	// Number of bytes in the source (original) object
		nr,	// Number of relocators associated with the object
		nd,	// Number of address adjustments
		de,	// Accumulated shift as we go through the object
		sp,	// Index into the source object
		tp,	// Index into target object
		rt,	// Relocation type
		np,
		ts,	// Target object size
		ns,	// Number of string pointers to verify
		ld,	// Index into the adjustment table
		wd,
		md,
		dx,
		da;

	byte	*ob,	// Image of the original object as read from EEPROM
		*oq,	// Pointer to the target object
		*vp;

	char	*cs,	// For verifying strings
		*es;

	word	*rel,	// Relocators
		*adj,	// Adjustements
		*del;	// Addresses (thresholds) for the adjustments

	int 	i;

	if (ee_read (addr, (byte*)&nb, 2) || (nb & 0xc000) || nb == 0)
		return EE_LOADERR_GARBAGE;

	// trace ("EEOL, object at %1d [%x], size %1d", addr, addr, nb);
	// Read the object into memory
	ob = new byte [nb];
	addr += 2;

	if (ee_read (addr, ob, nb)) {
Gar1:
		delete ob;
		// trace ("EEOL, garbage");
		return EE_LOADERR_GARBAGE;
	}

	addr += nb;

	// Read the relocation list
	if (ee_read (addr, (byte*)&nr, 2) || nr > (nb >> 1))
		goto Gar1;
	// trace ("EEOL, %1d relocation items", nr);

	if (nr == 0) {
		// There is nothing to relocate or repack, i.e., we only have
		// bytes
		if ((*op = (void*)(TheNode->memAlloc (nb, nb))) == NULL) {
			delete ob;
			return EE_LOADERR_NOMEM;
		}
		memcpy (*op, ob, nb);
		delete ob;
		return 0;
	}

	addr += 2;
		
	// Relocators
	rel = new word [nr];
	// The minimum absolutely safe size for the adjustments is 2 * nr
	del = new word [nd = nr + nr];
	// Adjustments
	adj = new word [nd];

	if (ee_read (addr, (byte*)rel, nd)) {
Gar2:
		delete rel;
		delete del;
		delete adj;
		goto Gar1;
	}

	// Endianness correction for the relocators

#if BYTE_ORDER != LITTLE_ENDIAN
	// This is baloney (if needed, this must be done in a whole lot of
	// other places)
	for (i = 0; i < nr; i++)
		re_endian_w (rel [i]);
#endif
	// for (i = 0; i < nr; i++)
		// trace ("RELOCATOR %1d = %04x", i, rel [i]);

	// The accumulated number of adjustments
	nd = 0;
	// Accumulated total adjustment
	de = 0;
	// Source pointer
	sp = 0;

	// Calculate adjustments and target object size
	for (i = 0; i < nr; i++) {

		// This assumes that the relocators occur in the increasing
		// order of locations within the object
		rt = rel [i] >> 14;
		// trace ("REL %1d, type %1d", i, rt);

		if (rt > 1)
			// For now, we only process addresses, assuming that
			// everything else is already properly aligned
			continue;

		// Byte address of the referenced source location
		sp = (rel [i] & 0x3fff) << 1;
		// trace ("REL %1d, offset %1d", i, sp);
		if (sp >= nb)
			goto Gar2;

		// Target pointer: shifted by the current adjustment
		np = tp = sp + de;
		AALIGN (np);
		// trace ("REL %1d, target %1d", i, np);

		if (np > tp) {
			// A new adjustment
			del [nd] = sp;			// Source addr >= this
			adj [nd] = de = (np - sp);	// Shifted by
			// trace ("REL %1d, adjustment %1d = %1d", i, nd, de);
			nd++;
		}

		// Here is where we are now as far as the source
		sp += 2;

		// Another (mandatory) adjustment (the if is just a comment)
		if (ADRS > 2)  {
			tp = sp + de + ADRS - 2;
			del [nd] = sp;
			adj [nd] = de = tp - sp;
			// trace ("REL %1d, adjustment %1d = %1d [forced]", i, nd, de);
			nd++;
		}
	}

	// We know all stepwise adjustments as well as the real target size; so
	// we can start assembling the object

	if ((*op = (void*)(TheNode->memAlloc (ts = nb + de, nb))) == NULL) {
		delete rel;
		delete del;
		delete adj;
		delete ob;
		return EE_LOADERR_NOMEM;
	}
	// trace ("TARGET OBJ, size = %1d", ts);

	oq = (byte*)(*op);

	// Now, fill the target

	ns = 0; ld = 0; sp = 0; tp = 0; de = 0;
	for (i = 0; i < nr; i++) {

		rt = rel [i] >> 14;
		np = (rel [i] & 0x3fff) << 1;
		// trace ("SecPass %1d, type = %1d, offset = %1d", i, rt, np);

		if (np > sp) {
			// A bunch of bytes to copy to the target
			dx = np - sp;
			memcpy (oq + tp, ob + sp, dx);
			// trace ("COPY from %1d to %1d, %1d items", sp, tp, dx);
			sp += dx;
			tp += dx;
		}

		// Locate the adjustment for this location
		while (ld < nd && del [ld] <= sp) {
			de = adj [ld];
			ld++;
		}

		// The target pointer
		tp = sp + de;
		// trace ("ADJUSTMENT = %1d [%1d], sp = %1d, tp = %1d", de, ld, sp, tp);

		switch (rt) {

			case 0:
			case 1:
				// The address to relocate (source variant)
				wd = *((word*)(ob + sp));
				re_endian_w (wd);
				// Check for adjustment
				for (da = 0, md = 0; md < nd && del [md] <= wd;
					md++)
						da = adj [md];
						// trace ("ADDRESS %1d [%04x] at %1d -> %1d [%08x] at %1d", wd, wd, sp, wd+da, wd+da, tp);
				wd += da;
				vp = oq + wd;
				// Store the new address
				*((byte**)(oq + tp)) = vp;

				if (rt == 1) {
					// Store string pointer for
					// verification
					// trace ("STRING VER %1d", ns);
					rel [ns++] = tp;
				}

				tp += ADRS;
				sp += 2;

				break;

			case 2:
				// Word
				wd = *((word*)(ob + sp));
				re_endian_w (wd);
				*((word*)(oq + tp)) = wd;
				// trace ("WORD %04x from %1d to %1d", wd, sp, tp);
				tp += 2;
				sp += 2;
				break;

			default:
				// LWord
				lwd = *((lword*)(op + sp));
				re_endian_lw (lwd);
				*((lword*)(oq + tp)) = lwd;
				// trace ("LWORD %08x from %1d to %1d", lwd, sp, tp);
				tp += 4;
				sp += 4;
		}
	}

	// The tail
	if (nb > sp) {
		memcpy (oq + tp, ob + sp, nb - sp);
		// trace ("TAIL from %1d to %1d, size = %1d", sp, tp, nb - sp);
	}

	delete del;
	delete adj;
	delete ob;

	for (es = (char*)oq + ts, wd = 0; wd < ns; wd++) {
		// trace ("STRING %1d at %1d", wd, rel [wd]);
		for (cs = (char*)(oq + rel [wd]); (cs < es) && *cs != '\0';
			cs++);
		if (cs >= es) {
			delete rel;
			delete oq;
			// trace ("FAILED!");
			return EE_LOADERR_GARBAGE;
		}
		// trace ("PASSED");
	}

	delete rel;
	return 0;
}
			
// ============================================================================
#else
// === PicOS version ==========================================================

byte ee_load (lword addr, void **op) {

	word nb, nr, rl, ta;
	address ap;

	if (ee_read (addr, (byte*)&nb, 2) || (nb & 0xc000) || nb == 0)
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

		if (rl & 0x8000)
			// This indicates a non-address (word/lword), which
			// at present is only used by VUEE
			continue;

		if ((ta = rl & 0x3fff) >= (nb >> 1))
			goto Garbage;

		// Relocate
		ta = ap [ta] += (word) ap;

		// Check if the relocated address is in range
		if (ta < (word) ap || ta >= (word) ap + nb)
			goto Garbage;

		if ((rl & 0x4000) != 0) {
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

#endif
