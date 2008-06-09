#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "locengine.h"

//
// Copyright (C) 2008 Olsonet Communications Corporation
//
// PG March 2008
//

//
// This is the naive and temporary implementation of the tpoint database: to
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

static int getint (char **lp, u32 *v) {
//
// Reads an integer from the character string
//
	char *rp, *p = *lp;
	while (isspace (*p))
		p++;

	*v = (u32) strtol (p, &rp, 0);

	if (rp == p)
		return -1;

	*lp = rp;
	return 0;
}

static int getfloat (char **lp, float *v) {
//
// Reads a float number from the character string
//
	char *rp, *p = *lp;

	while (isspace (*p))
		p++;

	*v = (float) strtod (p, &rp);

	if (rp == p)
		return -1;

	*lp = rp;
	return 0;
}

static void h_add (u32 p, tpoint_t *tp) {
//
// Adds the point to the Peg's set 
//
	thitem_t *tphi;
	tplink_t *tk;
	int h;

	tk = (tplink_t*) malloc (sizeof (tplink_t));
	if (tk == NULL) {
Mem:
		fprintf (stderr, "h_add: out of memory\n");
		exit (99);
	}

	tk -> TP = tp;

	tphi = TPHTABLE [h = HASHFUN (p)];

	while (tphi != NULL && tphi->Peg != p)
		tphi = tphi->next;

	if (tphi == NULL) {
		// Create a new one
		tphi = (thitem_t*) malloc (sizeof (thitem_t));
		if (tphi == NULL)
			goto Mem;
		tphi -> Peg = p;
		tphi -> next = TPHTABLE [h];
		tphi -> TPL = NULL;
		TPHTABLE [h] = tphi;
	}

	tk->next = tphi->TPL;
	tphi->TPL = tk;
}

// ============================================================================

int db_open (const char *fname) {
//
// Reads stuff from file to memory
//
	FILE *dbf;
	char *linebuf, *lp;
	tpoint_t *tp;
	tplink_t *tk;
	size_t lbs;
	float x, y;
	int ln, i, j, h;
	u32 pr, t, pg;

	if (DBFNAME) {
		fprintf (stderr, "must do db_close before db_open\n");
		exit (99);
	}

	DBFNAME = (char*) malloc (strlen (fname) + 1);
	if (DBFNAME == NULL) {
Mem:
		fprintf (stderr, "db_open: out of memory\n");
		exit (99);
	}

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
		if (getfloat (&lp, &x))
			// Treat as a comment
			continue;
		if (getfloat (&lp, &y))
			goto FError;
		if (getint (&lp, &pr))
			goto FError;
		if (getint (&lp, &t))
			goto FError;
		// We know the number of <Peg,rssi> pairs
		tp = (tpoint_t*) malloc (tpoint_tsize (t));
		if (tp == NULL)
			goto Mem;
		tp->x = x;
		tp->y = y;
		tp->properties = pr;
		tp->NPegs = (u16) t;

		for (i = 0; i < t; i++) {
			if (getint (&lp, &pg))
				goto FError;
			tp->Pegs [i] . Peg = pg;
			for (j = 0; j < i; j++)
				if (tp->Pegs [j] . Peg == pg)
					// They must be distinct
					goto FError;
			if (getfloat (&lp, &(tp->Pegs [i] . SLR)))
				goto FError;
		}
		db_add_point (tp);
	}
	fclose (dbf);
	modified = 0;
	CURRENT = NULL;
	return 1;

FError:
	fprintf (stderr, "Database file format error, line %1d\n", ln);
	exit (99);
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

	if (DBFNAME == NULL) {
		fprintf (stderr, "must do db_open before db_close\n");
		exit (99);
	}

	if (!modified)
		return;

	if ((dbf = fopen (DBFNAME, "w")) == NULL) {
		fprintf (stderr, "cannot open %s for writing\n", DBFNAME);
		exit (99);
	}

	for (tk = TPOINTS; tk != NULL; tk = tk->next) {
		tp = tk->TP;
		fprintf (dbf, "%10g %10g 0x%08x %1u",
			tp->x, tp->y, tp->properties, tp->NPegs);
		for (h = 0; h < tp->NPegs; h++)
			fprintf (dbf, " %1u %f", tp->Pegs [h] . Peg,
						 tp->Pegs [h] . SLR );
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

static	pegitem_t *APD_SORT;

static void sort_pegitems (int lo, int up) {
//
// A sorter for the association list: it is sorted by pegs to facilitate
// searching
//
	int k, l;
	pegitem_t m;

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
		sort_pegitems (lo, k - 1);
		lo = k + 1;
	}
}

void db_add_point (tpoint_t *tp) {
//
// Adds a point to the database
//
	tplink_t *tk;
	int i;

	if (DBFNAME == NULL) {
		fprintf (stderr, "db_add_point: db not open\n");
		exit (99);
	}

	// Sort the pegs: this will facilitate searches
	APD_SORT = tp->Pegs;
	sort_pegitems (0, tp->NPegs - 1);

	for (i = 0; i < tp->NPegs; i++) {
		// Sanity check
		if (i > 0 && (tp->Pegs [i].Peg == tp->Pegs [i-1].Peg)) {
			fprintf (stderr, "db_add_point: duplicate peg\n");
			exit (99);
		}
		h_add (tp->Pegs [i] . Peg, tp);
	}

	// Insert into the global list
	tk = (tplink_t*) malloc (sizeof (tplink_t));
	if (tk == NULL) {
		fprintf (stderr, "db_add_point: out of memory\n");
		exit (99);
	}
	tk -> next = TPOINTS;
	tk -> TP = tp;
	TPOINTS = tk;

#ifdef	DEBUGGING
	for (i = 0, tk = TPOINTS; tk != NULL; tk = tk->next, i++);
	printf ("db_add_point [%1d]: <%f,%f>\n", i, tp->x, tp->y);
	fflush (stdout);
#endif
	modified = 1;
}

void db_delete_point (tpoint_t *tp) {
//
// Deletes the current point. Note: this may be is slow, as deletion is not
// supposed to occur often.
//
	tplink_t *tk, *tl, *nc;
	thitem_t *th;
	int h;

	if (DBFNAME == NULL) {
		fprintf (stderr, "db_delete_point: db not open\n");
		exit (99);
	}

	if (tp == NULL) {
		if (CURRENT == NULL) {
			fprintf (stderr,
				"db_delete_point don't know what to delete\n");
			exit (99);
		}
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

	if (DBFNAME == NULL) {
		fprintf (stderr, "db_get_point: db not open\n");
		exit (99);
	}
	return CURRENT == NULL ? NULL : CURRENT->TP;
}

void db_next_point (void) {

	if (DBFNAME == NULL) {
		fprintf (stderr, "db_next_point: db not open\n");
		exit (99);
	}
	if (CURRENT != NULL) 
		CURRENT = CURRENT->next;
}

void db_start_points (u32 peg) {
//
// Start reading points from the DB
//
	thitem_t *th;

	if (DBFNAME == NULL) {
		fprintf (stderr, "db_start_points: db not open\n");
		exit (99);
	}

	if (peg == 0) {
		CURRENT = TPOINTS;
		return;
	}

	th = TPHTABLE [HASHFUN (peg)];

	while (th != NULL && th->Peg != peg)
		th = th->next;

	CURRENT = th == NULL ? NULL : th->TPL;
}
