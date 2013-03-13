#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "locengine.h"

//
// Copyright (C) 2008 Olsonet Communications Corporation
//
// PG March 2008
//

static	char	*LINE, *LP;
static	size_t	LINE_size;

static	tpoint_t **PBUFFER;	// Buffer for recently viewed points
static	size_t	 PBUFFER_size;	// Size of PBUFFER
static	int	NPB;		// Number of points in PBUFFER

#define	iss(p)	isspace ((int)(unsigned char) (p))
#define	isa(p)	isalpha ((int)(unsigned char) (p))
#define	skb(p)	do { while (iss (*p)) p++; } while (0)
#define	ska(p)	do { while (isa (*p)) p++; } while (0)

static int g_int (u32 *v) {
//
// Reads an integer from the input line
//
	char *rp;
	while (iss (*LP) || *LP == ',' || *LP == '<' || *LP == '>')
		LP++;

	*v = (u32) strtol (LP, &rp, 0);

	if (rp == LP)
		return -1;

	LP = rp;
	return 0;
}

static int g_float (float *v) {
//
// Reads a float number from the character string
//
	char *rp;
	while (iss (*LP) || *LP == ',' || *LP == '<' || *LP == '>')
		LP++;

	*v = (float) strtod (LP, &rp);

	if (rp == LP)
		return -1;

	LP = rp;
	return 0;
}

static void list_point (int pn) {
//
// List one point
//
	int i;
	tpoint_t *p;

	p = PBUFFER [pn];

	printf ("*%1d [%1.2f, %1.2f] <%1u> (%08x) | %1d:", 
		pn+1, p->x, p->y, p->Tag, p->properties, p->NPegs);

	for (i = 0; i < p->NPegs; i++) {
		printf (" <%1d,%1d,%3.0f>",
			p->Pegs [i] . Peg,
			p->Pegs [i] . RSSI,
			p->Pegs [i] . SLR);
	}

	printf ("\n---\n");
}

static void list_pbuffer () {
//
// List the contents of PBUFFER
//
	int i;

	if (PBUFFER == NULL || NPB == 0) {
		printf ("No points to list\n");
		return;
	}

	if (NPB == 1)
		printf ("One point:\n");
	else
		printf ("%1d points:\n", NPB);

	for (i = 0; i < NPB; i++)
		list_point (i);
}

static int init_pbuffer () {
//
// Resets the point buffer
//
	if (PBUFFER != NULL)
		free (PBUFFER);

	PBUFFER_size = 128;
	NPB = 0;
	return ((PBUFFER = (tpoint_t**) malloc (PBUFFER_size *
						sizeof (tpoint_t*))) == NULL);
}

static int reinit_pbuffer (int n) {
//
// Reallocate the point buffer
//
	tpoint_t **tq;

	tq = (tpoint_t**) realloc (PBUFFER,
				(PBUFFER_size = n) * sizeof (tpoint_t*));
	if (tq == NULL) {
		free (PBUFFER);
		PBUFFER = NULL;
		NPB = 0;
		return 1;
	}
	PBUFFER = tq;
	return 0;
}
	
// ============================================================================

void cmd_show () {
//
// Display PBUFFERR
//
	list_pbuffer ();
}

void cmd_delete () {
//
// List of point numbers (w.r.t PBUFFER)
//
	u32 *pts, n, i, pn;
	tpoint_t *tp;

	if (NPB == 0) {
		printf ("No points in buffer\n");
		return;
	}
	if ((pts = (u32*)malloc (NPB * sizeof (u32))) == NULL) {
		printf ("Out of memory\n");
		return;
	}

	n = 0;
	while (g_int (&pn) == 0) {
		for (i = 0; i < n; i++) {
			if (pts [i] == pn) {
				printf ("Point numbers are not distinct\n");
ERet:
				free (pts);
				return;
			}
		}
		if (pn > NPB) {
			printf ("No such point: %1d\n", pn);
			goto ERet;
		}
		pts [n++] = pn;
	}

	for (i = 0; i < n; i++) {
		pn = pts [i] - 1;
		// Numbered from 1
		tp = PBUFFER [pn];
		printf ("Deleting point:\n");
		list_point (pn);
		db_delete_point (tp);
		PBUFFER [pn] = NULL;
	}

	// Remove holes from PBUFFER
	for (i = n = 0; i < NPB; i++)
		if (PBUFFER [i] != NULL)
			PBUFFER [n++] = PBUFFER [i];

	NPB = n;
}

void cmd_add () {
//
// Add a point
//
	tpoint_t *tp;
	float	x, y;
	u32 	pr, t, pg, tag;
	int	i, j;

	if (g_float (&x) || g_float (&y) || g_int (&tag) || g_int (&pr) ||
	    g_int (&t)) {
Error:
		printf ("Illegal arguments, must be: "
				"x y tag prop npegs <p v>*\n");
		return;
	}

	tp = (tpoint_t*) malloc (tpoint_tsize (t));
	if (tp == NULL) {
		printf ("Out of memory\n");
		return;
	}

	tp->x = x; tp->y = y; tp->Tag = tag; tp->properties = pr;
	tp->NPegs = (u16) t;

	for (i = 0; i < t; i++) {

		if (g_int (&pg)) {
			free (tp);
			goto Error;
		}

		tp->Pegs [i] . Peg = pg;
		for (j = 0; j < i; j++)
			if (tp->Pegs [j] . Peg == pg) {
				printf ("Duplicate peg %1d\n", pg);
				free (tp);
				return;
			}
		if (g_int (&t)) {
			free (tp);
			goto Error;
		}
		tp->Pegs [i] . RSSI = (u16) t;
	}

	db_add_point (tp);
}

void cmd_find () {
// 
// Find points:
//
//		x y		= single point closest to x, y
//		x y r		= all points within radius r from x, y
//		x y x1 y1	= all within the rectangle
//
	int 		n;
	float		x0, y0, x1, y1;
	double		d, dd;
	tpoint_t	*tp, *fp;

	if (g_float (&x0) || g_float (&y0)) {
		// At least two coordinates expected
		printf (
		 "Illegal arguments, must be: x y, or x y r, or x0 y0 x1 y1\n");
		return;
	}

	if (g_float (&x1)) {
		// Single point closest to x y
		if (init_pbuffer ()) {
OOM:
			printf ("Out of memory\n");
			return;
		}
		db_start_points (0);
		fp = NULL;
		while ((tp = db_get_point ()) != NULL) {
			if (fp == NULL) {
				fp = tp;
				dd = dist (x0, y0, tp->x, tp->y);
			} else {
				if ((d = dist (x0, y0, tp->x, tp->y)) < dd) {
					fp = tp;
					dd = d;
				}
			}
			db_next_point ();
		}
		PBUFFER [0] = fp;
		NPB = 1;
	} else if (g_float (&y1)) {
		// All within radius
		if (init_pbuffer ())
			goto OOM;

		db_start_points (0);
		NPB = 0;
		while ((tp = db_get_point ()) != NULL) {
			if (dist (x0, y0, tp->x, tp->y) <= x1) {
				if (NPB == PBUFFER_size &&
					reinit_pbuffer (PBUFFER_size +
						PBUFFER_size))
							goto OOM;
				PBUFFER [NPB++] = tp;
			}
			db_next_point ();
		}
	} else {
		// Within rectangle
		if (x0 > x1 || y0 > y1) {
			printf (
			    "Illegal rectangle, must be x0 >= x1, y0 >= y1\n");
			return;
		}

		if (init_pbuffer ())
			goto OOM;

		while (1) {
			n = loc_findrect (x0, y0, x1, y1, PBUFFER,
				PBUFFER_size);
			if (n <= PBUFFER_size)
				break;

			if (reinit_pbuffer (n))
				goto OOM;
		}

		NPB = n;
	}
	list_pbuffer ();
}

void cmd_peg () {
//
// List the points including the given peg in the association list
//
	u32 		p;
	tpoint_t	*tp;

	if (g_int (&p)) {
		printf ("Illegal argument, should be Peg number\n");
		return;
	}

	db_start_points (p);

	if (init_pbuffer ()) {
OOM:
		printf ("Out of memory\n");
		return;
	}

	NPB = 0;

	while ((tp = db_get_point ()) != NULL) {
		if (NPB == PBUFFER_size &&
			reinit_pbuffer (PBUFFER_size + PBUFFER_size))
				goto OOM;
		PBUFFER [NPB++] = tp;
		db_next_point ();
	}

	list_pbuffer ();
}

void cmd_locate () {
//
// Estimate location based on the specified association list
//
	u32		n, prop, tag, peg, rssi;
	int		i, N;
	float 		x, y;
	alitem_t	*al;

	if (g_int (&tag) || g_int (&prop) || g_int (&n)) {
Error:
		printf ("Illegal arguments, should be tag prop N <p v>*\n");
		return;
	}

	N = (int) n;

	if (N < 1 || N > 128) {
		printf ("Illegal number of pegs, must be > 1 and < 129\n");
		return;
	}

	if ((al = (alitem_t*) malloc (N * sizeof (alitem_t))) == NULL) {
		printf ("Out of memory\n");
		return;
	}

	for (i = 0; i < N; i++) {
		if (g_int (&peg) || g_int (&rssi)) {
			free (al);
			goto Error;
		}
		al [i] . Peg = peg;
		al [i] . RSSI = (u16) rssi;
		// Just in case
		al [i] . SLR = 0.0;
	}

	i = loc_locate (tag, al, N, prop, &x, &y);
	free (al);

	printf ("Location: [%7.2f %7.2f] (%1d)\n", x, y, i);
}

void cmd_params () {
//
// Show parameters and statistics
//
	int i, n, minal, maxal, na;
	double sal;
	tpoint_t *tp;

	printf ("DB version: %1d\n", PM_dbver);
	printf ("RSSI->SLR table:");
	for (i = 0; i < PM_rts_n; i++)
		printf (" <%1d,%1.2f>", PM_rts_a [i], PM_rts_v [i]);
	printf ("\n");
	printf ("Minimum match dimension: %1d\n", PM_dis_min);
	printf ("Long match preference factor: %1.2f\n", PM_dis_fac);
	printf ("Tag RSSI/SLR: %1d/%1.2f\n", PM_dis_tag, PM_dis_taf);
	printf ("Minimum number of points: %1d\n", PM_sel_min);
	printf ("Maximum number of points: %1d\n", PM_sel_max);
	printf ("Trim factor: %1.2f\n", PM_sel_fac);
	printf ("Averaging formula: %s\n", PM_ave_for ? "exp" : "lin");
	printf ("Proximity factor: %1.2f\n", PM_ave_fac);

	// Calculate the number of points in the database
	db_start_points (0);

	minal = 9999;
	maxal = 0;
	sal = 0.0;
	n = na = 0;

	while ((tp = db_get_point ()) != NULL) {
		n++;
		sal += tp->NPegs;
		if (tp->NPegs > maxal)
			maxal = tp->NPegs;
		if (tp->NPegs < minal)
			minal = tp->NPegs;
		if ((tp->properties & PROP_AUTOPROF) != 0)
			na++;
		db_next_point ();
	}

	printf ("Number of points in DB: %1d\n", n);
	if (n == 0)
		return;
	printf ("Autoprofiled points: %1d\n", na);
	printf ("Minimum AL length: %1d\n", minal);
	printf ("Maximum AL length: %1d\n", maxal);
	printf ("Average AL length: %1.2f\n", sal / n);
}
	
void cmd_quit () {

	db_close ();
	exit (0);
}

// ============================================================================

int main (int argc, char *argv []) {

	char cmd, *dbn, *pmn;

	LINE = (char*) malloc ((LINE_size = 128) * sizeof (char));
	if (LINE == NULL) {
		printf ("No memory!!!\n");
		exit (99);
	}

	// Open the database
	dbn = (argc > 1) ? argv [1] : DBNAME;
	pmn = (argc > 2) ? argv [2] : PMNAME;

	if (db_open (dbn, pmn))
		printf ("Database %s exists\n", dbn);
	else
		printf ("New database: %s\n", dbn);

	while (1) {
		printf ("Command: "); fflush (stdout);
		if (getline (&LINE, &LINE_size, stdin) < 0)
			// Done
			exit (0);

		LP = LINE;
		skb (LP);
		if ((cmd = *LP) == '\0')
			continue;

		// Skip the command keyword
		ska (LP);

		// Do the command

		switch (cmd) {

			case 'a':
				cmd_add ();
				break;

			case 'f':
				cmd_find ();
				break;

			case 'p':
				cmd_peg ();
				break;

			case 'l':
				cmd_locate ();
				break;

			case 's':
				cmd_show ();
				break;

			case 'z':
				cmd_params ();
				break;

			case 'd':
				cmd_delete ();
				break;

			case 'q':
				cmd_quit ();
				break;

			default:

// ============================================================================
	printf ("Illegal command, legal commands are:\n");
	printf ("  a x y tag prop npegs p v ... p v\n");
	printf ("  f x y, or x y r, or x0 y0 x1 y1\n");
	printf ("  p peg\n");
	printf ("  l tag prop npegs p v ... p v\n");
	printf ("  z\n");
	printf ("  s\n");
	printf ("  d pn ... pn\n");
	printf ("  q\n");
// ============================================================================

		}
	}
}
