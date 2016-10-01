#include "network.h"

#include "wchansh.cc"
#include "wchansd.cc"
#include "wchannt.cc"

#define	MAX_UINT	((unsigned short) 0xFFFF)

int channel_type = -1;
sxml_t xml_data;

// ============================================================================

static void xenf (const char *s, const char *w) {
	excptn ("Root: %s specification not found within %s", s, w);
}

static void xemi (const char *s, const char *w) {
	excptn ("Root: %s attribute missing from %s", s, w);
}

static void xeai (const char *s, const char *w, const char *v) {
	excptn ("Root: attribute %s in %s has invalid value: %s", s, w, v);
}

static void xevi (const char *s, const char *w, const char *v) {
	excptn ("Root: illegal %s value in %s: %s", s, w, v);
}

static void xeni (const char *s) {
	excptn ("Root: %s table too large, increase NPTABLE_SIZE", s);
}

static void xmon (int nr, const unsigned short *wt, const double *dta,
								const char *s) {
//
// Validates the monotonicity of data to be put into an IVMapper
//
	int j;
	Boolean dec;

	if (nr < 2)
		return;

	for (j = 1; j < nr; j++) {
		if (wt [j] <= wt [j-1])
			excptn ("Root: representation entries in mapper %s "
				"are not strictly increasing", s);
	}

	dec = dta [1] < dta [0];
	for (j = 1; j < nr; j++) {
		if (( dec && dta [j] >= dta [j-1]) ||
	            (!dec && dta [j] <= dta [j-1])  )
			excptn ("Root: value entries in mapper %s are not "
				"strictly monotonic", s);
	}
}

static void sptypes (nparse_t *np, int tp) {

	int i;

	for (i = 0; i < NPTABLE_SIZE; i++)
		np [i].type = tp;
}

static int nstrcmp (const char *a, const char *b) {
//
// String compare that accounts for NULL
//
	if (a == NULL)
		return b != NULL;

	if (b == NULL)
		return -(a != NULL);

	return strcmp (a, b);
}

// ============================================================================

static void init_channel (int NN) {

	const char *sfname, *att, *xnam;
	double bn_db, beta, dref, sigm, loss_db, psir, pber, cutoff;
	nparse_t np [NPTABLE_SIZE];
	int K, nb, nr, i, j, syncbits, bpb, frml;
	sxml_t data, prp, cur;
	sir_to_ber_t	*STB;
	IVMapper	*ivc [6];
	MXChannels	*mxc;
	unsigned short wn, *wt;
	Boolean rmo, symm;
	double *dta, *dtb, *dtc;

	Ether = NULL;

	sptypes (np, TYPE_double);

	// Preset this to NULL
	memset (ivc, 0, sizeof (ivc));

	if ((data = sxml_child (xml_data, "channel")) == NULL)
		excptn ("Root: no <channel> element");

	// Determine the channel type
	if ((prp = sxml_child (data, "propagation")) == NULL)
		xenf ("<propagation>", "<channel>");

	if ((att = sxml_attr (prp, "type")) == NULL)
		xemi ("type", "<propagation>");

	if (strncmp (att, "sh", 2) == 0)
		channel_type = CTYPE_SHADOWING;
	else if (strncmp (att, "sa", 2) == 0)
		channel_type = CTYPE_SAMPLED;
	else if (strncmp (att, "ne", 2) == 0)
		channel_type = CTYPE_NEUTRINO;
	else
		xeai ("type", "<propagation>", att);

	if (channel_type != CTYPE_NEUTRINO) {

		// Some "common" attributes are ignored for neutrino type
		// channel

		if (channel_type == CTYPE_SAMPLED)
			// We need this one in advance, can be NULL
			sfname = sxml_attr (prp, "samples");

		bn_db = -HUGE;
		// The default value for background noise translating into
		// lin 0.0
		if ((att = sxml_attr (data, "bn")) != NULL) {
			if (parseNumbers (att, 1, np) != 1)
				xeai ("bn", "<channel>", att);
			bn_db = np [0].DVal;
		}

		sigm = -1.0;

		if ((att = sxml_attr (prp, "sigma")) != NULL) {
			// Expect a double
			if (parseNumbers (att, 1, np) != 1 ||
				(sigm = np [0].DVal) < 0.0)
					xeai ("sigma", "<propagation>", att);
		}

		// The BER table
		if ((cur = sxml_child (data, "ber")) == NULL)
			xenf ("<ber>", "<channel>");

		att = sxml_txt (cur);
		nb = parseNumbers (att, NPTABLE_SIZE, np);
		if (nb > NPTABLE_SIZE)
			excptn ("Root: <ber> table too large, "
				"increase NPTABLE_SIZE");

		if (nb < 4 || (nb & 1) != 0) {

			if (nb < 0)
				excptn ("Root: illegal numerical value in "
					"<ber> table");
			else
				excptn ("Root: illegal size of <ber> table "
					"(%1d), must be an even number >= 4",
					nb);
		}

		psir = HUGE;
		pber = -1.0;
		// This is the size of BER table
		nb /= 2;
		STB = new sir_to_ber_t [nb];

		for (i = 0; i < nb; i++) {
			// The SIR is stored as a linear ratio
			STB [i].sir = dBToLin (np [2 * i] . DVal);
			STB [i].ber = np [2 * i + 1] . DVal;
			// Validate
			if (STB [i] . sir >= psir)
				excptn ("Root: SIR entries in <ber> must be "
					"monotonically decreasing, %f and %f "
					"aren't", psir, STB [i] . sir);
			psir = STB [i] . sir;
			if (STB [i] . ber < 0)
				excptn ("Root: BER entries in <ber> must not "
					"be negative, %g is", STB [i] . ber);
			if (STB [i] . ber <= pber)
				excptn ("Root: BER entries in <ber> must be "
					"monotonically increasing, %g and %g "
					"aren't", pber, STB [i] . ber);
			pber = STB [i] . ber;
		}

		// The cutoff threshold wrt to background noise: the default
		// means no cutoff
		cutoff = -HUGE;
		if ((cur = sxml_child (data, "cutoff")) != NULL) {
			att = sxml_txt (cur);
			if (parseNumbers (att, 1, np) != 1)
				xevi ("<cutoff>", "<channel>", att);
			cutoff = np [0].DVal;
		}

		// Power
		if ((cur = sxml_child (data, "power")) == NULL)
			xenf ("<power>", "<network>");

		att = sxml_txt (cur);

		// Check for a single double value first
		if (parseNumbers (att, 2, np) == 1) {
			// We have a single entry case
			wt = new unsigned short [1];
			dta = new double [1];
			wt [0] = 0;
			dta [0] = np [0] . DVal;
			nr = 1;
		} else {
			for (i = 0; i < NPTABLE_SIZE; i += 2)
				np [i ]  . type = TYPE_int;
			nr = parseNumbers (att, NPTABLE_SIZE, np);
			if (nr > NPTABLE_SIZE)
				xeni ("<power>");

			if (nr < 2 || (nr % 2) != 0) 
				excptn ("Root: number of items in <power> must "
					"be either 1, or a nonzero multiple of "
					"2");
			nr /= 2;
			wt = new unsigned short [nr];
			dta = new double [nr];

			for (j = 0; j < nr; j++) {
				wt [j] = (unsigned short) (np [2*j] . IVal);
				dta [j] = np [2*j + 1] . DVal;
			}
			xmon (nr, wt, dta, "<power>");
			for (i = 0; i < NPTABLE_SIZE; i += 2)
				np [i] . type = TYPE_double;
			sptypes (np, TYPE_double);
		}

		ivc [XVMAP_PS] = new IVMapper (nr, wt, dta, YES);

		// RSSI map (optional for shadowing)
		if ((cur = sxml_child (data, "rssi")) != NULL) {

			att = sxml_txt (cur);

			for (i = 0; i < NPTABLE_SIZE; i += 2)
				np [i] . type = TYPE_int;

			nr = parseNumbers (att, NPTABLE_SIZE, np);
			if (nr > NPTABLE_SIZE)
				xeni ("<rssi>");

			if (nr < 2 || (nr % 2) != 0) 
				excptn ("Root: number of items in <rssi> must "
						"be a nonzero multiple of 2");
			nr /= 2;
			wt = new unsigned short [nr];
			dta = new double [nr];

			for (j = 0; j < nr; j++) {
				wt [j] = (unsigned short) (np [2*j] . IVal);
				dta [j] = np [2*j + 1] . DVal;
			}
			xmon (nr, wt, dta, "<rssi>");

			ivc [XVMAP_RSSI] = new IVMapper (nr, wt, dta, YES);

			sptypes (np, TYPE_double);

		} else if (channel_type == CTYPE_SAMPLED && sfname != NULL) {

			excptn ("Root: \"sampled\" propagation model with "
				"non-empty sample file requires RSSI table");
		}
	}

	if (channel_type == CTYPE_SHADOWING) {

		if (sigm < 0.0)
			// Use the default sigma of zero if missing
			sigm = 0.0;

		att = sxml_txt (prp);
		if ((nb = parseNumbers (att, 4, np)) != 4) {
			if (nb < 0)
				excptn ("Root: illegal number in "
					"<propagation>");
			else
				excptn ("Root: expected 4 numbers in "
					"<propagation>, found %1d", nb);
		}

		if (np [0].DVal != -10.0)
			excptn ("Root: the factor in propagation formula "
				"must be -10, is %f", np [0].DVal);

		beta = np [1].DVal;
		dref = np [2].DVal;
		loss_db = np [3].DVal;

	} else if (channel_type == CTYPE_SAMPLED) {

		K = 3;					// Sigma threshold
		if ((att = sxml_attr (prp, "average")) != NULL) {
			np [0].type = TYPE_int;
			nr = parseNumbers (att, NPTABLE_SIZE, np);
			if (nr != 1)
				xeai ("average", "<propagation>", att);
			K = (int) (np [0].LVal);
			if (K < 1 || K > 32)
				excptn ("Root: the number of samples to "
					"average must be between 1 and 32, is "
					"%1d", K);
			np [0].type = TYPE_double;
		}

		symm = NO;
		if ((att = sxml_attr (prp, "symmetric")) != NULL &&
			*att == 'y')
				symm = YES;

		att = sxml_txt (prp);

		// Expect the attenuation table

		nr = parseNumbers (att, NPTABLE_SIZE, np);
		if (nr > NPTABLE_SIZE)
			xeni ("<propagation>");

		if (sigm < 0.0) {
			// Just in case, make sure this is always sane
			sigm = 0.0;
			// We expect triplets: distance, attenuation, sigma
			if (nr < 6 || (nr % 3) != 0)
				excptn ("Root: the number of entries in "
					"<propagation> table must be divisible "
					"by 3 and at least equal 6");
			nr /= 3;
			dta = new double [nr];
			dtb = new double [nr];
			dtc = new double [nr];

			for (i = 0; i < nr; i++) {

				dta [i] = np [3 * i    ] . DVal;
				dtb [i] = np [3 * i + 1] . DVal;
				dtc [i] = np [3 * i + 2] . DVal;

				if (dta [i] < 0.0) {
PTErr1:
					excptn ("Root: negative distance in the"
						" <propagation> table, "
						"%g [%1d]",
						dta [i], i);
				}
				if (dtc [i] < 0.0)
					excptn ("Root: negative sigma in the"
						" <propagation> table, "
						"%g [%1d]",
						dtc [i], i);
				// Note: we do not insist on the sigmas being
				// monotonic. As we never lookup distances
				// based on sigma, it should be OK.
				if (i > 0) {
				    if (dta [i] <= dta [i - 1]) {
PTErr2:
					excptn ("Root: distances in the "
						"<propagation> table are not "
						"strictly increasing, "
						"%g [%1d] <= %g [%1d]",
							dta [i], i,
							dta [i - 1], i - 1);
				    }
				    if (dtb [i] >= dtb [i - 1]) {
PTErr3:
					excptn ("Root: levels in the "
						"<propagation> table are not "
						"strictly decreasing, "
						"%g [%1d] >= %g [%1d]",
							dtb [i], i,
							dtb [i - 1], i - 1);
				    }
			        }
			}

			ivc [XVMAP_SIGMA] =
				(IVMapper*) new DVMapper (nr, dta, dtc, NO);

		} else {
			// Dublets (sigma is fixed)
			if (nr < 4 || (nr % 2) != 0)
				excptn ("Root: <propagation> table needs an "
				    "even number of entries and at least 4");
			nr /= 2;

			dta = new double [nr];
			dtb = new double [nr];

			for (i = 0; i < nr; i++) {

				dta [i] = np [2 * i    ] . DVal;
				dtb [i] = np [2 * i + 1] . DVal;

				if (dta [i] < 0.0)
					goto PTErr1;

				if (i > 0) {
			    		if (dta [i] <= dta [i - 1])
						goto PTErr2;
			    		if (dtb [i] >= dtb [i - 1])
						goto PTErr3;
				}
			}

		}

		ivc [XVMAP_ATT] = (IVMapper*) new DVMapper (nr, dta, dtb, YES);

	} else {

		// Neutrino channel
		if ((att = sxml_attr (prp, "range")) != NULL) {
			nr = parseNumbers (att, 1, np);
			if (nr == 0)
				xeai ("range", "<propagation>", att);
			dref = np [0].DVal;
			if (dref <= 0.0)
				excptn ("Root: the propagation range must be > "
					"0.0, is %g", dref);
		} else {
			dref = HUGE;
		}
	}

	// Frame parameters
	if ((cur = sxml_child (data, "frame")) == NULL)
		xenf ("<frame>", "<channel>");
	att = sxml_txt (cur);
	np [0] . type = np [1] . type = np [2] . type = TYPE_LONG;
	if ((nr = parseNumbers (att, 3, np)) != 3) {
		if (nr != 2) {
			xevi ("<frame>", "<channel>", att);
		} else {
			syncbits = 0;
			bpb      = (int) (np [0].LVal);
			frml     = (int) (np [1].LVal);
		}
	} else {
		syncbits = (int) (np [0].LVal);
		bpb      = (int) (np [1].LVal);
		frml     = (int) (np [2].LVal);
	}

	if (syncbits < 0 || bpb <= 0 || frml < 0)
		xevi ("<frame>", "<channel>", att);

	// Prepare np for reading value mappers
	for (i = 0; i < NPTABLE_SIZE; i += 2) {
		np [i  ] . type = TYPE_int;
		np [i+1] . type = TYPE_double;
	}

	if ((cur = sxml_child (data, "rates")) == NULL)
		xenf ("<rates>", "<network>");

	// This tells us whether we should expect boost factors
	rmo = !nstrcmp (sxml_attr (cur, "boost"), "yes");
	att = sxml_txt (cur);

	if (rmo) {
		// Expect sets of triplets: int int double
		for (i = 0; i < NPTABLE_SIZE; i += 3) {
			np [i  ] . type = np [i+1] . type = TYPE_int;
			np [i+2] . type = TYPE_double;
		}
		nr = parseNumbers (att, NPTABLE_SIZE, np);
		if (nr > NPTABLE_SIZE)
			xeni ("<rates>");

		if ((nr < 3) || (nr % 3) != 0)
			excptn ("Root: number of items in <rates> must be a"
				" nonzero multiple of 3");
		nr /= 3;
		wt = new unsigned short [nr];
		dta = new double [nr];
		dtb = new double [nr];

		for (j = 0; j < nr; j++) {
			wt [j] = (unsigned short) (np [3*j] . IVal);
			// Actual rates go first (stored as double)
			dta [j] = (double) (np [3*j + 1] . IVal);
			// Boost
			dtb [j] = np [3*j + 2] . DVal;
		}

		xmon (nr, wt, dta, "<rates>");
		xmon (nr, wt, dtb, "<rates>");

		ivc [XVMAP_RATES] = new IVMapper (nr, wt, dta);
		ivc [XVMAP_RBOOST] = new IVMapper (nr, wt, dtb, YES);

	} else {

		// No boost specified, the boost IVMapper is null, which
		// translates into the boost of 1.0

		for (i = 0; i < NPTABLE_SIZE; i++)
			np [i] . type = TYPE_int;

		nr = parseNumbers (att, NPTABLE_SIZE, np);
		if (nr > NPTABLE_SIZE)
			xeni ("<rates>");

		if (nr < 2) {
			if (nr < 1) {
RVErr:
				excptn ("Root: number of items in <rates> must "
					"be either 1, or a nonzero multiple of "
						"2");
			}
			// Single entry - a special case
			wt = new unsigned short [1];
			dta = new double [1];
			wt [0] = 0;
			dta [0] = (double) (np [0] . IVal);
		} else {
			if ((nr % 2) != 0)
				goto RVErr;

			nr /= 2;
			wt = new unsigned short [nr];
			dta = new double [nr];

			for (j = 0; j < nr; j++) {
				wt [j] = (unsigned short) (np [2*j] . IVal);
				dta [j] = (double) (np [2*j + 1] . IVal);
			}
			xmon (nr, wt, dta, "<rates>");
		}

		ivc [XVMAP_RATES] = new IVMapper (nr, wt, dta);
	}

	// Channels

	if ((cur = sxml_child (data, "channels")) == NULL) {
		mxc = new MXChannels (1, 0, NULL);
	} else {
		if ((att = sxml_attr (cur, "number")) != NULL ||
		    (att = sxml_attr (cur, "n")) != NULL ||
		    (att = sxml_attr (cur, "count")) != NULL) {
			// Extract the number of channels
			np [0] . type = TYPE_LONG;
			if (parseNumbers (att, 1, np) != 1)
				xevi ("<channels>", "<channel>", att);
			if (np [0] . LVal < 1 || np [0] . LVal > MAX_UINT)
				xevi ("<channels>", "<channel>", att);
			wn = (unsigned short) (np [0] . LVal);
			if (wn == 0)
				xeai ("number", "<channels>", att);

			att = sxml_txt (cur);

			sptypes (np, TYPE_double);

			j = parseNumbers (att, NPTABLE_SIZE, np);

			if (j > NPTABLE_SIZE)
				excptn ("Root: <channels> separation table too"
					" large, increase NPTABLE_SIZE");

			if (j < 0)
				excptn ("Root: illegal numerical value in the "
					"<channels> separation table");

			if (j == 0) {
				// No separations
				dta = NULL;
			} else {
				dta = new double [j];
				for (i = 0; i < j; i++)
					dta [i] = np [i] . DVal;
			}

			mxc = new MXChannels (wn, j, dta);
		} else
			xemi ("number", "<channels>");
	}

	print ("\n");

	// Create the channel

	if (channel_type == CTYPE_SHADOWING)
		create RFShadow (NN, STB, nb, dref, loss_db, beta, sigm, bn_db,
			bn_db, cutoff, syncbits, bpb, frml, ivc, mxc, NULL);
	else if (channel_type == CTYPE_SAMPLED)
		create RFSampled (NN, STB, nb, K, sigm, bn_db, bn_db,
			cutoff, syncbits, bpb, frml, ivc, sfname, symm,
				mxc, NULL);
	else if (channel_type == CTYPE_NEUTRINO)
		create RFNeutrino (NN, dref, bpb, frml, ivc, mxc);

}

Long initNetwork () {

	double grid;
	const char *att;
	nparse_t np [1];
	sxml_t data;
	int NN, qual;

	xml_data = sxml_parse_input ('\0', NULL, NULL);

	if (!sxml_ok (xml_data))
		excptn ("Root: XML input data error, %s",
			(char*)sxml_error (xml_data));

	if (strcmp (sxml_name (xml_data), "network"))
		excptn ("Root: <network> data expected");

	np [0] . type = TYPE_LONG;

	// Decode the number of stations
	if ((att = sxml_attr (xml_data, "nodes")) == NULL)
		xemi ("nodes", "<network>");

	if (parseNumbers (att, 1, np) != 1)
		xeai ("nodes", "<network>", att);

	NN = (int) (np [0] . LVal);
	if (NN <= 0)
		excptn ("Root: 'nodes' in <network> must be strictly positive, "
			"is %1d", NN);

	// Timing
	np [0].type = TYPE_double;

	if ((data = sxml_child (xml_data, "grid")) != NULL) {
		att = sxml_txt (data);
		if (parseNumbers (att, 1, np) != 1)
			excptn ("Root: <grid> parameter error");
		grid = np [0].DVal;
	} else
		grid = 1.0;

	// ITU is equal to the propagation time across grid unit, assuming 1
	// ETU == 1 second
	setEtu (SOL_VACUUM / grid);

	// DU is equal to the propagation time (in ITU) across 1m
	setDu (1.0/grid);

	// Clock tolerance
	if ((data = sxml_child (xml_data, "tolerance")) != NULL) {
		att = sxml_txt (data);
		if (parseNumbers (att, 1, np) != 1)
			excptn ("Root: <tolerance> parameter error: %s",
				att);
		grid = np [0].DVal;
		if ((att = sxml_attr (data, "quality")) != NULL) {
			np [0].type = TYPE_LONG;
			if (parseNumbers (att, 1, np) != 1)
				excptn ("Root: <tolerance> 'quality' format "
					"error: %s", att);
			qual = (int) (np [0].LVal);
		} else
			qual = 2;	// This is the default

		setTolerance (grid, qual);
	}

	init_channel (NN);

	return NN;
}

// ============================================================================

void setrfpowr (Transceiver *rfi, unsigned short ix) {
//
// Set X power
//
	IVMapper *m;
	double v;

	if (channel_type == CTYPE_NEUTRINO) {
		v = 1.0;
	} else {
		m = Ether -> PS;
		assert (m->exact (ix), "setrfpowr: illegal power index %1d",
			ix);
		v = m->setvalue (ix);
	}

	rfi -> setXPower (v);
}

void setrfrate (Transceiver *rfi, unsigned short ix) {
//
// Set RF rate
//
	double rate;
	IVMapper *m;

	m = Ether -> Rates;

	assert (m->exact (ix), "setrfrate: illegal rate index %1d", ix);

	rate = m->setvalue (ix);
	rfi->setTRate ((RATE) round ((double)etuToItu (1.0) / rate));
	rfi->setTag ((rfi->getTag () & 0xffff) | (ix << 16));
}

void setrfchan (Transceiver *rfi, unsigned short ch) {
//
// Set RF channel
//
	MXChannels *m;

	m = Ether -> Channels;

	assert (ch <= m->max (), "setrfchan: illegal channel %1d", ch);

	rfi->setTag ((rfi->getTag () & ~0xffff) | ch);
}

