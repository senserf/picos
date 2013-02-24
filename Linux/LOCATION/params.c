#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "locengine.h"

// ============================================================================
// Parameters =================================================================
// ============================================================================

u32 PM_dbver = 0;	// DB version number; this one is encoded in the DB


#define	issp(a)	isspace ((int)(unsigned char)(a))

//
// Parameters for RSSI -> SLR conversion:
//
float *PM_rts_v = NULL;		// Values
u16   *PM_rts_a = NULL;		// Arguments
int    PM_rts_n = 0;		// The number of values/arguments, must be 0 or
				// > 1
// The idea is that a[i] is interpreted as RSSI value and v[i] is the
// corresponding SLR value; anything less than a[0] maps into V[0], anything
// greater than a[n-1] maps into v[n-1]; values in between are interpolated;
// a[i] must be strictly increasing, v[i] need not

// 
// Parameters for calculating the discrepancy, i.e., vector distance between
// association lists:
//
u32	PM_dis_min = 1;
		// The minimum number of slots that must be matched, i.e., the
		// minimum number of Pegs that must occur in both lists

float	PM_dis_fac = 1.0;
		// Preference factor for longer matches. The discrepancy is
		// calculated as sqrt (sum (Di^2) / (N * fac)), where N is
		// the number of matched Pegs; thus, fac > 1 will give
		// preference to longer matches.

int	PM_dis_tag = 0;		
		// Tells how to treat the point's Tag:
		//	0 - ignore the Tag
		//    > 0 - assume this to be the RSSI of the Tag to be matched
		//          in the same way as any Peg
		//    < 0 - assume -this to be the RSSI of the Tag to be
		//          matched in such a way that any query value >= that
		//	    RSSI gives discrepancy 0 on that entry, while any
		//	    value less than that is treated as deviating in the
		//	    same way as for a regular Peg

float	PM_dis_taf = 0.0;
		// This is the precomputed SLR corresponding to PM_dis_tag

u32	PM_sel_min = 1,
	PM_sel_max = 1000;
float	PM_sel_fac = 3.0;
		// These determine the way of selecting the points whose
		// coordinates will be averaged into estimated coordinates:
		//	min - the minimum number of points to average
		//	max - the maximum number of points to average
		//	fac - drop factor
		// This is how it works. First, all qualified points (based on
		// dis_min) are selected. Then, we trim their population down
		// to no more than max best points. Then we keep dropping
		// points from the set whose discrepancy exceeds the running
		// average discrepancy of the current set multiplied by fac,
		// for as long as the remaining number of points is >= min

int	PM_ave_for = 0;
float	PM_ave_fac = 1.0;
		// Formula and factor for averaging the coordinates of the
		// selected "close" points into the estimated coordinates.
		// There are two formulas based on this generic scheme:
		//
		//	u_est = (sum ui * Wi) / sum Wi
		//
		//	frm = 0 - linear: Wi = ((sum Dj) - Di) ^ (1/fac)
		//	frm = 1 - exponential: Wi = e ^ (-Di * fac)
		//
		// So larger values of fac amplify the impact of close points

// ============================================================================

// SXML wrapper functions adapted from SMURPH

static char *sxml_handle_includes (char *IF, int *MS, char **err) {
/*
 * Searches (heuristically and inefficiently) for included files and reads
 * them in. Note: I tried to do it in sxml.c, but that would require a rather
 * drastic reprogramming of the whole thing because the package uses pointers
 * to the original string. That means, in particular, that the string cannot
 * be re-alloc'ed half way through, which makes it impossible to include files
 * as we go. Consequently, I have to pre-include them before invoking the
 * parser.
 */
	int sk, tl, nc, nr, ms;
	char *e, *s, *t, *u, *v, *w, *f;

	e = (s = IF) + *MS;
	*err = NULL;

	while (1) {
		// Find a tag
		while (s < e && *s != '<')
			s++;
		if (s >= e)
			// All done
			return IF;
		// Will be deleting from s; u == keyword pointer
		v = u = s + 1;
		// This must be "xi:include" or just "include" (our shortcut)
		if (strncmp (v, "xi:include", 10) == 0) {
			sk = 10;
		} else if (strncmp (v, "include", 7) == 0) {
			sk = 7;
		} else {
			sk = 0;
		}
		if (!sk) {
			// Not an include tag
			s = v;
			continue;
		}
		// Skip the keyword
		v += sk;
		if (v >= e || !issp (*v)) {
NoHref:
			*err = "include tag requires an href attribute";
Error:
			free (IF);
			return NULL;
		}

		// Find the end of this tag; v points to the starting point
		// for locating href
		for (t = v; t < e && *t !='>'; t++);
		if (t == e) {
Broken:
			*err = "include tag format error";
			goto Error;
		}
		// Last point for locating href
		w = t;
		if (*(t-1) != '/') {
			// This is not a self-closing tag
			while (t < e && *t != '<')
				t++;
			if (t >= e || *++t != '/') {
Unmatched:
				*err = "unmatched include tag";
				goto Error;
			}
			// Keywords must match
			t++;
			if (strncmp (u, t, sk))
				goto Unmatched;
			while (t < e && *t !='>')
				t++;
			if (t == e)
				goto Unmatched;
		}
		// Will remove upto this
		t++;

		// Locate href
		do {
			if (v > w - 8)
				goto NoHref;
		} while (strncmp ("href=\"", v++, 6));

		// Skip to filename
		v += 5;

		while (v < e && issp (*v))
			v++;

		if (v == e || *v == '\"') {
			*err = "file name missing for include href";
			goto Error;
		}

		for (w = v + 1; !issp (*w) && *w != '\"'; w++)
			if (w >= t)
				goto Broken;
		// Can do it safely as that part will be removed
		*w = '\0';

		if ((sk = open (v, O_RDONLY)) < 0) {
			*err = "cannot open include file";
			goto Error;
		}

		if ((f = (char*) malloc (ms = 1024)) == NULL) {
Mem:
			oom ("sxml_handle_includes");
		}
		nc = 0;

		while (1) {

			if (nc == ms) {
				// Realloc
				if ((w = (char*) malloc (ms = ms + ms)) == NULL)
					goto Mem;
				memcpy (w, f, nc);
				free (f);
				f = w;
			}
				
			nr = read (sk, f + nc, ms - nc);

			if (nr < 0) {
				close (sk);
				free (w);
				*err = "cannot read from include file";
				goto Error;
			}

			if (nr == 0)
				break;

			nc += nr;
		}

		close (sk);

		// Offset from the beginning of how far we are
		ms = (s - IF);

		// Old tail to be appended
		tl = *MS - (t - IF);
		
		// New total length
		nr = ms + nc + tl;

		if ((w = (char*) malloc (nr)) == NULL)
			goto Mem;

		strncpy (w, IF, ms);
		strncpy (w + ms, f, nc);
		strncpy (w + ms + nc, t, tl);

		*MS = ms + nc + tl;

		free (IF);
		free (f);
		IF = w;
		s = IF + ms;
		e = IF + *MS;
	}
}

static sxml_t sxml_parse_pfile (const char *fn) {

	FILE 	*paf;
	int	c, CSize, MSize = 1024;
	char	*IF, *SF;

	void sxml_put (char c) {

		if (CSize == MSize) {
			SF = (char*) malloc (MSize + MSize);
			if (SF == NULL)
				oom ("sxml_put");
			memcpy (SF, IF, CSize);
			free (IF);
			IF = SF;
			MSize += MSize;
		}
		IF [CSize++] = c;
	};

	if ((paf = fopen (fn, "r")) == NULL) {
		abt ("sxml_parse_pfile: cannot open %s", fn);
		exit (99);
	}

	IF = (char*) malloc (MSize);
	CSize = 0;

	while ((c = getc (paf)) != EOF)
		sxml_put (c);

	if ((IF = sxml_handle_includes (IF, &CSize, &SF)) == NULL)
		// IF has been freed by sxml_handle_includes; SF passess the
		// error message
		return sxml_parse_str (SF, 0);
	
	return sxml_parse_str1 (IF, CSize);
	// No need to delete IF; it is being recycled by sxml_parse_str1 and
	// will be deallocated along with the entire structure by
}

// ============================================================================

void abt (const char *fmt, ...) { 

	va_list pmts;

	va_start (pmts, fmt);
	vfprintf (stderr, fmt, pmts);
	fprintf (stderr, "\n");
	exit (99);
}

int getint (char **lp, long *v) {
//
// Reads an integer from the character string
//
	char *rp, *p = *lp;
	long long ll;

	while (issp (*p))
		p++;

	ll = strtoll (p, &rp, 0);

	if (rp == p)
		return -1;

	*lp = rp;
	*v = (long) ll;
	return 0;
}

int findint (char **lp, long *v) {
//
// Finds the first integer number the character string. This is like getint
// except that we keep advancing the pointer until the decode succeeds
//
	char *rp, *p = *lp;
	long long ll;

	while (*p != '\0') {
		ll = strtoll (p, &rp, 0);
		if (rp != p) {
			*lp = rp;
			*v = (long) ll;
			return 0;
		}
		p++;
	}

	return -1;
}

int getdouble (char **lp, double *v) {
//
// Reads a double number from the character string
//
	char *rp, *p = *lp;

	while (issp (*p))
		p++;

	*v = strtod (p, &rp);

	if (rp == p)
		return -1;

	*lp = rp;
	return 0;
}

int finddouble (char **lp, double *v) {
//
// Reads a double number from the character string
//
	char *rp, *p = *lp;

	while (*p != '\0') {
		*v = strtod (p, &rp);
		if (rp != p) {
			*lp = rp;
			return 0;
		}
		p++;
	}

	return -1;
}

// ============================================================================

static void set_params_rts (sxml_t xmlp) {
//
// RSSI -> SLR conversion table
//
//	Example:	<slr> (40, 0.0) (60, 2.0) (80, 10.0) </slr>
//
#define	TRTSIZE 256
	char *data;
	int n;
	long a;
	u16 args [TRTSIZE];
	float f, vals [TRTSIZE];

	if ((xmlp = sxml_child (xmlp, "slr")) == NULL)
		return;

	// This should be a sequence of pairs: <RSSI, SLR>
	data = (char*) sxml_txt (xmlp);

	n = 0;
	while (1) {

		// The argument
		if (findint (&data, &a))
			break;

		if (n == TRTSIZE)
			abt ("<slr> list too long, %1d is max", TRTSIZE);

		if (a < 0)
			abt ("<slr> negative RSSI argument: %1d", a);

		if (a > 0x0000ffff)
			abt ("<slr> argument too big: %1d > %1d",
				a, 0x0000ffff);

		if (n > 0 && a <= args [n-1])
			abt ("<slr arguments not increasing %1d > %1d",
				args [n-1], a);

		args [n] = (u16) a;

		// Expect a float
		if (findfloat (&data, &f))
			abt ("<slr> list invalid, odd number of values");

		vals [n] = f;
		n++;
	}

	if (n == 0)
		// Empty list, should be legit, I think, defaulting to "no
		// conversion"
		return;

	if ((PM_rts_v = (float*) malloc (n * sizeof (float))) == NULL ||
	    (PM_rts_a = (u16*)   malloc (n * sizeof (u16  ))) == NULL )
		oom ("set_params_rts");

	memcpy (PM_rts_v, vals, n * sizeof (float));
	memcpy (PM_rts_a, args, n * sizeof (u16  ));
	PM_rts_n = n;
}

void set_params_dis (sxml_t xmlp) {
//
// Parameters for calculating distance between association lists
//
//	Example:	<dis min="2" fac="1.5" tag="-80"/>
//
	char *att;
	long iv;

	if ((xmlp = sxml_child (xmlp, "dis")) == NULL)
		return;

	if ((att = (char*) sxml_attr (xmlp, "min")) != NULL) {
		if (findint (&att, &iv))
			abt ("<dis> min attribute (%s) is not an int", att);
		if (iv <= 0)
			abt ("<dis> min attribute (%1d) has an illegal value",
				iv);
		PM_dis_min = (u32) iv;
	}

	if ((att = (char*) sxml_attr (xmlp, "tag")) != NULL) {
		if (findint (&att, &iv))
			abt ("<dis> tag attribute (%s) is not an int", att);
		// Note that set_params_rts was called earlier, so it is OK to
		// call rssi_to_slr
		if ((PM_dis_tag = iv) < 0)
			iv = -iv;
		// This is always nonnegative
		PM_dis_taf = rssi_to_slr (iv);
	}

	if ((att = (char*) sxml_attr (xmlp, "fac")) != NULL) {
		if (findfloat (&att, &PM_dis_fac))
			abt ("<dis> fac attribute (%s) is not an FP number ",
				att);
		if (PM_dis_fac <= 0.0)
			abt ("<dis> fac attribute (%f) has an illegal value",
				PM_dis_fac);
	}
}

void set_params_sel (sxml_t xmlp) {
//
// Parameters for selecting points for averaging
//
//	Example:	<sel min="3" max="9" fac="2.5"/>
//
	char *att;
	long iv;

	if ((xmlp = sxml_child (xmlp, "sel")) == NULL)
		return;

	if ((att = (char*) sxml_attr (xmlp, "min")) != NULL) {
		if (findint (&att, &iv))
			abt ("<sel> min attribute (%s) is not an int", att);
		if (iv <= 0)
			abt ("<sel> min attribute (%1d) has an illegal value",
				iv);
		PM_sel_min = (u32) iv;
	}

	if ((att = (char*) sxml_attr (xmlp, "max")) != NULL) {
		if (findint (&att, &iv))
			abt ("<sel> max attribute (%s) is not an int", att);
		if (iv <= 0)
			abt ("<sel> max attribute (%1d) has an illegal value",
				iv);
		PM_sel_max = (u32) iv;
	}

	if ((att = (char*) sxml_attr (xmlp, "fac")) != NULL) {
		if (findfloat (&att, &PM_sel_fac))
			abt ("<sel> fac attribute (%s) is not an FP number ",
				att);
		if (PM_sel_fac <= 0.0)
			abt ("<sel> fac attribute (%f) has an illegal value",
				PM_sel_fac);
	}

	if (PM_sel_min > PM_sel_max)
		abt ("<sel> min (%1d) > max (%1d)", PM_sel_min, PM_sel_max);
}

void set_params_ave (sxml_t xmlp) {
//
// Parameters for averaging the coordinates of selected points
//
//	Example:	<ave for="lin" fac="3.2"/>
//
	char *att;

	if ((xmlp = sxml_child (xmlp, "ave")) == NULL)
		return;

	if ((att = (char*) sxml_attr (xmlp, "for")) != NULL) {
		if (strncmp (att, "lin", 3) == 0)
			PM_ave_for = 0;
		else
			PM_ave_for = 1;
	}

	if ((att = (char*) sxml_attr (xmlp, "fac")) != NULL) {
		if (findfloat (&att, &PM_ave_fac))
			abt ("<ave> fac attribute (%s) is not an FP number ",
				att);
		if (PM_ave_fac <= 0.0)
			abt ("<ave> fac attribute (%f) has an illegal value",
				PM_ave_fac);
	}
}

void set_params (const char *pfn) {

	sxml_t xmlp;

	if (pfn == NULL)
		// This is in fact optional; we can work with defaults
		return;
	
	xmlp = sxml_parse_pfile (pfn);

	// ====================================================================

	set_params_rts (xmlp);
	set_params_dis (xmlp);
	set_params_sel (xmlp);
	set_params_ave (xmlp);

	// ====================================================================

	sxml_free (xmlp);
}

void get_db_version (char *ln) {

	long v;

	while (*ln != '\0') {
		if (strncmp (ln, "DBVersion", 9) == 0) {
			ln += 9;
			if (findint (&ln, &v)) {
				abt ("Version number missing in DB: %s", ln);
				exit (99);
			}
			if (v > DBVER) {
				abt ("Illegal DB version number: "
					"%1u, max is %u", v, DBVER);
				exit (99);
			}
			PM_dbver = (u32) v;
			return;
		}
	}
}

// ============================================================================
