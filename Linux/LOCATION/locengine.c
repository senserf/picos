#include <stdio.h>
#include <string.h>
#include "locengine.h"

//
// Copyright (C) 2008 Olsonet Communications Corporation
//
// PG March 2008
//

#ifdef DEBUGGING

static void dump_point (tpoint_t *p) {

	int i;

	printf ("*** [%1.2f, %1.2f] (%08x) | %1d:", 
		p->x, p->y, p->properties, p->NPegs);

	for (i = 0; i < p->NPegs; i++) {
		printf (" <%1d,%3.0f>", p->Pegs [i] . Peg, p->Pegs [i] . SLR);
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

typedef struct {
//
// Describes a single point match: coordinates of the points + discrepancy
//
	float	x, y;	// Point coordinates
	float	D;	// Discrepancy

} lmatch_t;

typedef struct {
//
// Used for selecting K best points: this is what has to be sorted
//
	tpoint_t *P;
	float    D;	// Discrepancy

} lskey_t;

static lskey_t *LM_SEL;	// Objects to select from
static int K_SEL;	// How many to select

static float pdist (tpoint_t *P, u32 *pegs, float *vs, int N) {
//
// Calculates the discrepancy between the point and the Tag's association list
//
	int i, NP, ll, rr, mid;
	double S, Delta;
	pegitem_t *PI;
	u32 CP, pp;

	PI = P->Pegs;
	NP = P->NPegs;

	S = 0.0;
	for (i = 0; i < N; i++) {
		// Current peg
		CP = pegs [i];
		// Find it on the point's list
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
		// Not found: assume zero reference, i.e., the outcome is the
		// same as if the peg would perceive the point at SLR = 0
		S += (double) vs [i] * vs [i];
		continue;
Found:
		Delta = (double) vs [i] - PI [mid] . SLR;
		S += Delta * Delta;
	}

	// If there are any Pegs that perceived this point, which are not on
	// the list, we ignore them: packets can be lost

	return (float) (sqrt (S) / N);
}

static void selK (int lo, int up) {
//
// Select K_SEL best points (ones with the smallest D)
//
	int k, l;
	lskey_t m;

	while (up > lo) {
		k = lo;
		l = up;
		m = LM_SEL [lo];
		while (k < l) {
			while (LM_SEL [l].D > m.D)
				l--;
			LM_SEL [k] = LM_SEL [l];
			while (k < l && LM_SEL [k].D <= m.D)
				k++;
			LM_SEL [l] = LM_SEL [k];
		}
		LM_SEL [k] = m;
		selK (lo, k-1);
		if (k >= K_SEL)
			return;
		lo = k+1;
	}
}

static int best_points (u32 *pg, float *v, int N, u32 prp, lmatch_t *r, int K) {
//
// Scans the points (db_stat_points has been called previously to select all
// points perceptible by one specific Peg) for K best matches
//
	int n, S;
	tpoint_t *TP;

	LM_SEL = (lskey_t*) malloc (sizeof (lskey_t) * (S = 128));
	if (LM_SEL == NULL) {
Mem:
		fprintf (stderr, "best_points: out of memory\n");
		exit (99);
	}

	n = 0;
	do {
		if ((TP = db_get_point ()) == NULL)
			break;

		if (TP->properties == prp) {
			if (n == S) {
				LM_SEL = (lskey_t*) realloc (LM_SEL,
					(S = S + S) * sizeof (lskey_t));
				if (LM_SEL == NULL)
					goto Mem;
			}

			LM_SEL [n] . P = TP;
			LM_SEL [n] . D = pdist (TP, pg, v, N);
#ifdef DEBUGGING
			printf ("Discrepancy: %f\n", LM_SEL [n] . D);
			dump_point (TP);
#endif
			n++;
		}
		db_next_point ();

	} while (1);

	if (n > 1) {
		// Select
		K_SEL = K;
		selK (0, n-1);
	}

	if (K > n)
		K = n;

	for (S = 0; S < K; S++) {
		TP = LM_SEL [S] . P;
		r [S] . x = TP->x;
		r [S] . y = TP->y;
		r [S] . D = LM_SEL [S] . D;
	}

	free (LM_SEL);
	return K;
}

#define	MAXSELPOINTS	128

int loc_locate (int K, u32 *pg, float *v, int N, u32 prop, float *X, float *Y) {
//
// Given a Tag reading in terms of the list of pegs (pg) and the respective 
// SLR values (v), N of each, plus the property set prop, returns the estimated
// coordinates of the Tag (via X, Y)
//
	lmatch_t match [MAXSELPOINTS];
	int NB, i, j, ix;
	u32 pt;
	float vt;
	double S, F, M;

	// A sanity check
	if (K > MAXSELPOINTS) {
		fprintf (stderr, "loc_locate: K is too large, %1d is max\n",
			MAXSELPOINTS);
		exit (99);
	}

	if (K < 1)
		// Make sure there is at least one point
		K = 1;

	// Sort the pegs from the closest to the furthest (there is just a
	// handful of them, so no need to be smart)

	for (i = 0; i < N - 1; i++) {
		vt = v [i];
		ix = i;
		for (j = i+1; j < N; j++) {
			if (v [j] > vt) {
				vt = v [j];
				ix = j;
			}
		}
		// Replace
		vt = v [i];
		pt = pg [i];
		v [i] = v [ix];
		pg [i] = pg [ix];
		v [ix] = vt;
		pg [ix] = pt;
	}
#ifdef DEBUGGING
	printf ("loc_locate: %1d pegs, best %1d\n", N, pg [0]);
#endif
	// For now, we shall just grab the first peg on the list and match
	// to the list of points that it can hear (so the above sort is not
	// really needed, except for the minimum)

	db_start_points (pg [0]);

	NB = best_points (pg, v, N, prop, match, K);
#ifdef DEBUGGING
	printf ("found %1d best points [%1d]\n", NB, K);
#endif
	*X = 0.0;
	*Y = 0.0;

	if (NB <= 1) {
		// Not much we can do
		if (NB == 1) {
			*X = match [0] . x;
			*Y = match [0] . y;
		}
		return NB;
	}
	
	M = S = 0.0;
	for (i = 0; i < NB; i++) {
		// Sum and max of discrepancies
		if ((F = (double) (match [i] . D)) > M)
			M = F;
		S += F;
	}
#ifdef DEBUGGING
	printf ("S M S = %f / %f / %f\n", S, M, M * NB - S);
#endif
	S = M * NB - S;
	for (i = 0; i < NB; i++) {
		if (S < 0.00001)
			F = 1.0 / NB;
		else
			F = (M - match [i] . D) / S;
		*X += (float) (match [i] . x * F);
		*Y += (float) (match [i] . y * F);
	}

	return NB;
}
