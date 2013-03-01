#include <stdio.h>
#include "locengine.h"

//
// Copyright (C) 2008-2012 Olsonet Communications Corporation
//
// PG March 2008, revised January 2012
//

#if DEBUGGING

static void dump_pt (tpoint_t *p) {

	int i;

	printf ("***PT [%1.2f, %1.2f] %1u (%08x) | %1d:", 
		p->x, p->y, p->Tag, p->properties, p->NPegs);

	for (i = 0; i < p->NPegs; i++) {
		printf (" <%1d,%1u,%3.0f>", p->Pegs [i] . Peg,
					    p->Pegs [i] . RSSI,
					    p->Pegs [i] . SLR);
	}

	printf ("\n---\n");
}

static void dump_al (alitem_t *al, int n) {

	int i;

	printf ("***AL:");

	for (i = 0; i < n; i++) {
		printf (" <%1d,%1u,%3.0f>", al [i] . Peg,
					    al [i] . RSSI,
					    al [i] . SLR);
	}

	printf ("\n---\n");
}

#endif

int loc_findrect (float x0, float y0, float x1, float y1, tpoint_t **v, int N) {
//
// Finds all points that fall into the specified rectangle and returns them via
// v (of size N). The function value returns the actual number of points
// falling into the rectangle. If that number is greater than N, it means that
// the array is too short to contain them all.
//
	tpoint_t *P;
	int np;

	// Start reading all points
	db_start_points (0);

	np = 0;
	while ((P = db_get_point ()) != NULL) {
		if (tpoint_inrec (x0, y0, x1, y1, P)) {
			if (np < N)
				v [np] = P;
			np++;
		}
		db_next_point ();
	}

	return np;
}

// ============================================================================

static float pdist (tpoint_t *P, alitem_t *al, int N) {
//
// Calculates the discrepancy between the point and the Tag's association list
//
	int i, NP, ll, rr, mid, NM;
	double S, Delta;
	alitem_t *PI;
	u32 CP, pp;
	float ss;

	PI = P->Pegs;
	NP = P->NPegs;

	// Partial sum
	S = 0.0;

	// The number of matched entries
	NM = 0;

	for (i = 0; i < N; i++) {

		// Current peg
		CP = al [i].Peg;

		// Check if this is the point's tag
		if (P->Tag == CP) {
			if (PM_dis_tag == 0)
				// Matching with tag not allowed, so this is a
				// mismatch, as Tag cannot be one of the Pegs
				continue;
			if ((P->properties & PROP_AUTOPROF) == 0)
				// Ignore if not autoprofile
				continue;
			// Current SLR
			ss = al [i].SLR;
			if (PM_dis_tag < 0) {
				// This is a bound
				if (ss > PM_dis_taf)
					ss = PM_dis_taf;
#if DEBUGGING
				printf ("DIST THR: %d, %u, %1.2f\n", i, CP, ss);
#endif
			}
			Delta = ss - PM_dis_taf;
			S += Delta * Delta;
#if DEBUGGING
			printf ("DIST DTHR: %d, %u, %1.2f %1.2f\n",
				i, CP, Delta, sqrt (S));
#endif
			NM ++;
			continue;
		}

		// Not the Tag, find it on the point's association list
			
		ll = 0;
		rr = NP;
		while (ll < rr) {
			pp = PI [mid = (ll + rr) >> 1] . Peg;
			if (pp > CP) {
				rr = mid;
			} else if (pp < CP) {
				ll = mid + 1;
			} else
				// Hit
				goto Found;
		}
		// Not found: ignore; FIXME: should we introduce (optional?)
		// penalty for such misses?
		continue;
Found:
		Delta = al [i].SLR - PI [mid].SLR;
		S += Delta * Delta;
#if DEBUGGING
		printf ("DIST: %d, %u, %1.2f, %1.2f\n", i, CP, Delta, sqrt (S));
#endif
		NM ++;
	}

	if (NM < PM_dis_min)
		// Cannot calculate the distance
		return -1.0;
#if DEBUGGING
		printf ("DIST FAC: %d, %1.2f\n", NM, sqrt (NM * PM_dis_fac));
#endif

	return (float) sqrt (S / (NM * PM_dis_fac));
}

// ============================================================================

struct lskey_s {
//
// Used for sorting points based on the discrepancy
//
	struct lskey_s *Next;
	tpoint_t *P;
	float    D;	// Discrepancy
};

typedef struct lskey_s lskey_t;

static lskey_t *LM_SEL;	// The pool of points
static int	LM_N;	// Pool size
static float	LM_MAX;	// The maximum D in the pool

static void padd (lskey_t *p) {
//
// Sort in the point into the pool
//
	lskey_t *c, *b;

	if (p->D >= LM_MAX) {
		p->Next = LM_SEL;
		LM_SEL = p;
		LM_MAX = p->D;
	} else {
		for (b = NULL, c = LM_SEL; c != NULL && c->D > p->D;
			c = (b = c)->Next);
		p->Next = c;
		if (b == NULL)
			abt ("padd internal error: b == NULL");
		b->Next = p;
	}
	LM_N++;
}

static void pdeltop (int n) {
//
// Delete n items from the top
//
	lskey_t *c;


	if (LM_N < n)
		abt ("pdeltop internal error: LM_N (%1d) < n (%1d)", LM_N, n);

	LM_N -= n;

	while (n--) {
		if (LM_SEL == NULL)
			abt ("pdeltop internal error: LM_SEL == NULL (%1d)", n);
		c = LM_SEL->Next;
		free (LM_SEL);
		LM_SEL = c;
	}

	if (LM_SEL == NULL)
		LM_MAX = 0.0;
	else
		LM_MAX = LM_SEL->D;
}

// ============================================================================

int loc_locate (u32 tag, alitem_t *al, int N, u32 prop, float *X, float *Y) {
//
// Carries out the location estimation:
//
//	tag	- the tag making the query (unused for now)
//	al	- the query association list
//	N	- the length of the query association list
//	prop	- properties of the query
//	X, Y	- returned estimated position
//
// Return value: the number of points used for estimation; 0 means estimation
// failed
//
	int i, m;
	float d;
	double D, s, W, x, y;
	tpoint_t *TP;
	lskey_t *p;

	// Make sure the pegs are sorted
	db_sort_al (al, N);

	// Make sure RSSI is transformed into SLR
	for (i = 0; i < N; i++)
		al [i].SLR = rssi_to_slr (al [i].RSSI);
#if DEBUGGING
	printf ("QUERY: %1u, %08x, %1d\n", tag, prop, N);
	dump_al (al, N);
#endif
	LM_N = 0;
	LM_SEL = NULL;
	LM_MAX = 0.0;

	db_start_points (0);

	while ((TP = db_get_point ()) != NULL) {
#if DEBUGGING
		printf ("TRYING: === "); dump_pt (TP);
#endif
		if (((TP->properties ^ prop) & PROP_ENVMASK) != 0)
			// Wrong point
			goto NextP;

		d = pdist (TP, al, N);
#if DEBUGGING
		printf ("DIST: %1.2f\n", d);
#endif
		if (d < 0.0)
			// No match at all
			goto NextP;

		if (LM_N < PM_sel_max) {
			// Just add it in; reverse sort based on d
			if ((p = (lskey_t*) malloc (sizeof (lskey_t))) == NULL)
				oom ("loc_locate <p0>");
			p->P = TP;
			p->D = d;
			padd (p);
			goto NextP;
		}
		// The list is full, so add only if better
		if (d >= LM_MAX)
			goto NextP;

		// Delete the current max
		pdeltop (1);
		if ((p = (lskey_t*) malloc (sizeof (lskey_t))) == NULL)
			oom ("loc_locate <p1>");
		p->P = TP;
		p->D = d;
		padd (p);
		//
NextP:		db_next_point ();
	}

#if DEBUGGING
	printf ("TO TRIM: %1d\n", LM_N);
	for (p = LM_SEL; p != NULL; p = p->Next) {
		printf ("DIST: %1.2f === ", p->D);
		dump_pt (p->P);
	}
#endif
	// Trim out

	while (LM_N > PM_sel_min) {

		// Calculate the average
		for (s = 0.0, p = LM_SEL; p != NULL; p = p->Next)
			s += p->D;

		s = (s / LM_N) * PM_sel_fac;
		// Max points that can be deleted
		m = LM_N - PM_sel_min;
#if DEBUGGING
		printf ("TRIM LOOP: %1.2f, %1d\n", s, m);
#endif
		for (i = 0, p = LM_SEL; p != NULL && p->D >= s && i < m;
			i++, p = p->Next);

		if (i == 0)
			// No more deletions
			break;
#if DEBUGGING
		printf ("TRIMMED %1d\n", i);
#endif
		pdeltop (i);
	}

	// We are left with the final pool
#if DEBUGGING
	printf ("FINAL: %1d\n", LM_N);
	for (p = LM_SEL; p != NULL; p = p->Next) {
		printf ("DIST: %1.2f === ", p->D);
		dump_pt (p->P);
	}
#endif
	if (LM_N == 0)
		// No way
		return 0;

	x = y = 0.0;

	if (PM_ave_for == 0) {
		// Linear
		for (D = 0.0, p = LM_SEL; p != NULL; p = p->Next)
			// Precompute sum of discrepancies
			D += p->D;
#if DEBUGGING
		printf ("SODISC: %1.2f\n", D);
#endif
		// Precompute the exponent
		for (s = 0.0, p = LM_SEL; p != NULL; p = p->Next) {
			// This is the weight
			W = pow (D - p->D, PM_ave_fac);
			// Accumulate
			s += W;
			x += p->P->x * W;
			y += p->P->y * W;
#if DEBUGGING
			printf ("LWEIGHT: %1.2f, %1.2f->%1.2f, %1.2f->%1.2f, "
				"%1.2f\n", W, p->P->x, x, p->P->y, y, s);
#endif
		}
	} else {
		// Exponential
		for (m = 0, D = 0.0, p = LM_SEL; p != NULL; p = p->Next, m++)
			// Calculate the average discrepancy to make the
			// result scale-invariant
			D += p->D;

		D /= m;

		for (s = 0.0, p = LM_SEL; p != NULL; p = p->Next) {
			// This is the weight
			W = exp (-((p->D/D) * PM_ave_fac));
			// Accumulate
			s += W;
			x += p->P->x * W;
			y += p->P->y * W;
#if DEBUGGING
			printf ("EWEIGHT: %g, %1.2f->%1.2f, %1.2f->%1.2f, "
				"%g\n", W, p->P->x, x, p->P->y, y, s);
#endif
		}
	}

	if (s < EPS) {
#if DEBUGGING
		printf ("ZERO WEIGHTS!!\n");
#endif
		// The very special case for which we have to prepare
		// ourselves, as quite likely we will be playing with
		// various coarse schemes where Di = 0 may be likely,
		// possibly for multiple points
		x = y = 0.0;
		for (p = LM_SEL; p != NULL; p = p->Next) {
			x += p->P->x;
			y += p->P->y;
		}
		s = (double) LM_N;
	}

	*X = (float) (x / s);
	*Y = (float) (y / s);
#if DEBUGGING
	printf ("ESTIMATE %1.2f, %1.2f\n", *X, *Y);
#endif
	// Free memory
	pdeltop (i = LM_N);

	if (LM_SEL != NULL)
		abt ("loc_locate internal error: LM_SEL != NULL");

	return i;
}

// ============================================================================

float rssi_to_slr (u16 rssi) {

	int i;

	if (PM_rts_n < 2) {
		// There's no table -> no translation
		return (float) rssi;
	}

	// Find the interval
	for (i = 0; i < PM_rts_n; i++)
		if (rssi <= PM_rts_a [i])
			break;

	if (i == 0)
		return PM_rts_v [0];

	if (i == PM_rts_n)
		return PM_rts_v [PM_rts_n - 1];

	// Interpolate
	return PM_rts_v [i-1] + (PM_rts_v [i] - PM_rts_v [i-1]) *
		(((double)(rssi - PM_rts_a [i-1])) /
			(PM_rts_a [i] - PM_rts_a [i-1]));
}
