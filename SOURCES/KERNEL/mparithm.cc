/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-05   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

/* --------------------------------------------------- */
/* Functions for multiple precision integer arithmetic */
/* --------------------------------------------------- */

#if     BIG_precision > 1

#include        "system.h"

#define         L       BIG_precision   // Some abbreviations
#define         LL      (L+L)           // 2 * BIG_precision

#define ALLONES     LCS(0xffffffffffffffff)
#define MAXSIGN     LCS(0x7fffffffffffffff)
#define HFRIGHT         0xffffffff
#define HFLEFT      LCS(0x7fffffff00000000)
#define HFLEFTU     LCS(0xffffffff00000000)
#define UCMASK      LCS(0xffffffff80000000)
#define LCMASK          0x7fffffff
#define	CCBIT           0x80000000
#define MSBIT       LCS(0x4000000000000000)

#define	BILH	(BIL/2)

#if     ZZ_AER                      // Arithmetic errors monitored
#define achk(a,b)       if (!(a)) zz_aerror (b);
#else
#define achk(a,b)       ;
#endif

BIG     BIG_0 = &BIG_0;                 // Forces initialization
BIG     BIG_1, BIG_inf;                 // Constants

#if	ZZ_NFP
// No floating point
#undef	FLOATING_POINT
#else
static  BIG     max_convertible;        // Max convertible to double
static  double  powers  [L];
static  int     n_powers;
#define FLOATING_POINT 1
#endif

static  int     big_bits = 0;

static  LONG    bmask [BIL+1] = {
				0x00000000,
				0x00000001,
				0x00000003,
				0x00000007,
				0x0000000f,
				0x0000001f,
				0x0000003f,
				0x0000007f,
				0x000000ff,
				0x000001ff,
				0x000003ff,
				0x000007ff,
				0x00000fff,
				0x00001fff,
				0x00003fff,
				0x00007fff,
				0x0000ffff,
				0x0001ffff,
				0x0003ffff,
				0x0007ffff,
				0x000fffff,
				0x001fffff,
				0x003fffff,
				0x007fffff,
				0x00ffffff,
				0x01ffffff,
				0x03ffffff,
				0x07ffffff,
				0x0fffffff,
				0x1fffffff,
				0x3fffffff,
				0x7fffffff,
				0xffffffff,
                        LCS(0x00000001ffffffff),
                        LCS(0x00000003ffffffff),
                        LCS(0x00000007ffffffff),
                        LCS(0x0000000fffffffff),
                        LCS(0x0000001fffffffff),
                        LCS(0x0000003fffffffff),
                        LCS(0x0000007fffffffff),
                        LCS(0x000000ffffffffff),
                        LCS(0x000001ffffffffff),
                        LCS(0x000003ffffffffff),
                        LCS(0x000007ffffffffff),
                        LCS(0x00000fffffffffff),
                        LCS(0x00001fffffffffff),
                        LCS(0x00003fffffffffff),
                        LCS(0x00007fffffffffff),
                        LCS(0x0000ffffffffffff),
                        LCS(0x0001ffffffffffff),
                        LCS(0x0003ffffffffffff),
                        LCS(0x0007ffffffffffff),
                        LCS(0x000fffffffffffff),
                        LCS(0x001fffffffffffff),
                        LCS(0x003fffffffffffff),
                        LCS(0x007fffffffffffff),
                        LCS(0x00ffffffffffffff),
                        LCS(0x01ffffffffffffff),
                        LCS(0x03ffffffffffffff),
                        LCS(0x07ffffffffffffff),
                        LCS(0x0fffffffffffffff),
                        LCS(0x1fffffffffffffff),
                        LCS(0x3fffffffffffffff),
                        LCS(0x7fffffffffffffff),
                        LCS(0xffffffffffffffff)
   };


BIG::BIG (BIG*) {

/* ----------------------------------------- */
/* Initialize the multiple precision package */
/* ----------------------------------------- */

#if	FLOATING_POINT
double  p, q;
#endif
int     i;

	// Somebody may try to actually use this dummy conversion
	achk (!big_bits, "illegal conversion from BIG* to BIG");

	big_bits = L * (BIL-1);         // The number of bits in representation

#if	FLOATING_POINT
	q = HUGE / 2.0;                 // Double precision powers of two

	for (n_powers = 1, powers [0] = p = 1.0;
		n_powers < L; n_powers++) {

		for (i = 0; i < BIL-1; i++) {
			if (p > q) goto DONE;
			p *= 2.0;
		}

		powers [n_powers] = p;
	}
DONE:
#endif
	for (i = 0; i < L; i++) {       // Constants
		BIG_0.x [i] = 0x0;
		BIG_1.x [i] = 0x0;
		BIG_inf.x [i] = MAXSIGN;
	}

	BIG_1.x [0] = 1;

#if	FLOATING_POINT
/* ------------------------------------------------------ */
/* Calculate the maximum BIG number convertible to double */
/* ------------------------------------------------------ */
	for (i = L - 1; i >= n_powers; i--) max_convertible.x [i] = 0;

	for (p = HUGE; i >= 0; i--) {
		q = p / powers [i];
		if (q > powers [1]) {

			if (i == L - 1) {

				max_convertible = BIG_inf;
				break;

			} else {
				cerr << // Quite unlikely to ever get here
				   "Long arithmetic initialization screwup\n";
				exit (17);
			}
		}
		max_convertible.x [i] = (LONG) q;

		p -= max_convertible.x [i] * powers [i];
	}
#endif
}

int  operator< (const BIG &a, const BIG &b) {       // BIG < BIG

	for (int i = L - 1; i >= 0; i--) {
		if (a.x [i] < b.x [i]) return (1);
		if (a.x [i] > b.x [i]) return (0);
	}
	return (0);
}

#if  ZZ_TAG
int  ZZ_TBIG::cmp (const ZZ_TBIG &a) {

/* -------------- */
/* TAG comparison */
/* -------------- */

	for (int i = L - 1; i >= 0; i--) {
		if (a.x [i] < x [i]) return (1);
		if (a.x [i] > x [i]) return (-1);
	}
	// Base parts are equal: check the tags
	if (a.tag < tag) return (1);
	if (a.tag > tag) return (-1);
	return (0);
}

int  ZZ_TBIG::cmp (const BIG &a) {

/* -------------------- */
/* comparison with TIME */
/* -------------------- */

	for (int i = L - 1; i >= 0; i--) {
		if (a.x [i] < x [i]) return (1);
		if (a.x [i] > x [i]) return (-1);
	}
	return (0);
}

int  ZZ_TBIG::cmp (const BIG &a, LONG tg) {

/* -------------- */
/* TAG comparison */
/* -------------- */

	for (int i = L - 1; i >= 0; i--) {
		if (a.x [i] < x [i]) return (1);
		if (a.x [i] > x [i]) return (-1);
	}
	// Base parts are equal: check the tags
	if (tg < tag) return (1);
	if (tg > tag) return (-1);
	return (0);
}
#endif

int  operator<= (const BIG &a, const BIG &b) {      // BIG <= BIG

	for (int i = L - 1; i >= 0; i--) {
		if (a.x [i] > b.x [i]) return (0);
		if (a.x [i] < b.x [i]) return (1);
	}
	return (1);
}

int  operator> (const BIG &a, const BIG &b) {       // BIG > BIG

	for (int i = L - 1; i >= 0; i--) {
		if (a.x [i] > b.x [i]) return (1);
		if (a.x [i] < b.x [i]) return (0);
	}
	return (0);
}

int  operator>= (const BIG &a, const BIG &b) {      // BIG >= BIG

	for (int i = L - 1; i >= 0; i--) {
		if (a.x [i] < b.x [i]) return (0);
		if (a.x [i] > b.x [i]) return (1);
	}
	return (1);
}

BIG  operator+ (const BIG &a, const BIG &b) {       // BIG + BIG

	BIG     r;                      // The result
        int i, carry;

	for (i = 0, carry = 0; i < L; i++) {

		if ((r.x [i] = a.x [i] + b.x [i] + carry) < 0) {
			carry = 1;
			r.x [i] &= MAXSIGN;
		} else {
			carry = 0;
		}
	}

	achk (!carry, "addition overflow");
	return (r);
}

BIG  operator+ (const BIG &a, LONG b) {

	// Note: this function is semantically redundant: c++ could execute
	// BIG + LONG  as  BIG + (BIG) LONG.  It exists for improved
	// efficiency

	BIG     r;                      // The result
        int     i;

	if (b >= 0) {

		if ((r.x [0] = a.x [0] + b) >= 0) {     // No carry
			for (int i = 1; i < L; i++) r.x [i] = a.x [i];
			return (r);
		}

		r.x [0] &= MAXSIGN;                  // Carry

		for (i = 1; (i < L) && ((r.x [i] = a.x [i] + 1) < 0); i++)
			r.x [i] &= MAXSIGN;          // Propagate carry

		achk (i < L, "overflowed while adding integer to BIG");

		while (++i < L) r.x [i] = a.x [i];

	} else {

		if ((r.x [0] = a.x [0] + b) >= 0) {       // No carry
			for (int i = 1; i < L; i++) r.x [i] = a.x [i];
			return (r);
		}

		r.x [0] &= MAXSIGN;

		for (i = 1; (i < L) && ((r.x [i] = a.x [i] - 1) < 0); i++)
			r.x [i] &= MAXSIGN;

		achk (i < L, "underflowed while adding integer to BIG");

		while (++i < L) r.x [i] = a.x [i];
	}

	return (r);
}

double  operator+ (const BIG &a, double b) {
#if	FLOATING_POINT
	achk (convertible (a), "adding double to unconvertible BIG");
	return (b + (double) a);
#else
	zz_nfp ("BIG + double");
	return 0.0;
#endif
}

BIG  operator- (const BIG &a, const BIG &b) {

	BIG     r;                      // The result
	int     i, carry;

	for (i = 0, carry = 0; i < L; i++) {

		if ((r.x [i] = a.x [i] - b.x [i] - carry) < 0) {
			carry = 1;
			r.x [i] &= MAXSIGN;
		} else {
			carry = 0;
		}
	}

	achk (!carry, "subtraction overflow");

	return (r);
}

BIG  operator- (const BIG &a, LONG b) {

	// Redundant -- exists for increased efficiency

	BIG     r;                      // The result
	int	i;

	if (b >= 0) {

		if ((r.x [0] = a.x [0] - b) >= 0) {     // No carry
			for (int i = 1; i < L; i++) r.x [i] = a.x [i];
			return (r);
		}

		r.x [0] &= MAXSIGN;                  // Carry

		for (i = 1; (i < L) && ((r.x [i] = a.x [i] - 1) < 0); i++)
			r.x [i] &= MAXSIGN;          // Propagate carry

		achk (i < L, "underflowed while subtracting integer from BIG");

		while (++i < L) r.x [i] = a.x [i];

	} else {

		if ((r.x [0] = a.x [0] + b) >= 0) {       // No carry
			for (int i = 1; i < L; i++) r.x [i] = a.x [i];
			return (r);
		}

		r.x [0] &= MAXSIGN;

		for (i = 1; (i < L) && ((r.x [i] = a.x [i] + 1) < 0); i++)
			r.x [i] &= MAXSIGN;

		achk (i < L, "overflowed while subtracting integer from BIG");

		while (++i < L) r.x [i] = a.x [i];
	}

	return (r);
}

double  operator- (const BIG &a, double b) {
#if	FLOATING_POINT
	achk (convertible (a), "subtracting double from unconvertible BIG");
	return ((double) a - b);
#else
	zz_nfp ("BIG - double");
	return 0.0;
#endif
}

double  operator- (double a, const BIG &b) {
#if	FLOATING_POINT
	achk (convertible (b), "subtracting unconvertible BIG from double");
	return (b - (double) a);
#else
	zz_nfp ("double - BIG");
	return 0.0;
#endif
}

BIG  operator* (const BIG &a, const BIG &b) {

	// Note: this is slow -- avoid at all cost

	U_LONG   	tmp [LL];
	BIG             r;              // The result
	int		i;

	for (i = 0; i < L; i++) tmp [i] = 0;

	r = a * b.x [0];                // Initialize the result

	for (i = L-1; i > 0; i--) {

		if (b.x [i]) {          // Ignore zero factors

			*((BIG*)(&(tmp [L]))) = a * b.x [i];
#if     ZZ_AER
			for (int j = LL - 1; j >= LL-i; j--)
				achk (!tmp [j], "multiplication overflow");
#endif
			r = r + *((BIG*)(&(tmp[L-i])));
		}
	}
	return (r);
}

BIG  operator* (const BIG &a, LONG b) {

U_LONG    	aa [LL], cc [LL], sc [LL];
U_LONG    	mp1, mp2, carry, ccc;
int             i, al, k;
BIG             r;                      // The result

	// Convert the operand and calculate the effective length

	for (i = 0, k = 0, al = -1; i < L; i++, k += 2) {
		if ( a.x [i] ) {
			al = k + 2;
			aa [k] = a.x [i] & HFRIGHT;
			aa [k+1] = (a.x [i] & HFLEFT) >> BILH;
		} else {
			aa [k] = aa [k+1] = 0;
		}
	}

	if (al <= 0) {                  // The result is 0
		return (BIG_0);
	}

	if (b < 0) {
		achk (1, "multiplying BIG by negative LONG");
		return (BIG_0);
	}

	mp1 = b & HFRIGHT;
	mp2 = (b & HFLEFT) >> BILH;

	// The first iteration -- initialize the result

	for (i = 0, carry = 0; i < al; ) {

		cc [i] = aa [i] * mp1 + carry;
		carry = (cc [i] & HFLEFT) >> BILH;
		cc [i++] &= HFRIGHT;

		cc [i] = aa [i] * mp1 + carry;
		carry = (cc [i] & UCMASK) >> (BILH - 1);
		cc [i++] &= LCMASK;
	}

	while (i < LL) {
		cc [i++] = carry;       carry = 0;
	}

	if (carry) {
		achk (1, "multiplication overflow");
		return (BIG_0);
	}

	// The remaining iterations

	if (mp2) {

		for (i = 0, carry = 0; i < al;) {

			aa [i+1] <<= 1;
			if (aa [i] & CCBIT) aa [i+1] |= 1;
			aa [i] &= LCMASK;

			sc [i] = aa [i] * mp2 + carry;
			carry = (sc [i] & UCMASK) >> (BILH-1);
			sc [i++] &= LCMASK;

			sc [i] = aa [i] * mp2 + carry;
			carry = (sc [i] & HFLEFTU) >> BILH;
			sc [i++] &= HFRIGHT;
		}

		if (carry) {

			if (i < LL - 1) {
				if (ccc = ((sc [i] = carry) & UCMASK)) {
					sc [i] &= LCMASK;
					sc [i+1] = ccc >> (BILH-1);
					i += 2;
				} else {
					i++;
				}
			} else {
				achk (1, "multiplication overflow");
				return (BIG_0);
			}
		}

		for (k = 0, carry = 0; k < i; k++) {

			ccc = sc [k] + carry;
			if ((k >= LL - 1) && ccc) {
				achk (1, "multiplication overflow");
				return (BIG_0);
			}

			if (k & 1) {
				carry = ((cc [k+1] += ccc) & HFLEFTU) >> BILH;
				cc [k+1] &= HFRIGHT;
			} else {
				carry = ((cc [k+1] += ccc) & UCMASK) >> (BILH-1);
				cc [k+1] &= LCMASK;
			}
		}
	}

	for (i = 0, k = 0; i < L; i++, k += 2)
		r.x [i] = cc [k] | (cc [k+1] << BILH);

	return (r);
}

double  operator* (const BIG &a, double b) {

	// Note: the double is supposed to be small enough, so that the result
	// fits into a double.
#if	FLOATING_POINT
	int             i;
	double          w, p;
	BIG             sl;

	if (a <= max_convertible) {             // a is convertible to double
		// Convert a to double
		for (i = 0, w = 0.0; i < n_powers; i++)
			if (a.x [i]) w += a.x [i] * powers [i];
		/* And multiply directly */
		return (w * b);
	}

	// Adjust by a power of two (not very efficient but not supposed
	// to be used too often.

	for (i = 0; i < L; i++) sl.x [i] = a.x [i];
	p = 2.0;

	while (1) {
		zz_shiftB (sl, -1);
		if (sl <= max_convertible) break;
		p *= 2.0;
	}

	// Now convert BIG operand to double */

	for (i = 0, w = 0.0; i < n_powers; i++)
		if (sl.x [i]) w += sl.x [i] * powers [i];
	return ((w * b) * p);   // Exception possible, but who cares
#else
	zz_nfp ("BIG * double");
	return 0.0;
#endif
}

BIG  operator/ (const BIG &a, const BIG &b) {

	// Note: the algorithm for division is naive and slow.  BIG division
	// is not supposed to be used in the simulation. The same about modulo
	// and muliplication of LONG by LONG.

	int             pos, i;
	BIG             u, v, rem;
	BIG             r;              // The result

	for (pos = 0; pos < L; pos++)
		if (b.x [pos] != 0) break;
	if (pos >= L) {
		achk (1, "division by zero");
		return (BIG_0);
	}
	
	u = b;
	r = BIG_0;
	
	for (pos = 0; ; pos++) {
		if (a < u) {
			pos--;
			zz_shiftB (u, -1);
			break;
		}
		if ( u.x [L-1] & MSBIT ) break;
		zz_shiftB (u, 1);
	}

	for (i = 0; i < L; i++) rem.x [i] = a.x [i];

	if (pos >= 0) {
		while (1) {
			rem = rem - u;
			v.x [0] = 1;
			for (i = 1; i < L; i++) v.x [i] = 0;
			zz_shiftB (v, pos);
			r = r + v;
			while (rem < u) {
				--pos;
				zz_shiftB (u, -1);
			}
			if (pos < 0) break;
		}
	}
	return (r);
}

BIG  operator% (const BIG &a, const BIG &b) {

	int             pos, i;
	BIG             u;
	BIG             r;              // The result

	for (pos = 0; pos < L; pos++)
		if (b.x [pos] != 0) break;
	if (pos >= L) {
		achk (1, "division by zero");
		return (BIG_0);
	}
	
	u = b;
	
	for (pos = 0; ; pos++) {
		if (a < u) {
			pos--;
			zz_shiftB (u, -1);
			break;
		}
		if ( u.x [L-1] & MSBIT ) break;
		zz_shiftB (u, 1);
	}

	for (i = 0; i < L; i++) r.x [i] = a.x [i];

	if (pos >= 0) {
		while (1) {
			r = r - u;
			while (r < u) {
				--pos;
				zz_shiftB (u, -1);
			}
			if (pos < 0) break;
		}
	}
	return (r);
}

double  operator/ (const BIG &a, double b) {

	// Note: the double is supposed to be big enough, so that the result
	// fits into a double.
#if	FLOATING_POINT
	int             i;
	double          w, p;
	BIG             sl;

	achk (b != 0.0, "division by double zero");
	if (a <= max_convertible) {     // Convert a to double
		for (i = 0, w = 0.0; i < n_powers; i++)
			if (a.x [i]) w += a.x [i] * powers [i];
					// And divide directly
		return (w / b);
	}

	// Adjust by a power of two

	for (i = 0; i < L; i++) sl.x [i] = a.x [i];
	p = 2.0;

	while (1) {
		zz_shiftB (sl, -1);
		if (sl <= max_convertible) break;
		p *= 2.0;
	}

	// Now convert BIG operand to double

	for (i = 0, w = 0.0; i < n_powers; i++)
		if (sl.x [i]) w += sl.x [i] * powers [i];
	return ((w / b) * p);
#else
	zz_nfp ("BIG / double");
	return 0.0;
#endif
}

double  operator/ (double a, const BIG &b) {
#if	FLOATING_POINT
	int             i;
	double          w, p;
	BIG             sl;

	achk (b != BIG_0, "division of double by BIG zero");
	if (b <= max_convertible) {     // Convert a to double
		for (i = 0, w = 0.0; i < n_powers; i++)
			if (b.x [i]) w += b.x [i] * powers [i];
					// And divide directly
		return (a / w);
	}

	// Adjust by a power of two

	for (i = 0; i < L; i++) sl.x [i] = b.x [i];
	p = 2.0;

	while (1) {
		zz_shiftB (sl, -1);
		if (sl <= max_convertible) break;
		p *= 2.0;
	}

	// Now convert BIG operand to double

	for (i = 0, w = 0.0; i < n_powers; i++)
		if (sl.x [i]) w += sl.x [i] * powers [i];
	return ((a / w) / p);
#else
	zz_nfp ("double / BIG");
	return 0.0;
#endif
}

LONG  operator% (const BIG &a, LONG b) {

	BIG     r;

	r = a % ((BIG) b);

	return (r.x [0]);
}

BIG::BIG (double a) {                   // Constructor from double
#if	FLOATING_POINT
	int     i;
	double  f;
	
	if (a < 0.0) {
		achk (1, "converting negative double to BIG");
		*this = BIG_0;
	}

	for (i = L-1; i >= 0; i--) {
		if (i >= n_powers) {
			x [i] = 0;
			continue;
		}
		f = a / powers [i];
		if (f >= powers [1]) {
			achk (1, "overflowed while converting double to BIG");
			*this = BIG_inf;
		}
		x [i] = (LONG) f;
		a -= ((double) x [i]) * powers [i];
	}
#else
	zz_nfp ("BIG (double)");
#endif
}

BIG::operator  LONG () const {          // Conversion to LONG

#if     ZZ_AER

	for (int i = 1; i < L; i++)
		if (x [i]) achk (1, "overflowed while converting BIG to LONG");
#endif

	return (x [0]);
}

BIG::operator  double () const {        // Conversion to double
#if	FLOATING_POINT
	int     i;
	double  d;

	if (*this > max_convertible) {
		achk (1, "overflowed while converting BIG to double");
		return (HUGE);
	}
	for (i = 0, d = 0.0; i < n_powers; i++)
		if (x [i]) d += x [i] * powers [i];
	return (d);
#else
	zz_nfp ("double (BIG)");
	return 0.0;
#endif
}

int  convertible (const BIG &a) {
#if	FLOATING_POINT
	return (a <= max_convertible);
#else
	return 0
#endif
}

BIG  atob (const char *s) {             // String to BIG conversion

	BIG     r;

	while (*s == ' ') s++;
	if (*s == '+') s++;
	while (*s == ' ') s++;

	r = BIG_0;

	while ((*s <= '9') && (*s >= '0')) {
		r = r * 10 + (*s - '0');
		s++;
	}

	return (r);
}

static  char    *btoabuf = NULL;
static  int     btoable = 0;

char    *btoa (const BIG &a, char *s, int nc) { // BIG to string

	U_LONG    	aa [LL];
	U_LONG    	carry, ccc;
	int             i, j, al, k;
	char            temp [256], *tm, *res;  // Hopefully enough

	if (s == NULL) {
		if (nc > 0 && btoable < nc) {
			if (btoable > 0) delete (btoabuf);
			btoabuf = new char [(btoable = nc) + 1];
		}
		s = btoabuf;
	}
	res = s;

	/* Convert the operand and calculate the effective length */

	for (i = 0, k = 0, al = -1; i < L; i++, k += 2) {
		if ( a.x [i] ) {
			al = k + 2;
			aa [k] = a.x [i] & HFRIGHT;
			aa [k+1] = (a.x [i] & HFLEFT) >> BILH;
		} else {
			aa [k] = aa [k+1] = 0;
		}
	}

	tm = &(temp [255]);

	for (j = 0, al--; j < 256; j++) {

		if (al < 0) {
			if (j == 0) *--tm = '0'; else break;
			continue;
		}

		for (i = al, al = -1; i >= 0; ) {

			carry = aa [i] - (ccc = aa [i] / 10) * 10;
			if ((aa [i] = ccc) && (al < 0)) al = i;
			aa [--i] |= (carry << BILH);
			carry = aa [i] - (ccc = aa [i] / 10) * 10;
			if ((aa [i] = ccc) && (al < 0)) al = i;
			if (--i >= 0)
				aa [i] |= (carry << (BILH-1));
		}

		if (!(al & 1)) al++;

		*--tm = '0' + (char) carry;
	}

	if (j <= nc) {
		while (nc-- >  j) *s++ = ' ';
		while (nc-- >= 0) *s++ = *tm++;
		*s = '\0';
		return (res);
	}

	if ((i = j - nc + 2) > 9)
		i = j - nc + 3;

	if (i >= j) {
		while (nc-- > 0) *s++ = '*';
		*s = '\0';
		return (res);
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
	return (res);
}

void    zz_shiftB (BIG &a, int c) {     // Shifts BIG by c binary positions

	int     w, b, i;
	LONG    p, q, m;

	if (c > 0) {
		if (c >= big_bits) {
			for (i = 0; i < L; i++) a.x [i] = 0;
			return;
		}
		w = c / (BIL-1);             /* Words */
		b = c % (BIL-1);             /* Bits */

		if (w) {
			for (i = L - 1; i >= w; i--)
				a.x [i] = a.x [i - w];
			for (; i >= 0; i--) a.x [i] = 0;
		}

		if (b) {
			m = bmask [b] << ((BIL-1) - b);
			for ( i = w, p = 0; i < L; i++) {
				q = (a.x [i] & m) >> ((BIL-1) - b);
				a.x [i] = ((a.x [i] & ~m) << b) | p;
				p = q;
			}
		}
	} else {
		if (c <= -big_bits) {
			for (i = 0; i < L; i++) a.x [i] = 0;
			return;
		}
		w = (c = -c) / (BIL-1);      // Words
		b = c % (BIL-1);             // Bits

		if (w) {
			for (i = 0; i < L - w; i++)
				a.x [i] = a.x [i + w];
			for (; i < L; i++) a.x [i] = 0;
		}

		if (b) {
			m = bmask [b];
			for ( i = L - w - 1, p = 0; i >= 0; i--) {
				q = (a.x [i] & m) << ((BIL-1) - b);
				a.x [i] = ((a.x [i] & ~m) >> b) | p;
				p = q;
			}
		}
	}
}

#endif

#if     BIG_precision != 1
/* -------------------------------------- */
/* Overload "<<" to print out BIG numbers */
/* -------------------------------------- */
ostream &operator<< (ostream &s, const BIG &b) {

	char xx [24], *xxx;

	if (b == TIME_inf) {
		s << "TIME_inf";
	} else {
		btoa (b, xx);
		for (xxx = xx; *xxx == ' '; xxx++);
		s << xxx;
	}
	return (s);
};

/* --------------------------------- */
/* Overload ">>" to read BIG numbers */
/* --------------------------------- */
istream &operator>> (istream &s, BIG &b) {
	char    c;
	int     ef;

	while (ef = !s.eof ()) {
		s.get (c);
		if ((c != ' ') && (c != '\t') && (c != '\n')) break;
	}
	if (!ef) {
		b = BIG_0;
		return (s);
	}
	if (c == '+') {
		s.get (c);
		ef = !s.eof ();
	}
	if (!ef || (c < '0') || (c > '9')) {
		b = BIG_0;
		return (s);
	}
	for (b = (BIG) (c - '0'); ef = !s.eof (); ) {
		s.get (c);
		if (c < '0' || c > '9') break;
		b = b * 10 + (c - '0');
	}
	if (ef) s.putback (c);
	return (s);
};
#endif
