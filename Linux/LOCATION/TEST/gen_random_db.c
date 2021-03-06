/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "locengine.h"


#define	MAXPEGS		128
//#define	DEBUGGING	1

typedef struct {
	float x, y;
	u32 Tag;
} pp_t;

double	XL, YL, XH, YH, Range, Redundancy, Fuzziness, Delta;
int NPoints, NAuto;

int CNPegsS, CNPegs = 0, CNPointsS, CNPoints = 0;

pp_t *PPegs, *PPoints;

float dtorank (double dist) {
//
// Converts distance to the rank, i.e., the equivalent of RSSI reading
//
	if (dist > Range)
		return (float) (drand48 () * 2.0);

	if (dist < 0.5)
		return (float) (250.0 + drand48 () * 5.0);

	return (float) ((1.0 - (Range - dist) / Range) * 250.0);
}

double fuzzy (double d) {

	return d * (1.0 + (drand48 () - 0.5) * Fuzziness);
}

void add_peg (double x, double y) {

	if (CNPegs == 0) {
		PPegs = (pp_t*) malloc ((CNPegsS = 1024) * sizeof (pp_t));
	} else if (CNPegs == CNPegsS) {
		PPegs = (pp_t*) realloc (PPegs,
					(CNPegsS += CNPegs) * sizeof (pp_t));
	}

	PPegs [CNPegs] . x = (float) x;
	PPegs [CNPegs] . y = (float) y;
	CNPegs++;
#if DEBUGGING
	printf ("New peg: %1d <%f,%f>\n", CNPegs, x, y); fflush (stdout);
#endif
}

void add_point (double x, double y, u32 tag) {

	if (CNPoints == 0) {
		PPoints = (pp_t*) malloc ((CNPointsS = 1024) * sizeof (pp_t));
	} else if (CNPoints == CNPointsS) {
		PPoints = (pp_t*) realloc (PPoints,
				(CNPointsS += CNPoints) * sizeof (pp_t));
	}

	PPoints [CNPoints] . x = x;
	PPoints [CNPoints] . y = y;
	PPoints [CNPoints] . Tag = tag;
#if DEBUGGING
	printf ("New point: %1d <%f,%f>\n", CNPoints, x, y); fflush (stdout);
#endif
	CNPoints++;
}

int neighbors (double x, double y) {
//
// Calculates how many neighbors (points within the transmission range)
// the point has
//
	int i, N;

	for (N = i = 0; i < CNPoints; i++)
		if (dist (PPoints [i].x, PPoints [i].y, x, y) <= Delta)
			N++;

	return N;
}

void create_pegs () {

	double x, y, dx, dy;
	int nx, ny, i, j;

	// Cover the perimeter first

	for (nx = 1; (dx = (XH - XL) / nx) > Delta; nx++);
	for (ny = 1; (dy = (YH - YL) / ny) > Delta; ny++);

	nx++; ny++;

	for (y = YL, i = 0; i < ny; i++, y += dy)
		for (x = XL, j = 0; j < nx; j++, x += dx)
			add_peg (x, y);
}

void create_points () {

	double	X, Y, T;
	int i, j;

	T = 0.5;

	for (i = 0; i < NPoints; i++) {
#if 0
		// Try 10 times at random locations and pick the one with the
		// smallest coverage
		M = 10000000;
		for (j = 0; j < 10; j++) {
			x = XL + (XH - XL) * drand48 ();
			y = YL + (YH - YL) * drand48 ();
			if ((m = neighbors (x, y)) < M) {
				M = m;
				X = x;
				Y = y;
			}
		}
#else
		X = XL + (XH - XL) * drand48 ();
		Y = YL + (YH - YL) * drand48 ();
#endif

		if (X - XL < T)
			X = XL;

		if (XH - X < T)
			X = XH;

		if (Y - YL < T)
			Y = YL;

		if (YH - Y < T)
			Y = YH;

		add_point (X, Y, MAXPEGS + MAXPEGS);
	}

	if (NAuto == 0)
		return;

	// Create autoprofile points
	for (i = 0; i < CNPegs; i++) {
		for (j = 0; j < NAuto; j++)
			add_point (PPegs [i].x, PPegs [i].y, i+1);
	}
}

void make_database () {

	double x, y, D;
	tpoint_t *TP;
	alitem_t PI [MAXPEGS];
	int i, j, k;
	u32 tag;

	db_open (DBNAME, PMNAME);

	for (i = 0; i < CNPoints; i++) {
		x = PPoints [i] . x;
		y = PPoints [i] . y;
		tag = PPoints [i] . Tag;
		for (k = j = 0; j < CNPegs; j++) {
			if (j+1 == tag)
				// Autosample for this Peg
				continue;
			D = fuzzy (dist (PPegs [j].x, PPegs [j].y, x, y));
			if (D > Range)
				continue;
			if (k == MAXPEGS) {
				fprintf (stderr, "make_database: too many pegs"
					" per point\n");
				exit (99);
			}
			PI [k] . Peg = (u32) (j+1);
			PI [k] . RSSI = (u16) (dtorank (D) + 0.5);
			k++;
		}
		if (k < 3) {
#if DEBUGGING
			printf ("<%f,%f>: %1d < 3 pegs\n", x, y, k);
#endif
			continue;
		}

		TP = (tpoint_t*) malloc (tpoint_tsize (k));
		TP->x = x;
		TP->y = y;
		TP->Tag = tag;
		TP->properties = (tag <= MAXPEGS) ? PROP_AUTOPROF : 0;
		TP->NPegs = k;
		bcopy (PI, TP->Pegs, sizeof (alitem_t) * k);
		db_add_point (TP);
	}

	db_close ();
}
			
int main () {

	printf ("Rectangle coordinates (XL, YL, XH, YH) in meters:\n");
	scanf ("%lf %lf %lf %lf", &XL, &YL, &XH, &YH);

	printf ("Transmission range in meters:\n");
	scanf ("%lf", &Range);

	printf ("Coverage redundancy (min = 0.0):\n");
	scanf ("%lf", &Redundancy);

	printf ("Number of autoprofile samples per peg:\n");
	scanf ("%d", &NAuto);

	printf ("Fuzziness (min = 0.0, max = 1.0):\n");
	scanf ("%lf", &Fuzziness);

	printf ("Number of points:\n");
	scanf ("%d", &NPoints);

	// This determines the density of Peg deployment
	Delta = Range / (1.0 + Redundancy);

	printf ("Rectangle: [%f, %f, %f, %f]\n", XL, YL, XH, YH);
	printf ("Range = %f, Redundancy = %f, Fuzziness = %f\n",
		Range, Redundancy, Fuzziness);

	printf ("Number of points: %d\n", NPoints);

	srand48 (time (NULL));

	create_pegs ();

	printf ("Number of pegs: %d\n", CNPegs);

	create_points ();

	make_database ();

	exit (0);
}
