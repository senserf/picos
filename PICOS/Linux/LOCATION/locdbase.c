/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#define _GNU_SOURCE 1
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "locengine.h"


//
// This is a naive and temporary implementation of the tpoint database: to
// be (trivially) replaced by, say, MySQL or something like that.
//

#define	HASHTSIZE	4096		// Must be a power of two

#define	HASHMASK	(HASHTSIZE - 1)
#define	HASHFUN(a)	((int)((a) & HASHMASK))

struct tplink_s {
//
// A link to build lists of points
//
	tpoint_t	*TP;
	struct tplink_s *next;

};

typedef	struct tplink_s tplink_t;

struct thitem_s {
//
// For rehashing pegs that hash into the same slot in the table
//
	u32			Peg;
	tplink_t		*TPL;
	struct thitem_s		*next;

};

typedef	struct thitem_s thitem_t;

static thitem_t *TPHTABLE [HASHTSIZE];

static tplink_t *TPOINTS = NULL,	// The global list of all points
		*CURRENT = NULL;	// The cursor

static char *DBFNAME = NULL;

static bool modified = 0;

static void h_add (u32 p, tpoint_t *tp) {
//
// Adds the point to the Peg's set 
//
	thitem_t *tphi;
	tplink_t *tk;
	int h;

	tk = (tplink_t*) malloc (sizeof (tplink_t));
	if (tk == NULL)
		oom ("h_add");

	tk -> TP = tp;

	tphi = TPHTABLE [h = HASHFUN (p)];

	while (tphi != NULL && tphi->Peg != p)
		tphi = tphi->next;

	if (tphi == NULL) {
		// Create a new one
		tphi = (thitem_t*) malloc (sizeof (thitem_t));
		if (tphi == NULL)
			oom ("h_add");
		tphi -> Peg = p;
		tphi -> next = TPHTABLE [h];
		tphi -> TPL = NULL;
		TPHTABLE [h] = tphi;
	}

	tk->next = tphi->TPL;
	tphi->TPL = tk;
}

// ============================================================================

void oom (const char *msg) {
	abt ("%s: out of memory!", msg);
}

int db_open (const char *fname, const char *pname) {
//
// Reads the database from file to memory
//
	FILE *dbf;
	char *linebuf, *lp;
	tpoint_t *tp;
	size_t lbs;
	float x, y, fv;
	int ln, i, j;
	u32 pr, t, pg, tag, iv;

	if (DBFNAME)
		abt ("must do db_close before db_open");

	// Start by parsing the parameters as the interpretation of data in
	// the database may depend on them
	set_params (pname);

	DBFNAME = (char*) malloc (strlen (fname) + 1);
	if (DBFNAME == NULL)
		oom ("db_open");

	strcpy (DBFNAME, fname);

	if ((dbf = fopen (fname, "r")) == NULL) {
		// The file doesn't exist, so the database starts empty
		return 0;
	}

	// Read the stuff. The format is this:
	//
	// x y p n [peg v]* \n
	//

	linebuf = NULL;
	lbs = 0;
	ln = 0;

	while (getline (&linebuf, &lbs, dbf) >= 0) {
		ln++;
		lp = linebuf;
		if (getfloat (&lp, &x)) {
			// Treat as a comment, but check for version; note that
			// it is OK for the version to change half way through
			get_db_version (linebuf);
			continue;
		}
		if (getfloat (&lp, &y))
			goto FError;

		if (PM_dbver) {
			// The tag is expected; otherwise assume it is zero,
			// meaning "no tag info"
			if (getu32 (&lp, &tag))
				goto FError;
		} else
			tag = 0;

		if (getu32 (&lp, &pr))
			goto FError;
		if (getu32 (&lp, &t))
			goto FError;
		// We know the number of <Peg,rssi> pairs
		tp = (tpoint_t*) malloc (tpoint_tsize (t));
		if (tp == NULL)
			oom ("db_open");
		tp->x = x;
		tp->y = y;
		tp->Tag = tag;
		tp->properties = pr;
		tp->NPegs = (u16) t;

		for (i = 0; i < t; i++) {
			if (getu32 (&lp, &pg))
				goto FError;
			tp->Pegs [i] . Peg = pg;
			for (j = 0; j < i; j++)
				if (tp->Pegs [j] . Peg == pg)
					// They must be distinct
					goto FError;
			if (PM_dbver) {
				// These numbers are integers in the new
				// with the SLR stored separately
				if (getu32 (&lp, &iv))
					goto FError;
			} else {
				if (getfloat (&lp, &fv))
					goto FError;
				// Round it
				iv = (u32)(fv + 0.5);
			}
			tp->Pegs [i] . RSSI = (u16) iv;
		}
		db_add_point (tp);
	}
	fclose (dbf);
	modified = 0;
	CURRENT = NULL;
	return 1;

FError:
	abt ("Database file format error, line %1d", ln);
	return 0;
}

void db_close (void) {
//
// Write back any changes to the file
//
	FILE *dbf;
	tplink_t *tk, *tl;
	tpoint_t *tp;
	thitem_t *th, *ti;
	int h;

	if (DBFNAME == NULL)
		abt ("must do db_open before db_close");

	if (!modified)
		return;

	if ((dbf = fopen (DBFNAME, "w")) == NULL)
		abt ("cannot open %s for writing", DBFNAME);

	// Output the version number
	fprintf (dbf, "DBVersion %1d\n", DBVER);

	for (tk = TPOINTS; tk != NULL; tk = tk->next) {
		tp = tk->TP;
		fprintf (dbf, "%10g %10g %1u 0x%08x %1u",
			tp->x, tp->y, tp->Tag, tp->properties, tp->NPegs);
		for (h = 0; h < tp->NPegs; h++)
			fprintf (dbf, " %1u %1u", tp->Pegs [h] . Peg,
						  tp->Pegs [h] . RSSI );
		fprintf (dbf, "\n");
	}

	fclose (dbf);
	free (DBFNAME);
	DBFNAME = NULL;

	// Deallocate everything
	for (tk = TPOINTS; tk != NULL; tk = tl) {
		tl = tk->next;
		free (tk->TP);
		free (tk);
	}

	for (h = 0; h < HASHTSIZE; h++) {
		th = TPHTABLE [h];
		TPHTABLE [h] = NULL;
		while (th != NULL) {
			for (tk = th->TPL; tk != NULL; tk = tl) {
				tl = tk->next;
				free (tk);
			}
			ti = th->next;
			free (th);
			th = ti;
		}
	}

	modified = 0;
	CURRENT = NULL;
}

static	alitem_t *APD_SORT;

static void sort_alitems (int lo, int up) {
//
// A sorter for the association list: it is sorted by pegs to facilitate
// searching
//
	int k, l;
	alitem_t m;

	while (up > lo) {
		k = lo;
		l = up;
		m = APD_SORT [lo];
		while (k < l) {
			while (APD_SORT [l].Peg > m.Peg)
				l--;
			APD_SORT [k] = APD_SORT [l];
			while (k < l && APD_SORT [k].Peg <= m.Peg)
				k++;
			APD_SORT [l] = APD_SORT [k];
		}
		APD_SORT [k] = m;
		sort_alitems (lo, k - 1);
		lo = k + 1;
	}
}

void db_sort_al (alitem_t *al, int n) {
//
// Sorts the association list by pegs (not reentrant)
//
	APD_SORT = al;
	sort_alitems (0, n-1);
}

void db_add_point (tpoint_t *tp) {
//
// Adds a point to the database
//
	tplink_t *tk;
	int i;

	if (DBFNAME == NULL)
		abt ("db_add_point: db not open");

	// Sort the pegs: this will facilitate searches
	db_sort_al (tp->Pegs, tp->NPegs);

	for (i = 0; i < tp->NPegs; i++) {
		// Sanity check
		if (i > 0 && (tp->Pegs [i].Peg == tp->Pegs [i-1].Peg))
			abt ("db_add_point: duplicate peg %1d",
				tp->Pegs [i].Peg);
		if (tp->Pegs [i].Peg == tp->Tag)
			abt ("db_add_point: peg %1d is same as Tag",
				tp->Pegs [i].Peg);
		tp->Pegs [i] . SLR = rssi_to_slr (tp->Pegs [i] . RSSI);
		h_add (tp->Pegs [i] . Peg, tp);
	}

	// Insert into the global list
	tk = (tplink_t*) malloc (sizeof (tplink_t));
	if (tk == NULL)
		oom ("db_add_point");
	tk -> next = TPOINTS;
	tk -> TP = tp;
	TPOINTS = tk;

#if DEBUGGING > 1
	for (i = 0, tk = TPOINTS; tk != NULL; tk = tk->next, i++);
	printf ("db_add_point [%1d]: <%f,%f>\n", i, tp->x, tp->y);
	fflush (stdout);
#endif
	modified = 1;
}

void db_delete_point (tpoint_t *tp) {
//
// Deletes the current point
//
	tplink_t *tk, *tl;
	thitem_t *th;
	int h;

	if (DBFNAME == NULL)
		abt ("db_delete_point: db not open");

	if (tp == NULL) {
		if (CURRENT == NULL)
			abt ("db_delete_point don't know what to delete");
		tp = CURRENT->TP;
	}

	if (CURRENT != NULL && tp == CURRENT->TP) {
		// Deleting CURRENT, make sure it is always sane
		CURRENT = CURRENT->next;
	}

	for (tk = TPOINTS, tl = NULL; tk != NULL; tl = tk, tk = tk->next)
		if (tk->TP == tp)
			break;

	if (tk != NULL) {
		if (tl == NULL)
			TPOINTS = tk->next;
		else
			tl->next = tk->next;
		free (tk);
	}

	// From Pegs' lists
	for (h = 0; h < HASHTSIZE; h++) {
		th = TPHTABLE [h];
		while (th != NULL) {
			for (tk = th->TPL, tl = NULL; tk != NULL; tl = tk,
			    tk = tk->next)
				if (tk->TP == tp)
				    break;
			if (tk != NULL) {
				if (tl == NULL)
					th->TPL = tk->next;
				else
					tl->next = tk->next;
				free (tk);
			}
			th = th->next;
		}
	}

	free (tp);
	modified = 1;
}

tpoint_t *db_get_point (void) {

	if (DBFNAME == NULL)
		abt ("db_get_point: db not open");
	return CURRENT == NULL ? NULL : CURRENT->TP;
}

void db_next_point (void) {

	if (DBFNAME == NULL)
		abt ("db_next_point: db not open");
	if (CURRENT != NULL) 
		CURRENT = CURRENT->next;
}

void db_start_points (u32 peg) {
//
// Start reading points from the DB
//
	thitem_t *th;

	if (DBFNAME == NULL)
		abt ("db_start_points: db not open");

	if (peg == 0) {
		CURRENT = TPOINTS;
		return;
	}

	th = TPHTABLE [HASHFUN (peg)];

	while (th != NULL && th->Peg != peg)
		th = th->next;

	CURRENT = th == NULL ? NULL : th->TPL;
}
