/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-06   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

#if	ZZ_NOS

/* --------------------------------- */
/* Functions operating on RVariables */
/* --------------------------------- */

#include        "system.h"
#include        "rvoffs.h"

void    RVariable::zz_start () {

/* ----------------------- */
/* Nonstandard constructor */
/* ----------------------- */

	s = NULL;
	if (TheProcess == ZZ_Main && TheStation != NULL &&
		TheStation != System) {
		// Created at level 0 -- should be assigned to its station
		Assert (!zz_flg_started,
	       "RVariable: cannot create from Root after protocol has started");
		pool_in ((ZZ_Object*)this, TheStation->ChList, ZZ_Object);
	} else {
		// Not level 0 -- assign to the rightful owner, i.e. the
		// process that creates the object

#if     ZZ_OBS
		if (zz_observer_running) {
			// Observers are also allowed to create RVariables
			pool_in (this, zz_current_observer->ChList,
				ZZ_Object);
		} else
#endif
		{
			pool_in (this, TheProcess->ChList, ZZ_Object);
		}
	}

	// Note: We assume that DisplayActive cannot be set before Root
	// initializes things
}

void    RVariable::setup (int type, int moments) {

/* -------------------------- */
/* Initialize a new RVariable */
/* -------------------------- */

	// type    - the counter type (TYPE_long or TYPE_BIG)
	// moments - the number of central moments to be calculated

	int             size;

	assert (moments >= 0, "RVariable: negative number of moments %f",
		moments);
	assert (moments <= MAX_MOMENTS, "RVariable: too many moments (%1d), "
		"the maximum is %1d", moments, MAX_MOMENTS);
	assert ((type == TYPE_long) || (type == TYPE_BIG),
		"RVariable: illegal type %1d", type);

	Class = OBJ_rvariable;
	Id    = sernum++;

#if BIG_precision == 1
	size = (MOMENT + moments);
	s = (LONG*) (new double [size]);
	stype(s) = moments;
#else
	if (type == TYPE_long)
		size = (MOMENT + moments);
	else
		size = (BMOMENT + moments);

	s = (LONG*) (new double [size]);

	// For a regular statistics, the type gives directly the
	// number of moments.   For a BIG RVariable, the type is
	// equal to the negative number of moments - 1.

	if (type == TYPE_long)
		stype(s) = moments;
	else
		stype(s) = - moments - 1;
#endif
	erase ();
}

void    RVariable::erase () {

/* ------------------------- */
/* Reset the random variable */
/* ------------------------- */

	LONG moments; int i;

	if ((moments = stype(s)) >= 0) {
		counter(s) = 0;
		minimum(s) = HUGE;
		maximum(s) = -HUGE;
		for (i = 0; i < moments; i++) moment(s,i) = 1.0;

	} else {
		moments = -moments - 1;
		bcounter(s) = BIG_0;
		bminimum(s) = HUGE;
		bmaximum(s) = -HUGE;
		for (i = 0; i < moments; i++) bmoment(s,i) = 1.0;
	}
}

RVariable::~RVariable   () {

/* ---------- */
/* Destructor */
/* ---------- */

	LONG    size;

	if (s != NULL) {
		if ((size = stype (s)) >= 0) {
			// TYPE_long RVariable
			size += MOMENT;
		} else {
			// TYPE_BIG  RVariable
			size = BMOMENT - 1 - size;
		}
		assert (size <= BMOMENT + MAX_MOMENTS,
			"RVariable destructor: illegal RVariable contents");

		// Remove the array
		delete (s);

		// Remove the RVariable from the owner's list
		pool_out (this);

                // Notify the server that the object has been removed
                zz_DREM (this);
	}

	if (zz_nickname != NULL) delete (zz_nickname);
}

#if	ZZ_NOC
#if	ZZ_NOS
ZZ_TRVariable::~ZZ_TRVariable   () {

/* ------------------------------------ */
/* Destructor for a temporary RVariable */
/* ------------------------------------ */

	LONG    size;

	if (s != NULL) {
		if ((size = stype (s)) >= 0) {
			// TYPE_long RVariable
			size += MOMENT;
		} else {
			// TYPE_BIG  RVariable
			size = BMOMENT - 1 - size;
		}
		assert (size <= BMOMENT + MAX_MOMENTS,
			"RVariable destructor: illegal RVariable contents");

		// Remove the array
		delete (s);

		// Fool the superclass destructor
		s = NULL;
	}
}
#endif
#endif

void  RVariable::operator= (RVariable &ss) {

/* -------------------------------- */
/* Copy the current RVariable to ss */
/* -------------------------------- */

	LONG    size1, size2;

	if (stype(ss.s) < 0)
		size1 = BMOMENT - stype(ss.s) - 1;
	else
		size1 = MOMENT + stype(ss.s);

	assert (size1 <= BMOMENT + MAX_MOMENTS,
		"RVariable = (rhs): illegal RVariable contents");

	if (stype(s) < 0)
		size2 = BMOMENT - stype(s) - 1;
	else
		size2 = MOMENT + stype(s);

	assert (size2 <= BMOMENT + MAX_MOMENTS,
		"(rhs) = RVariable: illegal RVariable contents");

	assert ((stype(s) = stype(ss.s)) && (size1 == size2),
		"RVariable = RVariable: incompatible RVariable types: "
			"%1d[%1d] != %1d[%1d]",
				stype(ss.s), size1, stype(s), size2);

	bcopy ((BCPCASTS) ss.s, (BCPCASTD) s,
			(IPointer) (size1 * sizeof (double)));
}

void    RVariable::update (double value, LONG inc) {

/* ---------------------------------------------------------- */
/* Add  one  sample  (or  a  number of the same samples) to a */
/* TYPE_long RVariable                                        */
/* ---------------------------------------------------------- */

	double  fact, nw, ap;
	LONG    moments, i;

	moments = stype (s);

#if     BIG_precision != 1
	if (moments < 0) {
		// This is a TYPE_BIG RVariable
		update (value, (BIG) inc);
		return;
	}
#endif
	assert (moments <= MAX_MOMENTS,
		"RVariable->update: illegal RVariable contents");
	assert (inc >= 0L, "RVariable->update: negative increment");

	// Update the minimum and maximum

	if (maximum(s) < value) maximum(s) = value;
	if (minimum(s) > value) minimum(s) = value;

	if (inc == 0L) return;
	fact = (double) (counter(s));
	fact /=  (nw = (double) (counter(s) += inc));
	nw = inc / nw;

	if (moments == 2) {     // The most frequent (default) case
		// The mean value
		moment(s,0) = (moment(s,0) * fact) + (ap = value * nw);
		// The variance
		moment(s,1) = (moment(s,1) * fact) + (ap *= value);
	} else if (moments > 0) {
		moment(s,0) = (moment(s,0) * fact) + (ap = value * nw);
		for (i = 1; i < moments; i++)
			moment(s,i) = (moment(s,i) * fact) + (ap *= value);
	}
}

#if     BIG_precision != 1

void    RVariable::update (double value, BIG inc) {

/* ---------------------------------------------------------- */
/* Add  one  sample  (or  a  number of the same samples) to a */
/* TYPE_BIG RVariable                                         */
/* ---------------------------------------------------------- */

	double  nw, ap, t;
	LONG    moments, i;
	BIG     tt;

	moments = stype (s);

	if (moments >= 0) {
		// This is a TYPE_long RVariable (may not always work)
		update (value, (LONG) inc);
		return;
	}

	moments = - moments - 1;

	assert (moments <= MAX_MOMENTS,
		"RVariable->update: illegal RVariable contents");

	if (bmaximum(s) < value) bmaximum(s) = value;
	if (bminimum(s) > value) bminimum(s) = value;

	if (inc == BIG_0) return;

	tt = bcounter (s) + inc;

	if (moments == 2) {     // The standard case

		// The clumsy way of calculating the statistics reduces the
		// chance of an overflow

		if (fabs (bmoment (s, 0)) > 1.0)
			bmoment (s, 0) = bcounter (s) / (tt / bmoment (s, 0));
		else {
			if ((t = bcounter (s) * bmoment (s, 0)) == 0.0)
			  bmoment (s, 0) = 0.0;
			else
			  bmoment (s, 0) = 1.0 / (tt / t);
		}

		if (fabs (bmoment (s, 1)) > 1.0)
			bmoment (s, 1) = bcounter (s) / (tt / bmoment (s, 1));
		else {
			if ((t = bcounter (s) * bmoment (s, 1)) == 0.0)
			  bmoment (s, 1) = 0.0;
			else
			  bmoment (s, 1) = 1.0 / (tt / t);
		}

		if (fabs (nw = inc * value) > 1.0) {
			bmoment (s, 0) += (ap = 1.0 / (tt / nw));
		} else if (convertible (tt)) {
			bmoment (s, 0) += (ap = nw / (double) tt);
		} else
			ap = 0.0;

		bmoment (s, 1) += ap * value;

	} else if (moments > 0) {

		for (i = 0; i < moments; i++) {
			if (fabs (bmoment (s, i)) > 1.0)
				bmoment (s, i) = bcounter(s) /
					(tt / bmoment (s, i));
			else {
			  if ((t = bcounter (s) * bmoment (s, i)) == 0.0)
			    bmoment (s, i) = 0.0;
			  else
			    bmoment (s, i) = 1.0 / (tt / t);
			}
		}

		// Now the second part

		if (fabs (nw = inc * value) > 1.0) {
			bmoment(s,0) += (ap = 1.0 / (tt / nw));
		} else if (convertible (tt)) {
			bmoment(s,0) += (ap = nw / (double) tt);
		} else
			ap = 0.0;

		for (i = 1; i < moments; i++) bmoment (s, i) += (ap *= value);
	}
	bcounter (s) = tt;
}

#endif

sexposure (RVariable)

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr);         // Full contents

		sexmode (1)

			exPrint1 (Hdr, YES);    // Abbreviated contents

		sexmode (2)

			exPrint1 (Hdr, NO);     // Short contents
	}

	sonscreen {

		sfxmode (0)

			exDisplay0 (YES);       // Full contents + region

		sexmode (1)

			exDisplay0 (NO);        // Full contents - region

		sexmode (2)

			exDisplay1 ();          // Short contents
	}
	USESID;
}

static double confidence (double z, double mean, double dev, double n) {

/* ------------------------------ */
/* Calculates confidence interval */
/* ------------------------------ */

	if (mean < 0.0) mean = - mean;
	return (((dev / sqrt (n)) * z) / mean);
}

void    RVariable::exPrint0 (const char *hdr) {

/* ------------------------------------ */
/* Print full contents of the RVariable */
/* ------------------------------------ */

	double  mom [BMOMENT+MAX_MOMENTS], min, max;
	LONG    lcount;
	BIG     bc;

#if     BIG_precision != 1
	BIG     bcount;
#endif
	LONG    i, moments;

	if (stype(s) < 0) moments = - stype(s) - 1; else moments = stype(s);

#if     BIG_precision != 1
	if (stype (s) < 0)
		calculate (min, max, mom, bcount);
	else
#endif
		calculate (min, max, mom, lcount);

	if (hdr != NULL) {
		Ouf << hdr;
	} else {
		Ouf << getOName () << ':';
	}

	Ouf << "\n\n";
		
#if     BIG_precision != 1
	if (stype(s) < 0) {      // Big count
		print (bcount, "      Number of samples:    ");
		bc = bcount;
	} else
#endif
	{
		print (lcount, "      Number of samples:    ");
		bc = lcount;
	}

	if (bc > BIG_0) {
		print (min, "      Minimum value:        ");
		print (max, "      Maximum value:        ");

		for (i = 0; i < moments; i++) {
			if (i == 0) {
			   print (mom [i], "      Mean value:           ");
			} else if (i == 1) {
			   print (mom [i], "      Variance:             ");
			   print (min = (mom [i] >= 0.0) ? sqrt (mom [i]) : 0.0,
				"      Standard deviation:   ");
			   if (convertible (bc))
			     max = (double) bc;
			   else
			     max = HUGE;
			   print (confidence (ZALPHA95, mom [0], min, max),
				"      Confidence int @95%:  ");
			   print (confidence (ZALPHA99, mom [0], min, max),
				"      Confidence int @99%:  ");
			} else
			   print (mom [i], form ("      Moment number %3d:    ",
				i+1));
		}
	}

	Ouf << '\n';    // The terminating empty line
}
   
void    RVariable::exPrint1 (const char *hdr, int longer) {

/* ------------------------------------------------- */
/* Print abbreviated/short contents of the RVariable */
/* ------------------------------------------------- */

	double  mom [BMOMENT+MAX_MOMENTS], min, max;
	LONG    lcount;
	BIG     bc;

#if     BIG_precision != 1
	BIG     bcount;
#endif
	LONG    moments;

	if (stype(s) < 0) moments = - stype(s) - 1; else moments = stype(s);

#if     BIG_precision != 1
	if (stype (s) < 0)
		calculate (min, max, mom, bcount);
	else
#endif
		calculate (min, max, mom, lcount);

	if (longer) {
		print ("      RVariable", 16);
		print ("        Samples", 16);
		print ("        Minimum", 16);
		print ("        Maximum", 16);
		if (moments > 0)
			print ("            Mean", 16);
		Ouf << '\n';
	}

	if (hdr == NULL) hdr = getOName ();

	print (hdr, 16); Ouf << ' ';
#if     BIG_precision != 1
	if (stype(s) < 0) {             // Big count
		print (bcount, NULL, 15, 0);
		bc = bcount;
	} else
#endif
	{
		print (lcount, NULL, 15, 0);
		bc = lcount;
	}
	if (bc > BIG_0) {
		Ouf << ' ';
		print (min, NULL, 15, 0);      Ouf << ' ';
		print (max, NULL, 15, 0);
		if (moments > 0) {
			Ouf << ' ';
			print (mom[0], NULL, 15, 0);
		}
	}
	Ouf << '\n';
}

void    RVariable::exDisplay0 (int reg) {

/* -------------------------------------- */
/* Display full contents of the RVariable */
/* -------------------------------------- */

	double  mom [BMOMENT+MAX_MOMENTS], min, max, *x;
	LONG    lcount;
	BIG     bc;

#if     BIG_precision != 1
	BIG     bcount;
#endif
	LONG    i, moments;
	int     nr;

	if (reg) {              // Start with region

	    if (DisplayOpening) {
		// First time around -- create  the region array
		x = new double [MAX_REGION + 1];
		x [0] = 0.0;    // No samples so far
		TheWFrame = x;
	    } else if (DisplayClosing) {
		// Last time around -- deallocate the region array
		x = (double*) TheWFrame;
		delete (x);
		return;
	    }

	    // A regular call

	    x = (double*) TheWFrame;    // Recover region arrays
	    nr = (int) x[0];            // Counter
	    x++;

	    // OK -- the array is set up
	}

	if (stype(s) < 0) moments = - stype(s) - 1; else moments = stype(s);

#if     BIG_precision != 1
	if (stype (s) < 0) {
		calculate (min, max, mom, bcount);
		bc = bcount;
	} else
#endif
	{
		calculate (min, max, mom, lcount);
		bc = lcount;
	}

	if (reg) {

	    // Print the region first

	    if (nr < MAX_REGION) {
		// Filling up
		x [nr++] = mom [0];
		*(x-1) = nr;
	    } else {
		// Shift down
		for (i = 0; i < MAX_REGION-1; i++) x [i] = x [i+1];
		x [MAX_REGION-1] = mom [0];
	    }

	    startRegion (0.0, (double)(MAX_REGION-1), min, max);
	    startSegment ();
	    for (i = 0; i < nr; i++) displayPoint ((double)i, x[i]);
	    endSegment ();
	    endRegion ();

	    // Done, now the numerical part     
	}

#if     BIG_precision != 1
	if (stype(s) < 0)               // Big count
		display (bcount);       // Number of samples
	else
#endif
		display (lcount);

	if (bc > BIG_0) {
		display (min);
		display (max);
		for (i = 0; i < moments; i++) {
			if (i == 1) {
			   // Standard deviation
			   display (min = (mom [i] >= 0.0) ?
				sqrt (mom [i]) : 0.0);
			   if (convertible (bc))
			     max = (double) bc;
			   else
			     max = HUGE;
			   display (confidence (ZALPHA95, mom [0], min, max));
			}
			display (mom [i]);
		}
	} else {
		display ("undefined");
		display ("undefined");
		for (i = 0; i < moments; i++) {
			if (i == 1) {
				display ("undefined");
				display ("undefined");
			}
			display ("undefined");
		}
	}
}
   
void    RVariable::exDisplay1 () {

/* --------------------------------------------- */
/* Display single-line contents of the RVariable */
/* --------------------------------------------- */

	double  mom [BMOMENT+MAX_MOMENTS], min, max;
	LONG    lcount;
	BIG     bc;

#if     BIG_precision != 1
	BIG     bcount;
#endif
	LONG    moments;

	if (stype(s) < 0) moments = - stype(s) - 1; else moments = stype(s);

#if     BIG_precision != 1
	if (stype (s) < 0)
		calculate (min, max, mom, bcount);
	else
#endif
		calculate (min, max, mom, lcount);

#if     BIG_precision != 1
	if (stype(s) < 0) {             // Big count
		display (bcount);       // Number of samples
		bc = bcount;
	} else
#endif
	{
		display (lcount);
		bc = lcount;
	}

	if (bc > BIG_0) {
		display (min);
		display (max);

		if (moments > 0) {
			display (mom[0]);               // Mean
			// Standard deviation
			display ((mom [1] >= 0.0 ) ? sqrt (mom [1]) : 0.0);
		} else {
			display ("---");
			display ("---");
		}
	} else {
		display ("undefined");
		display ("undefined");

		if (moments > 0) {
			display ("undefined");
			display ("undefined");
		} else {
			display ("---");
			display ("---");
		}
	}
}

void    RVariable::calculate (double &min, double &max, double *mmnts,
		LONG &count) {

/* ---------------------------------------------------------- */
/* Converts  the  contents of a TYPE_long RVariable to actual */
/* measures                                                   */
/* ---------------------------------------------------------- */

	LONG    moments, j, k;
	double  sum, f, a, m;

	moments = stype (s);

#if     BIG_precision != 1
	Assert (moments >= 0, "RVariable->calculate: illegal counter type");
#else
	if (moments < 0) moments = - moments - 1;
#endif
	Assert (moments <= MAX_MOMENTS,
			"RVariable->calculate: illegal RVariable contents");

	count = counter(s);
	min = minimum (s);
	max = maximum (s);
	if (moments == 0) return;
	mmnts [0] = moment(s,0);
	if (moments < 2) return;
	mmnts [1] = moment(s,1) - (mmnts[0] * mmnts[0]);  // Variance

	for (j = 3; j <= moments; j++) {
		for (sum = 0.0, k = 0, f = 1.0, m = 1.0; k <= j; k++) {
			if (j == k)
				a = f * m;
			else
				a = f * moment (s, j - k - 1) * m;
			m *= moment (s, 0);
			if (k & 1)
				sum -= a;
			else
				sum += a;
			f = (f * (j - k)) / (k + 1);
		}
		mmnts [j-1] = sum;
	}
}

#if     BIG_precision != 1

void    RVariable::calculate (double &min, double &max, double *mmnts,
		BIG &count) {

/* ---------------------------------------------------------- */
/* Converts  the  contents of a TYPE_long RVariable to actual */
/* measures                                                   */
/* ---------------------------------------------------------- */

	LONG    moments, j, k;
	double  sum, f, a, m;

	moments = stype(s);
	Assert (moments < 0, "RVariable->calculate: illegal counter type");

	moments = - moments - 1;

	Assert (moments <= MAX_MOMENTS,
		"RVariable: illegal RVariable contents");

	count = bcounter(s);
	min = bminimum (s);
	max = bmaximum (s);
	if (moments == 0) return;
	mmnts [0] = bmoment(s,0);       // The mean value
	if (moments < 2) return;
	mmnts [1] = bmoment(s,1) - (mmnts[0] * mmnts[0]);  // Variance
	if (mmnts [1] < 0.0) mmnts [1] = 0.0;              // Just in case

	for (j = 3; j <= moments; j++) {
		for (sum = 0.0, k = 0, f = 1.0, m = 1.0; k <= j; k++) {
			if (j == k)
				a = f * m;
			else
				a = f * bmoment (s, j - k - 1) * m;
			m *= bmoment (s, 0);
			if (k & 1)
				sum -= a;
			else
				sum += a;
			f = (f * (j - k)) / (k + 1);
		}
		mmnts [j-1] = sum;
	}
}

#endif

void    combineRV (RVariable *ss, RVariable *tt, RVariable *rr) {

/* ------------------------------- */
/* Combine two RVariables into one */
/* ------------------------------- */


	LONG    moments, i;
	double  sc, tc;
#if     BIG_precision != 1
	BIG     scounts, tcounts, rcounts;
	int     smin, smax, smm, tmin, tmax, tmm;
#endif
	LONG    rcntr;

	if ((stype(ss->s) >= 0) && (stype(tt->s) >= 0)) {

		Assert ((stype(ss->s) <= MAX_MOMENTS) && (stype(tt->s) <=
			MAX_MOMENTS),
				"combineRV: illegal RVariable contents");

		// Both input RVariables are of TYPE_long

		moments = (stype(ss->s) > stype(tt->s)) ? stype(tt->s) :
			stype(ss->s);
		stype(rr->s) = moments;

#if     BIG_precision == 1
PROCESS_AS_REGULAR:
#endif
		rcntr = counter(ss->s) + counter(tt->s);

		maximum(rr->s) = (maximum(ss->s) > maximum(tt->s)) ?
			maximum(ss->s) : maximum(tt->s);

		minimum(rr->s) = (minimum(ss->s) < minimum(tt->s)) ?
			minimum(ss->s) : minimum(tt->s);

		if (rcntr == 0) tc = 1.0; else tc = (double) rcntr;

		sc = ((double)(counter(ss->s))) / tc;
		tc = ((double)(counter(tt->s))) / tc;

		counter(rr->s) = rcntr;

		for (i = 0; i < moments; i++)
			moment(rr->s,i) = sc * (moment(ss->s,i)) + tc *
				(moment(tt->s,i));
		return;
	}

	// At least one RVariable id TYPE_BIG

#if     BIG_precision == 1
	if ((moments = stype(ss->s)) < 0) moments = - moments - 1;
	if ((i = stype(tt->s)) < 0) i = - i - 1;
	Assert ((moments <= MAX_MOMENTS) && (i <= MAX_MOMENTS),
		"combineRV: illegal RVariable contents");
	if (i < moments) moments = i;
	stype(rr->s) = - moments - 1;
	goto PROCESS_AS_REGULAR;
#else
	if (stype(ss->s) < 0) {
		moments = - stype(ss->s) - 1;
		scounts = bcounter(ss->s);
		smin = BMIN;
		smax = BMAX;
		smm  = BMOMENT;
	} else {
		moments = stype(ss->s);
		// Convert count to BIG
		scounts = counter(ss->s);
		smin = MIN;
		smax = MAX;
		smm  = MOMENT;
	}

	if (stype(tt->s) < 0) {
		i = - stype(tt->s) - 1;
		tcounts = bcounter(tt->s);
		tmin = BMIN;
		tmax = BMAX;
		tmm  = BMOMENT;
	} else {
		i = stype(tt->s);
		// Convert count to BIG
		tcounts = counter(tt->s);
		tmin = MIN;
		tmax = MAX;
		tmm  = MOMENT;
	}

	Assert ((moments <= MAX_MOMENTS) && (i <= MAX_MOMENTS),
		"combineRV: illegal RVariable contents");

	if (i < moments) moments = i;
	stype(rr->s) = - moments - 1;

	rcounts = bcounter(rr->s) = scounts + tcounts;

	bmaximum(rr->s) = (((double*)(ss->s))[smax]) >
		(((double*)(tt->s))[tmax]) ?
			(((double*)(ss->s))[smax]) : (((double*)(tt->s))[tmax]);
	bminimum(rr->s) = (((double*)(ss->s))[smin]) <
		(((double*)(tt->s))[tmin]) ?
			(((double*)(ss->s))[smin]) : (((double*)(tt->s))[tmin]);

	if (moments == 0) return;

	while (! convertible (rcounts)) {

		/* Ssssssssslllllllllloooooooooowwwwwwwwww !!!!! */

		scounts /= 2;
		tcounts /= 2;
		rcounts /= 2;
	}

	if (rcounts == BIG_0) tc = 1.0; else tc = rcounts;

	sc = (double) scounts / tc;
	tc = (double) tcounts / tc;

	for (i = 0; i < moments; i++)
		bmoment(rr->s,i) = sc * (((double*) ss->s)[smm+i]) +
					tc * (((double*) tt->s)[tmm+i]);
#endif
}

#endif	/* NOS */
