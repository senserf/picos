/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#define	MAXPTBSIZE	128

typedef	unsigned char	Boolean;

#define	YES		1
#define	NO		0

#define	dBToLin(v)	pow (10.0, (double)(v) / 10.0)
#define	linTodB(v)	(log10 (v) * 10.0)

typedef struct { int S, D; } edge_t;

template <class RType> class XVMapper {

	// This is a generic converter between model parameters (like signal
	// levels or bit rates) and their representations for the praxis.
	// For example, there may be 8 discrete power level setting
	// represented by numbers from 0 to 7 that should be mapped into
	// FP multipliers. The converter is able to do it both ways, i.e.,
	// convert discrete representations to the actual values, as well
	// as convert the values to their discrete representations.

	Boolean Dec,		// Decreasing order of values
		Log;		// Logarithmic
	unsigned short NL;	// Number of levels
	RType *VLV;		// Representations
	double *SLV;		// Values
	double *FAC;		// To speed up interpolation

	inline Boolean vtor (double a, double b) {
		// Returns YES if b's representation is larger than a's, i.e.,
		// b is to the "right" of a 
		return (Dec && (a >= b)) || (!Dec && (a <= b));
	}

	public:

	XVMapper (unsigned short n, RType *wt, double *dt, Boolean lg = NO) {

		int i;

		NL = n;
		VLV = wt;

		if (n < 2)
			Dec = 0;
		else
			Dec = (dt [1] < dt [0]);
		Log = lg;

		// Interpolation
		SLV = dt;
		if (NL > 1) {
			FAC = new double [NL - 1];
			for (i = 0; i < NL-1; i++)
				FAC [i] = (double) (SLV [i+1] - SLV [i]) /
					(VLV [i+1] - VLV [i]);
		} else {
			FAC = NULL;
		}
	};

	unsigned short size () { return NL; };

	RType row (int n, double &v) {
		v = SLV [n];
		return VLV [n];
	};

	double setvalue (RType w, Boolean lin = YES) {
	//
	// Converts representation to value
	//
		double d;
		int a, b, ix;

		a = 0; b = NL;

		do {
			ix = (a + b) >> 1;

			if (VLV [ix] <= w) {

				if (ix+1 == NL) {
					d = SLV [ix];
					goto Ret;
				}

				if (VLV [ix+1] <= w) {
					// Go to right
					a = ix+1;
					continue;
				}
				// Interpolate and return
				break;
			}

			if (ix == 0) {
				// At the beginning
				d = SLV [0];
				goto Ret;
			}

			if (VLV [ix-1] > w) {
				// Go to left
				b = ix;
				continue;
			}

			// Interpolate and return
			ix--;
			break;

		} while (1);

		// Interpolate
		d = ((w - VLV [ix]) * FAC [ix]) + SLV [ix];
	Ret:
		return (Log && lin) ? dBToLin (d) : d;
	};

	RType getvalue (double v, Boolean db = YES) {
	//
	// Converts value to representation
	//
		int a, b, ix;

		a = 0; b = NL;

		if (Log && db)
			v = linTodB (v);

		do {
			ix = (a + b) >> 1;

			if (vtor (SLV [ix], v)) {
				if (ix+1 == NL)
					// At the end
					return VLV [ix];

				if (vtor (SLV [ix+1], v)) {
					// Go to right
					a = ix+1;
					continue;
				}
				// Interpolate and return
				break;
			}

			if (ix == 0)
				// At the beginning
				return VLV [0];

			if (!vtor (SLV [ix-1], v)) {
				// Go to left
				b = ix;
				continue;
			}

			// Interpolate and return
			ix--;
			break;

		} while (1);

		return (RType) ((v - SLV [ix]) / FAC [ix]) + VLV [ix];
	};

	Boolean exact (RType w) {
	//
	// Checks if the representation is in range, i.e., between min and
	// max
	//
		int a, b, ix;

		a = 0; b = NL;

		do {
			ix = (a + b) >> 1;

			if (VLV [ix] == w)
				return YES;

			if (VLV [ix] < w) {
				// Go to the right
				if (ix + 1 == NL || VLV [ix + 1] > w)
					return NO;

				a = ix + 1;
				continue;
			}

			// Go to the left
			if (ix == 0 || VLV [ix - 1] < w)
				return NO;

			b = ix;

		} while (1);
	};

	Boolean inrange (RType w) {

		if (NL == 1)
			return (w == VLV [0]);

		return (w >= lower () && w <= upper ());
	};

	// Return the limits on the index
	RType lower () { return VLV [0]; };
	RType upper () { return VLV [NL-1]; };
};

typedef	XVMapper<unsigned short>	IVMapper;
typedef	XVMapper<double>		DVMapper;

// ============================================================================

DVMapper *Prop;

double	XL, YL, XH, YH, W, H;
double	Grid;
double	GCover;
double	MinDev, MaxDev;
double	MinDis, MaxDis;

int	Edges;
int	MinOvr, MaxOvr;
int	NX, NY;
int	Symmetric;

int	*Points;

// ============================================================================

unsigned short Seeds [3];

static inline double rnd () { return erand48 (Seeds); };

static inline int irnd (int a, int b) {

	int res;

	return (int) (a + (b - a + 1) * rnd ());
}

// ============================================================================

static void abt (const char *m, ...) {

	va_list pmts;

	va_start (pmts, m);

	vfprintf (stderr, m, pmts);
	fprintf (stderr, "\n");
	exit (99);
}

// ============================================================================

static void process_edge (int s, int d) {

	int		p, nx, ny;
	double		XA, YA, XB, YB, dst, dx, dy, RS;

	fprintf (stderr, "Edge %1d %1d\n", s, d);

	p = Points [s];

	ny = p / NX;
	nx = p % NX;
	XA = XL + Grid * nx;
	YA = YL + Grid * ny;

	p = Points [d];

	ny = p / NX;
	nx = p % NX;
	XB = XL + Grid * nx;
	YB = YL + Grid * ny;

	dx = XA - XB;
	dy = YA - YB;

	// Distance
	dst = sqrt (dx * dx + dy * dy);

	if (dst > Prop->upper ())
		// Too far
		return;

	RS = Prop->setvalue (dst);

	// Apply deviation
	RS *= (1.0 + MinDev + rnd () * (MaxDev - MinDev));

	if (RS < 0.0)
		RS = 0.0;
	else if (RS > 255.0)
		RS = 255.0;

	// Number of samples
	nx = irnd (MinOvr, MaxOvr);

	printf ("# D=%1.3f, R=%1.3f, N=%1d\n", dst, RS, nx);

	while (nx--) {
		dst = RS * (1.0 + MinDis + rnd () * (MaxDis - MinDis));
		if (dst < 0.0)
			dst = 0.0;
		else if (dst > 255.0)
			dst = 255.0;
		printf ("%1.5f %1.5f %1.5f %1.5f %1d\n", XA, YA, XB, YB,
			(int) dst);
	}
}

static void generate () {

	double d;
	int i, k, nt, np, a, b;
	edge_t *Used;

	// Number of times the grid will fit into width
	NX = (int) (W / Grid);

	if (NX < 1) {
TooLarge:
		abt ("Grid unit %g too large", Grid);
	}

	// Calculate horizontal offset
	d = (W - NX * Grid) / 2.0;
	XL += d;
	XH -= d;
	W = XH - XL;

	// Calculate vertical offset
	NY = (int) (H / Grid);

	if (NY < 1)
		goto TooLarge;

	d = (H - NY * Grid) / 2.0;
	YL += d;
	YH -= d;
	H = YH - YL;

	NX++;
	NY++;

	// ====================================================================

	nt = NX * NY;
	np = nt * GCover;

	fprintf (stderr, "Points: %1d/%1d\n", nt, np);

	Points = new int [np];

	for (k = i = 0; i < nt; i++) {
		d = (double) (np - k) / (double) (nt - i);
		if (rnd () < d)
			Points [k++] = i;
	}

	np = k;
			
	fprintf (stderr, "Trimmed down to: %1d\n", np);

	if (np < 2)
		abt ("Can do nothing with a single point");

	// The maximum number of edges
	nt = np * (np - 1);

	if (Symmetric)
		nt /= 2;

	fprintf (stderr, "Edges: %1d/%1d\n", nt, Edges);

	if (Edges > nt)
		abt ("Requested number of edges exceeds the possibilities");

	Used = new edge_t [Edges];

	for (k = 0; k < Edges; k++) {
		do {
			a = irnd (0, np-1);
			do {
				b = irnd (0, np-1);
			} while (b == a);
			// Check if already included
			for (i = 0; i < k; i++) {
				if (Used [i] . S == a && Used [i] . D == b)
					goto Next;
				if (Symmetric &&
				    Used [i] . S == b && Used [i] . D == a)
					goto Next;
			}
			break;
Next:			continue;
		} while (1);

		process_edge (a, b);
		Used [k] . S = a;
		Used [k] . D = b;
	}

	delete Used;
}

int main () {

	double	*at, *bt;
	int 	i, n;
	struct	timeval Tm;

	fprintf (stderr, "Rectangle coordinates (XL, YL, XH, YH) in meters:\n");
	scanf ("%lf %lf %lf %lf", &XL, &YL, &XH, &YH);

	W = XH - XL;
	H = YH - YL;

	if (W <= 0.0 || H <= 0.0)
		abt ("Illegal coordinates, must be XH > XL and YH > YL");

	fprintf (stderr, "Symmetric (0|1):\n");
	scanf ("%d", &Symmetric);

	fprintf (stderr, "Grid unit in meters:\n");
	scanf ("%lf", &Grid);

	if (Grid > W || Grid > H)
		abt ("Grid unit %g is larger than area dimension %g, %g",
			Grid, W, H);

	fprintf (stderr, "Grid coverage by points (percent):\n");
	scanf ("%lf", &GCover);

	if (GCover <= 0.0 || GCover > 100.0)
		abt ("Grid coverage must be a percentage > 0, is %g", GCover);

	fprintf (stderr, "Number of edges:\n");
	scanf ("%d", &Edges);

	if (Edges < 0)
		abt ("Number of edges must be > 0, is %1d", Edges);

	fprintf (stderr, "Readings per sample (min, max):\n");
	scanf ("%d %d", &MinOvr, &MaxOvr);

	if (MinOvr < 1 || MinOvr > MaxOvr)
		abt ("Illegal oversampling parameters %1d, %1d, min must be "
			">= 1 and max <= min", MinOvr, MaxOvr);

	fprintf (stderr, "Deviation (min, max) per S-D pair (percent):\n");
	scanf ("%lf %lf", &MinDev, &MaxDev);

	if (MinDev < -100.0 || MinDev > MaxDev || MaxDev > 100.0)
		abt ("Illegal deviation %g, %g, must be +-percentage "
			"max >= min", MinDev, MaxDev);

	fprintf (stderr, "Discrepancy (min, max) per single sample:\n");
	scanf ("%lf %lf", &MinDis, &MaxDis);

	if (MinDis < -100.0 || MinDis > MaxDis || MaxDis > 100.0)
		abt ("Illegal discrepancy %g, %g, must be +-percentage "
			"max >= min", MinDis, MaxDis);

	fprintf (stderr, "Number of entries in propagation table:\n");
	scanf ("%d", &n);

	if (n <= 0 || n > MAXPTBSIZE)
		abt ("Illegal number of entries in propagation table %1d, "
			"must be > 0 and <= %1d", n, MAXPTBSIZE);

	fprintf (stderr, "Propagation table (%1d number pairs: dist RSSI):\n");

	at = new double [n];
	bt = new double [n];

	for (i = 0; i < n; i++) {
		scanf ("%lf %lf", at + i, bt + i);
		if (at [i] < 0.0)
			abt ("Illegal distance in propagation table %1d, "
				"cannot be negative", at [i]);

		if (i > 0) {
			if (at [i] <= at [i - 1])
				abt ("Distances in propagation table must be "
					"strictly increasing, see %g and %g",
						at [i-1], at [i]);
			if (bt [i] >= bt [i - 1])
				abt ("Signal levels in propagation table must "
					"be strictly decreasing, see %g and %g",
						bt [i-1], bt [i]);
		}
	}

	// Copy the values to the standard output

	gettimeofday (&Tm, NULL);

	Seeds [0] = Tm.tv_sec & 0xFFFF;
	Seeds [1] = (Tm.tv_sec >> 16) & 0xFFFF;
	Seeds [2] = Tm.tv_usec & 0xFFFF;

	printf ("#\n# Generated on %s", ctime (&(Tm.tv_sec)));
	printf ("#\n");
	printf ("#    Rectangle:             %1.3f %1.3f %1.3f %1.3f",
			XL, YL, XH, YH);
	if (Symmetric)
		printf (" (symmetric)");
	printf ("\n");

	printf ("#    Grid unit:             %1.5f\n", Grid);
	printf ("#    Grid covergage:        %1.2f\%\n", GCover);
	printf ("#    Number of edges:       %1d\n", Edges);
	printf ("#    Readings per sample:   %1d %1d\n", MinOvr, MaxOvr);
	printf ("#    Deviation:             %1.2f %1.2f\n",
						MinDev, MaxDev);
	printf ("#    Discrepancy:           %1.2f %1.2f\n",
						MinDis, MaxDis);
	printf ("#\n");
	printf ("#    Propagation table:\n");

	for (i = 0; i < n; i++)
		printf ("#      %10.3f  %5g\n", at [i], bt [i]);

	Prop = new DVMapper (n, at, bt);

	printf ("#\n");

	GCover /= 100.0;
	MinDev /= 100.0;
	MaxDev /= 100.0;
	MinDis /= 100.0;
	MaxDis /= 100.0;

	generate ();
}
