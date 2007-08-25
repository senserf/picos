/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-07   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

/* ---------------------------------- */
/* Functions for numeric input/output */
/* ---------------------------------- */

#include        "system.h"

#if	ZZ_NFP
#undef	FLOATING_POINT
#else
#define	FLOATING_POINT 1
static  double          last_real = 0.0;        // Last floating number read
#endif

static  LONG            last_integer = 0;       // Last integer number read
#if     BIG_precision != 1
static  BIG             last_big = BIG_0;       // Last BIG number read
#endif

static  int             ni_counter = 0,         // Pending count for readInteger
#if     BIG_precision != 1
			nl_counter = 0,         // ... and for readBig
#endif
			nr_counter = 0;         // ... and for readReal

/* ---------------------------------------------------------- */
/* Input   subroutines:  for  reading  numbers  and  skipping */
/* garbage                                                    */
/* ---------------------------------------------------------- */

#ifdef	FLOATING_POINT

int parseNumbers (const char *txt, int max, nparse_t *res) {

	int count = 0;
	char *en, *em;
	double	dv;
	LONG	lv;
	long	iv;
	Boolean FPE, INE;

	while (1) {
		while (	*txt != '\0' &&
				*txt != '+' &&
					*txt != '-' &&
						!isdigit (*txt))
							txt++;
		if (*txt == '\0')
			// We are done
			break;

		if (count < max) {

			// Still room
			switch (res->type) {

			    case TYPE_hex:

				iv = (int) strtoll (txt, &en, 16);
				goto Hex;

			    case TYPE_int:

				// Expect int
				iv = strtol (txt, &en, 10);
Hex:
				if (en == txt) {
					// No number
					txt++;
					continue;
				}
				if (errno == ERANGE)
					return ERROR;

				// This will be optimized out on 32-bit
				// machines
				if ((sizeof (long) > sizeof (int)) &&
				    (iv > (long)MAXINT || iv < (long)MININT))
					return ERROR;

				res->IVal = (int) iv;
				txt = en;
				break;

			    case TYPE_LONG:

				// Expect LONG
				lv = strtoll (txt, &en, 10);
				if (en == txt) {
					// No number
					txt++;
					continue;
				}
				if (errno == ERANGE)
					return ERROR;
				res->LVal = lv;
				txt = en;
				break;

			    case TYPE_double:

				// Expect double
				dv = strtod (txt, &en);
				if (en == txt) {
					// Failure
					txt++;
					continue;
				}
				if (errno == ERANGE)
					return ERROR;
				res->DVal = dv;
				txt = en;
				break;

			    default:

			      if (res->type < 0) {
				// Any type, determine based on number format
				dv = strtod (txt, &en);
				// Try double first
				if (en == txt) {
					// Failure
					txt++;
					continue;
				}
				// Check for error
				FPE = (errno == ERANGE);
				// Try LONG
				lv = strtoll (txt, &em, 10);
				INE = (errno == ERANGE);
				if (INE && FPE)
					// Error
					return ERROR;
				if (FPE || ((em == en) && !INE)) {
					// We opt for LONG
					res->type = TYPE_LONG;
					res->LVal = lv;
					txt = em;
				} else {
					res->type = TYPE_double;
					res->DVal = dv;
					txt = en;
				}
			      } else
				excptn ("parseNumbers: illegal item type: %1d",
					res->type);
			}

			res++;

		} else {

			// Skip a double (who cares)
			strtod (txt, &en);
			txt = (en == txt) ? txt + 1 : en;
		}
		count++;
	}
	return count;
}

#endif
	
static  char    find_number () {

/* ---------------------------- */
/* Locate the next input number */
/* ---------------------------- */

	char    k, l;
	int     ef;

	if (EndOfData) return (' ');
SKIP:
	while (ef = !Inf.eof ()) {
		Inf.get (k);
		if (((k < '0') || (k > '9')) && (k != '+') && (k != '-') &&
			(k != '%')) {

			// A character to be skipped

			if (k == '*')   // Comment - skip to eol
				while (ef = !Inf.eof ()) {
					Inf.get (k);
					if (k == '\n') break;
				}
			if (!ef) break;
		} else
			// An interesting character to be looked at
			break;
	}

	if (ef) {

		// No special status

		if ((k >= '0') && (k <= '9')) {
			Inf.putback (k);
		} else {
			if ((k == '+') || (k == '-')) {
				// Must be followed by a digit
				if (ef = !Inf.eof ()) {
					Inf.get (l);
					if ((l < '0') || (l > '9'))
						goto SKIP;
					else
						Inf.putback (l);
				}
			}
		}
	}

	if (ef) return (k);

	if (Inf.eof ()) {
		EndOfData = YES;
		return (0);
	};

	excptn ("i/o error on input file");

	return (0);     // No way to get here
}

void    readIn (LONG &frs) {

/* -------------------------- */
/* Read (LONG) integer number */
/* -------------------------- */

	LONG            res;
	char            sign, k;
	int             ef;

	Assert (zz_ifpp != NULL,
		"readIn: can't read data file after the protocol has started");
	if (ni_counter) {
		ni_counter--;
		frs = last_integer;
		return;
	}

	k = find_number ();
	if (EndOfData) {
		frs = 0;
		return;
	}

	if (k == '%') {
		frs = last_integer;
		if (!Inf.eof ()) {
			Inf.get (k); Inf.putback (k);
			if ((k != '+') && (k != '-')) goto I_DONE;
		} else {
			if (Inf.eof ()) {
				EndOfData = YES;
				frs = 0;
				return;
			} else
				excptn ("i/o error on input file");
		}
	} else {
		if (k < '0') Inf.putback (k);
		frs = 0;
	}

	while (YES) {
		sign = find_number ();
		if (EndOfData) goto I_DONE;
		for (res=0; ef = !Inf.eof (); ) {
			Inf.get (k);
			if (k < '0' || k > '9') break;
			res = res * 10 - (k - '0');
		}
		if (!ef && !Inf.eof ()) excptn ("i/o error on input file");
		Inf.putback (k);
		frs += (sign == '-') ? res : -res;
		if ((k != '+') && (k != '-')) break;
	}
I_DONE:
	if (ef && k == '/') {
		Inf.get (k);            // Skip the slash
		for  (ni_counter = 0; ef = !Inf.eof (); ) {
			Inf.get (k);
			if (k < '0' || k > '9') break;
			ni_counter = ni_counter * 10 + (k - '0');
		}
		if (ni_counter) ni_counter--;
	}

	last_integer = frs;
}

void    readIn (double &frs) {

/* ---------------------------------------------- */
/* Read the next floating point number from input */
/* ---------------------------------------------- */
#if	FLOATING_POINT
	char 		collect [32];
	Boolean		wasdot, wasexp, wasdig, wassig;
	double          res;
	char            sign, k;
	int             ef, nc;

	Assert (zz_ifpp != NULL,
		"readIn: can't read data file after the protocol has started");
	if (nr_counter) {
		nr_counter--;
		frs = last_real;
		return;
	}

	k = find_number ();
	if (EndOfData) {
		frs = 0.0;
		return;
	}

	if (k == '%') {
		frs = last_real;
		if (ef = !Inf.eof ()) {
			Inf.get (k); Inf.putback (k);
			if ((k != '+') && (k != '-')) goto F_DONE;
		} else {
			if (Inf.eof ()) {
				EndOfData = YES;
				frs = 0.0;
				return;
			} else
				excptn ("i/o error on input file");
		}
	} else {
		if (k < '0') Inf.putback (k);
		frs = 0.0;
	}

	while (YES) {
		sign = find_number ();  // Find beginning of the number
		if (EndOfData) goto F_DONE;
		if (sign == '-') {
			collect [0] = '-';
			nc = 1;
		} else {
			nc = 0;
		}

		wasdot = wasexp = wasdig = wassig = NO;
		while ((ef = !Inf.eof ())) {
			Inf.get (k);
			if (k >= '0' && k <= '9') {
				// A digit, OK
				wasdig = YES;
			} else if (k == 'E' || k == 'e') {
				if (wasexp)
					break;
				if (!wasdig)
					break;
				wasexp = YES;
				wasdig = NO;
			} else if (k == '.') {
				if (wasdot || !wasdig)
					break;
				wasdot = YES;
				wasdig = NO;
			} else if (k == '+' || k == '-') {
				if (!wasexp || wassig || wasdig)
					break;
				wassig = YES;
				wasdig = NO;
			} else
				break;

			if (nc == 31)
				break;

			collect [nc++] = k;
		}

		collect [nc] = '\0';

		res = strtod (collect, NULL);

		if (ef) Inf.putback (k);
		frs +=  res;
		if ((k != '+') && (k != '-')) break;
	}
F_DONE:
	if (ef && k == '/') {
		Inf.get (k);            // Skip the slash
		for  (nr_counter = 0; ef = !Inf.eof (); ) {
			Inf.get (k);
			if (k < '0' || k > '9') break;
			nr_counter = nr_counter * 10 + (k - '0');
		}
		if (nr_counter) nr_counter--;
	}

	if (!ef && ! Inf.eof ()) excptn ("i/o error on input file");

	last_real = frs;
#else
	zz_nfp ("readIn (double)");
#endif
}

#if     BIG_precision != 1

void    readIn (BIG &frs) {

/* ------------------------- */
/* Read (BIG) integer number */
/* ------------------------- */

	BIG             res;
	char            sign, k;
	int             ef;

	Assert (zz_ifpp != NULL,
		"readIn: can't read data file after the protocol has started");
	if (nl_counter) {
		nl_counter--;
		frs = last_big;
		return;
	}

	k = find_number ();
	if (EndOfData) {
		frs = BIG_0;
		return;
	}

	if (k == '%') {
		frs = last_big;
		if (!Inf.eof ()) {
			Inf.get (k); Inf.putback (k);
			if ((k != '+') && (k != '-')) goto B_DONE;
		} else {
			if (Inf.eof ()) {
				EndOfData = YES;
				frs = BIG_0;
				return;
			} else
				excptn ("i/o error on input file");
		}
	} else {
		if (k < '0') Inf.putback (k);
		frs = BIG_0;
	}

	while (YES) {
		sign = find_number ();  // Find number on input
		if (EndOfData) goto B_DONE;

		for (res=BIG_0; ef = !Inf.eof (); ) {
			Inf.get (k);
			if (k < '0' || k > '9') break;
			res = res * 10 + (k - '0');
		}

		if (ef) Inf.putback (k);

		if (sign == '-') {
			if (res > frs)
				ierror ("negative BIG number on input");
			frs -= res;
		} else {
			frs += res;
		}
		if ((k != '+') && (k != '-')) break;
	}

B_DONE:
	if (ef && k == '/') {
		Inf.get (k);            // Skip the slash
		for  (nl_counter = 0; ef = !Inf.eof (); ) {
			Inf.get (k);
			if (k < '0' || k > '9') break;
			nl_counter = nl_counter * 10 + (k - '0');
		}
		if (nl_counter) nl_counter--;
	}

	if (!ef && !Inf.eof ()) excptn ("i/o error on input file");

	last_big = frs;
}

#endif

/* ------------------ */
/* Two simple kludges */
/* ------------------ */

void    readIn (int &res) { LONG r; readIn (r); res = (int) r; }
#if	ZZ_LONG_is_not_long
void    readIn (long &res) { LONG r; readIn (r); res = (long) r; }
#endif
#if	FLOATING_POINT
void    readIn (float &res) { double r; readIn (r); res = r; }
#else
void    readIn (float &res) { zz_nfp ("readIn (float)"); }
#endif

void    encodeLong (LONG nn, char *s, int nc) {

/* ------------------------------------------ */
/* Encode LONG integer with optional exponent */
/* ------------------------------------------ */

	int             i, j, sf;
	LONG            al;
	char            temp [40], *tm;

	// Turn the number to negative and remember the sign
	if (nn < 0) {
		sf = 1;
	} else {
		sf = 0;
		nn = -nn;
	}

	tm = &(temp [39]);

	for (j = 0; j < 40; j++) {

		if (nn == 0) {
			if (j == 0) *--tm = '0'; else break;
			continue;
		}

		al = nn;
		*--tm = '0' + (char)((((nn /= 10) * 10) - al));
	}

	if (sf) {       // Insert minus
		j++;
		*--tm = '-';
	}

	if (j <= nc) {
		while (nc-- >  j) *s++ = ' ';
		while (nc-- >= 0) *s++ = *tm++;
		*s = '\0';
		return;
	}

	if ((i = j - nc + 2) > 9)
		i = j - nc + 3;

	if (i >= j) {
		while (nc-- > 0) *s++ = '*';
		*s = '\0';
		return;
	}

	while (j-- > i) *s++ = *tm++;
	*s++ = 'E';
	if (i > 9) {
		*s++ = (j = (i/10)) + '0';
		*s++ = (i - (j * 10)) + '0';
	} else {
		*s++ = i + '0';
	}
	*s ='\0';
}

void    encodeInt (int i, char *s, int nc) {

/* ---------------------------------------------------------- */
/* Encodes  an  integer, as opposed to LONG. Differs from the */
/* above in the initializer.                                  */
/* ---------------------------------------------------------- */

	encodeLong ((LONG) i, s, nc);
}

void    print (LONG n, const char *hdr, int nsize, int hsize) {

/* ------------------------------------------------- */
/* Prints out an integer number preceded by a header */
/* ------------------------------------------------- */

	char    f [256];  const char *h;

	Assert (nsize >= 0 && hsize >= 0, "print: item size (%1d) is negative",
		nsize);
	if (hdr == NULL) h = ""; else h = hdr;

	if (hsize == 0) {
		Ouf << h;
	} else {
		while (*h != '\0' && hsize--) Ouf.put (*h++);
		while (hsize-- > 0) Ouf.put (' ');
	}

	encodeLong (n, f, nsize);
	Ouf << f;
	if (hdr != NULL) Ouf << "\n";
}

void    print (double n, const char *hdr, int nsize, int hsize) {

/* ----------------------------------------------- */
/* Prints out a double number preceded by a header */
/* ----------------------------------------------- */
#if	FLOATING_POINT
        char    f [16];  const char *h;

        if (hdr == NULL) h = ""; else h = hdr;

        if (nsize < 7) nsize = 7;

        if (hsize == 0) {
                Ouf << h;
        } else {
                while (*h != '\0' && hsize--) Ouf.put (*h++);
                while (hsize-- > 0) Ouf.put (' ');
        }

        strcpy (f, form ("%%%1d.%1dg", nsize, nsize - 5));
        Ouf << form (f, n);
        if (hdr != NULL) Ouf << "\n";
#else
	zz_nfp ("print (double, ...)");
#endif
}

#if     BIG_precision != 1

void    print (const BIG &n, const char *hdr, int nsize, int hsize) {

/* -------------------------------------------- */
/* Prints out a BIG number preceded by a header */
/* -------------------------------------------- */

	char    f [256]; const char *h;

	Assert (nsize >= 0 && hsize >= 0, "print: item size (%1d) is negative",
		nsize);
	if (hdr == NULL) h = ""; else h = hdr;

	if (hsize == 0) {
		Ouf << h;
	} else {
		while (*h != '\0' && hsize--) Ouf.put (*h++);
		while (hsize-- > 0) Ouf.put (' ');
	}

	btoa (n, f, nsize);
	Ouf << f;
	if (hdr != NULL) Ouf << "\n";
}

#endif

void    print (const char *n, int nsize) {

/* ------------------- */
/* Prints out a string */
/* ------------------- */

	int     l;

	Assert (nsize >= 0, "print: item size (%1d) is negative",
		nsize);
	if (n == NULL) n = "";

	if (nsize == 0) {
		Ouf << n;
	} else {
		l = strlen (n);
		while (nsize > l) {
			Ouf.put (' ');
			nsize--;
		}
		while (*n != '\0' && nsize--) Ouf.put (*n++);
	}
}

void    zz_ptime (TIME &t, int s) {

/* ---------------------- */
/* Prints out time values */
/* ---------------------- */

	if (undef (t)) print ("undefined", s); else print (t, s);
}

/* =========================== */
/* Wrappers for sxml functions */
/* =========================== */

sxml_t	sxml_parse_input (char del) {
/*
 * Parse the input data. The optional character (if not 0) gives the delimiter
 * expected to occur as the only character of a terminating line. Otherwise,
 * the stream is read until EOF.
 */
	int	CSize, MSize = 1024;
	char	*IF, *SF, c, d;
	sxml_t	xml;
	Boolean	EOL;

#define	sxml_put(h)	do { \
				if (CSize == MSize) { \
					SF = new char [MSize + MSize]; \
					memcpy (SF, IF, CSize); \
					delete IF; \
					IF = SF; \
					MSize += MSize; \
				} \
				IF [CSize++] = h; \
			} while (0)

	if (EndOfData)
		return sxml_parse_str ("", 0);

	IF = new char [MSize];
	CSize = 0;
	EOL = YES;

	while (!Inf.eof ()) {
		Inf.get (c);
		if (EOL && c != '\0' && c == del) {
			if (Inf.eof ())
				break;
			d = c;
			Inf.get (c);
			if (c == '\n')
				break;
			sxml_put (d);
		}
		sxml_put (c);
		EOL == (c == '\n');
	}

	xml = sxml_parse_str1 (IF, CSize);
	// No need to delete IF; it is being recycled by sxml_parse_str1 and
	// will be deallocated along with the entire structure by sxml_free
	return xml;
}

void 	zz_outth () {

// Used to include the time header in paper exposures

	Ouf << "Time: " << Time;
	if (Etu != 1.0)
		Ouf << ::form (" [%8.6f]", ituToEtu (Time));
}

#if  ZZ_TAG
void    zz_ptime (ZZ_TBIG &t, int s) {

/* ----------------------------- */
/* Prints out tagged time values */
/* ----------------------------- */

	TIME    tt;

	t . get (tt);
	zz_ptime (tt, s);
}
#endif
