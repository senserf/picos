#ifndef	__wchansd_c__
#define	__wchansd_c__

#include "wchansd.h"

#include "wchan.cc"

#undef trc
//#define	trc(a, ...)	trace (a, ## __VA_ARGS__)

#ifndef trc
#define	trc(a, ...)
#endif

#if ZZ_R3D
#define	__ndim 3
#else
#define	__ndim 2
#endif

#define	__nrof (__ndim + __ndim)
#define	__ntrd (__nrof + 2)

typedef struct {
//
// A sample being read from the input data file
//
	sdpair_t	SDP;
	double		Attenuation,
			Sigma;
	Long		NS;		// Number of specimens so far

} rss_rsample_t;

static inline double vlength (sdpair_t &sdp) {
//
// Note that the length is in meters
//
	double x, y;
#if ZZ_R3D
	double z;
#endif

	x = (double) (sdp.XA - sdp.XB);
	y = (double) (sdp.YA - sdp.YB);
#if ZZ_R3D
	y = (double) (sdp.ZA - sdp.ZB);
#endif
	return sqrt (x * x + y * y
#if ZZ_R3D
			   + z * z
#endif
					) / Du;
};

// ============================================================================

void RFSampled::read_samples (const char *sfname) {
//
// Read the input file containing data samples; the line format is:
//
//	Xs Ys Xd Yd RSSI pow (for 3d: Xs Ys Zs Xd Yd Zd RSSI pow)
//
// The last number is optional and defaults to a model parameter
//
	rss_rsample_t		*RSamples;
	Long			SRSamples, ln, e, i;
	FILE			*sf;
	nparse_t		NP [__ntrd];
	sdpair_t		SDP;
	rss_rsample_t           *RSMP;
	rss_sample_t            *SMP, SSMP;
	char			*c, *linebuf;
	size_t			lbs;
	unsigned short		RS, PO;
	unsigned int		uv;
	double			d, att;
	int			cnt;
	
	NSamples = SRSamples = 0;

	print ("\n");

	if ((sf = fopen (sfname, "r")) == NULL) {
#ifdef	DATALIBDIRS
		if (*sfname != '/') {
			// File name is not absolute, try DataLib
			i = 0;
			c = NULL;
			do {
				const char *fc;
				free (c);
				if ((fc = DATALIBDIRS [i++]) == NULL)
					break;
				c = (char*) malloc (strlen (fc) +
						    strlen (sfname) + 2);
				assert (c != NULL, "RFSampled: out of memory "
					"(opening sample file)");
				strcpy (c, fc);
				strcat (c, "/");
				strcat (c, sfname);
				if ((sf = fopen (c, "r")) != NULL) {
					print (form ("  Sample file: %s\n", c));
					free (c);
					goto Opened;
				}
			} while (1);
		}
#endif
		excptn ("RFSampled: cannot open sample file %s", sfname);
	} else {
		print (form ("  Sample file: %s\n", sfname));
	}
Opened:
	trc ("sample file opened");

	for (cnt = 0; cnt < __ndim + __ndim; cnt++)
		NP [cnt] . type = TYPE_double;

	NP [__nrof + 0] . type =
	NP [__nrof + 1] . type =
						TYPE_int;

	ln = 0;
	lbs = 0;
	linebuf = NULL;
	while (getline (&linebuf, &lbs, sf) >= 0) {

		ln++;

		// Ignore empty lines and comments (starting with #)
		for (c = linebuf; isspace (*c); c++);
		if (*c == '#' || *c == '\0')
			continue;

		e = parseNumbers (linebuf, __ntrd, NP);
		if (e < __ntrd - 1 || e > __ntrd)
			excptn ("RFSampled: illegal content of line %1d in the "
				"sample file", ln);
		// Read the coordinates
		cnt = 0;
		SDP.XA = (Long) duToItu (NP [cnt++] . DVal);
		SDP.YA = (Long) duToItu (NP [cnt++] . DVal);
#if ZZ_R3D
		SDP.ZA = (Long) duToItu (NP [cnt++] . DVal);
#endif
		SDP.XB = (Long) duToItu (NP [cnt++] . DVal);
		SDP.YB = (Long) duToItu (NP [cnt++] . DVal);
#if ZZ_R3D
		SDP.ZB = (Long) duToItu (NP [cnt++] . DVal);
#endif
		PO = PS->upper ();
		uv = (unsigned int) (NP [cnt++] . IVal);
		assert (uv <= 0x0FFFF, "RFSampled: illegal RSSI (%1u) in line "
			"%1d in the sample file", uv, ln);
		RS = (unsigned short) uv;
		if (e > __ntrd - 1) {
			uv = (unsigned int) (NP [cnt] . IVal);
			assert (uv <= 0x0FFFF,
				"RFSampled: illegal power level (%1u) in line "
				"%1d in the sample file", uv, ln);
			PO = (unsigned short) uv;
		}

		// Look up the same pair of points in the existing set
		
		RSMP = NULL;
		for (i = 0; i < NSamples; i++) {
			if (sdpcmp (SDP, RSamples [i] . SDP)) {
				RSMP = &(RSamples [i]);
				break;
			}
		}

		if (RSMP == NULL) {
			if (NSamples == SRSamples) {
				// Reached the limit of present array
				if (NSamples == 0) {
					// Initial allocation
					SRSamples = 256;
					RSamples = (rss_rsample_t*)
					    malloc (sizeof (rss_rsample_t) *
						SRSamples);
				} else {
					SRSamples += SRSamples;
					RSamples = (rss_rsample_t*)
					    realloc (RSamples,
						sizeof (rss_rsample_t) *
						    SRSamples);
				}
				assert (RSamples != NULL,
					"RFSampled: out of memory "
						"(creating sample list)");
			}
			RSMP = &(RSamples [NSamples]);
			NSamples++;
			RSMP->SDP = SDP;
			RSMP->Attenuation = 0.0;
			RSMP->Sigma = 0.0;
			RSMP->NS = 0;
		}

		// Transform the RSS signal into attenuation in dB; note that
		// attenuation is not the right word: it is in fact
		// "amplification" (negative, less than 1); this is why we
		// subtract transmit power from received signal strength, not
		// the other way around
		att = RSSIC->setvalue (RS, NO) - PS->setvalue (PO, NO);

		//
		// Here's the "online" algorithm (due to Knuth) for mean and
		// variance:
		//
		//	def online_variance(data):
    		//		n = 0
    		//		mean = 0
    		//		M2 = 0
		//
		//		for x in data:
        	//			n = n + 1
        	//			delta = x - mean
        	//			mean = mean + delta/n
        	//			M2 = M2 + delta*(x - mean)
		//
    		//		variance = M2/(n - 1)
    		//		return variance
		//

		RSMP->NS++;
		d = att - RSMP->Attenuation;
		RSMP->Attenuation += d / RSMP->NS;
		RSMP->Sigma += d * (att - RSMP->Attenuation);
	}

	trc ("processed %1d lines\n", ln);

	// We are done with the file
	fclose (sf);
	free (linebuf);

	// Transform the sample array in place into the online database
	Samples = (rss_sample_t*) RSamples;

	assert (NSamples >= K, "RFSampled: less than K=%1d SD samples (%1d) in "
		"the sample file", K, NSamples);

	print (NSamples, "  Number of SD points:", 10, 26);

	for (e = 0; e < NSamples; e++) {

		SMP = Samples + e;
		RSMP = RSamples + e;

		// Prepare the target view
		SSMP.SDP = RSMP->SDP;

		SSMP.Attenuation = (float)(RSMP->Attenuation);
		if (RSMP->NS < 2) {
			// Single-valued sample, use the default Sigma
			SSMP.Sigma = (SIGMA == NULL) ? Sigma :
				SIGMA->setvalue (vlength (SSMP.SDP));
		} else {
			SSMP.Sigma = (float) sqrt (RSMP->Sigma /
				(RSMP->NS - 1));
		}

		memcpy (SMP, &SSMP, sizeof (rss_sample_t));

#if ZZ_R3D
		trc ("SMP <%1d,%1d,%1d> - <%1d,%1d,%1d>, %g, %g",
			SMP->SDP.XA,
			SMP->SDP.YA,
			SMP->SDP.ZA,
			SMP->SDP.XB,
			SMP->SDP.YB,
			SMP->SDP.ZB,
			SMP->Attenuation,
			SMP->Sigma);
#else
		trc ("SMP <%1d,%1d> - <%1d,%1d>, %g, %g",
			SMP->SDP.XA,
			SMP->SDP.YA,
			SMP->SDP.XB,
			SMP->SDP.YB,
			SMP->Attenuation,
			SMP->Sigma);
#endif
	}

	Samples = (rss_sample_t*) realloc (Samples, NSamples *
		sizeof (rss_sample_t));
}

// ============================================================================
	
void RFSampled::setup (

	Long nt,		// The number of transceivers
	const sir_to_ber_t *st,	// SIR to BER conversion table
	int    sl,		// Length of the conversion table
	int	k,		// Number of closest samples to average
	double eaf,		// Exponential averaging factor
	double sig,		// Default lognormal random component
	double no,		// Background noise (dBm)
	double bu,		// Channel busy signal threshold dBm
	double co,		// Cut off signal level
	Long mp,		// Minimum received preamble length
	int bpb,		// Bits per byte
	int frm,		// Packet frame (extra physical bits)
	IVMapper **ivcc,	// Value converters
	const char *sfname,	// Data file name
	Boolean symm,		// Symmetric
	MXChannels *mxc,
	double (*g) (Transceiver*, Transceiver*)
) {
	double	r, v;
	int 	i;
	size_t	s;

	RadioChannel::setup (nt, no, st, sl, bpb, frm, ivcc, mxc);
	gain = g;
	COSL = (co == -HUGE) ? 0.0 : dBToLin (co);
	MinPr = mp;
	BThrs = dBToLin (bu);
	Symmetric = symm;
	Sigma = sig;
	EAF = eaf;

	if ((K = k) > RFSMPL_MAXK)
		excptn ("RFSampled: samples to average larger than max (%1d)",
			RFSMPL_MAXK);
	ATTB = (DVMapper*)(ivcc [XVMAP_ATT]);
	SIGMA = (DVMapper*)(ivcc [XVMAP_SIGMA]);

	print (MinPr, 		"  Minimum preamble:", 10, 26);
	print (bu,    		"  Activity threshold:", 10, 26);
	print (co,    		"  Cutoff threshold:", 10, 26);
	print (EAF,   		"  Averaging exponent:", 10, 26);
	print (K,     		"  Samples to average:", 10, 26);
	print (Symmetric, 	"  Symmetric:", 10, 26);

	print ("\n  Distance(m)  Attenuation(dB)    Sigma\n");

	for (i = 0; i < ATTB->size (); i++) {
		r = ATTB->row (i, v);
		print (form ("  %11.3f       %8g %8g\n", r, v, sigma (r)));
	}

	NSamples = 0;
	Samples = NULL;
	HTable = NULL;

	if (sfname) {
		// There is a sample file
		read_samples (sfname);
		// Allocate the hash table
		if ((HTable = (dict_item_t**)malloc (s = sizeof (dict_item_t*) *
		    RFSMPL_HASHSIZE)) == NULL)
			excptn ("RFSampled: out of memory allocating hash "
				"table");
		memset (HTable, 0, s);
	}
}

// ============================================================================

double RFSampled::attenuate (sdpair_t &sdp) {

	dict_item_t	*di, *dh;
	Long		rss, ix;
	double 		res, d;

	if (HTable == NULL) {
		// No sampled data, fallback to the table
		d = vlength (sdp);
		res = dBToLin (ATTB->setvalue (vlength (sdp), NO) +
			dRndGauss (0.0, sigma (d)));
		trc ("attenuate: no sampled data %g %g", res, sigma (d));
		return res;
	}

	for (dh = NULL, di = HTable [ix = hash (sdp)]; di != NULL;
	    dh = di, di = (dict_item_t*)(di->Rehash))
		if (sdpcmp (sdp, di->SMP.SDP))
		    break;

	if (di != NULL) {
		// Use this entry, but also move it to the front
		if (dh) {
			dh->Rehash = di->Rehash;
			di->Rehash = (void*) (HTable [ix]);
			HTable [ix] = (dict_item_t*) di;
		}
#if ZZ_R3D
		trc ("attenuate HE found: <%1d,%1d,%1d> - <%1d,%1d,%1d>, "
			"%g, %g",
			di->SMP.SDP.XA,
			di->SMP.SDP.YA,
			di->SMP.SDP.ZA,
			di->SMP.SDP.XB,
			di->SMP.SDP.YB,
			di->SMP.SDP.ZB,
			di->SMP.Attenuation,
			di->SMP.Sigma);
#else
		trc ("attenuate HE found: <%1d,%1d> - <%1d,%1d>, %g, %g",
			di->SMP.SDP.XA,
			di->SMP.SDP.YA,
			di->SMP.SDP.XB,
			di->SMP.SDP.YB,
			di->SMP.Attenuation,
			di->SMP.Sigma);
#endif
	} else {
		// Need to build a new entry
		di = interpolate (sdp);
		di -> Rehash = (void*) (HTable [ix]);
		HTable [ix] = di;
#if ZZ_R3D
		trc ("attenuate new HE: <%1d,%1d,%1d> - <%1d,%1d,%1d>, %g, %g",
			di->SMP.SDP.XA,
			di->SMP.SDP.YA,
			di->SMP.SDP.ZA,
			di->SMP.SDP.XB,
			di->SMP.SDP.YB,
			di->SMP.SDP.ZB,
			di->SMP.Attenuation,
			di->SMP.Sigma);
#else
		trc ("attenuate new HE: <%1d,%1d> - <%1d,%1d>, %g, %g",
			di->SMP.SDP.XA,
			di->SMP.SDP.YA,
			di->SMP.SDP.XB,
			di->SMP.SDP.YB,
			di->SMP.Attenuation,
			di->SMP.Sigma);
#endif
	}

	res = dBToLin (di->SMP.Attenuation + dRndGauss (0.0, di->SMP.Sigma));
	trc ("attenuate: %g", res);
	return res;
}

dict_item_t *RFSampled::interpolate (sdpair_t &sdp) {

	rss_sample_t	*s, *BK [RFSMPL_MAXK];
	double		D, A, S, T, W, DBK [RFSMPL_MAXK], TBK [RFSMPL_MAXK];
	Long		NBK, i, j, k;
	dict_item_t	*DI;

#if ZZ_R3D
	trc ("interpolate: <%1d,%1d,%1d> - <%1d,%1d,%1d> l=%g",
		sdp.XA, sdp.YA, sdp.ZA, sdp.XB, sdp.YB, sdb.ZB, vlength (sdp));
#else
	trc ("interpolate: <%1d,%1d> - <%1d,%1d> l=%g", sdp.XA, sdp.YA, sdp.XB,
		sdp.YB, vlength (sdp));
#endif

	for (i = NBK = 0; i < NSamples; i++) {

		// Select K closest samples
		s = Samples + i;
		D = sdpdist (s->SDP, sdp);

		if (NBK == K) {
			// Table already filled
			if (D >= DBK [0])
				// Greater that current max, skip it
				continue;
			// Make sure the max is at the top
			A = D;
			k = 0;
			for (j = 1; j < NBK; j++) {
				if (DBK [j] > A) {
					k = j;
					A = DBK [j];
				}
			}

			if (k) {
				// Move the new max to the top
				BK [0] = BK [k];
				DBK [0] = A;
			}
		} else {
			// Put it in, but keep the max at the top
			if (NBK && DBK [0] < D) {
				// Replace the head
				DBK [NBK] = DBK [0];
				BK [NBK] = BK [0];
				k = 0;
			} else {
				k = NBK;
			}
			NBK++;
		}

		BK [k] = s;
		DBK [k] = D;
	}

	// Calculate the average distance to make the exponential weights
	// scale-invariant
	A = 0.0;
	for (i = 0; i < NBK; i++)
		A += DBK [i];

	A /= NBK;
	if (A == 0.0)
		// This is impossible, but be prepared, e.g., some degenerate
		// case of NBK == 1
		A = 1.0;

	for (i = 0; i < NBK; i++) {
		// Stage 1: replace distances by exponents; the distance is
		// transposed to Du (meters), so we know what the factor (EAF)
		// applies to
#if ZZ_R3D
		trc ("in[%1d] = <%1d,%1d,%1d> - <%1d,%1d,%1d>, %g, %g, d=%g", i,
			BK [i]->SDP.XA,
			BK [i]->SDP.YA,
			BK [i]->SDP.ZA,
			BK [i]->SDP.XB,
			BK [i]->SDP.YB,
			BK [i]->SDP.ZB,
			BK [i]->Attenuation,
			BK [i]->Sigma, DBK [i]);
#else
		trc ("in[%1d] = <%1d,%1d> - <%1d,%1d>, %g, %g, d=%g", i,
			BK [i]->SDP.XA,
			BK [i]->SDP.YA,
			BK [i]->SDP.XB,
			BK [i]->SDP.YB,
			BK [i]->Attenuation,
			BK [i]->Sigma, DBK [i]);
#endif
		DBK [i] = exp (-((DBK [i] / A) * EAF));
		trc ("in d->w %g [EAF=%g]", DBK [i], EAF);
	}

	W = A = S = D = 0.0;
	for (i = 0; i < NBK; i++) {
		// Stage2: convert weight di to ti = di * Prod (1 - dj) (j!=i);
		// this makes sure that 1) if there is (one) di == 1, all ti's
		// except that one will be zero, 2) di closer to one are
		// further amplified (will have to check it empirically)
		T = DBK [i];
		for (j = 0; j < NBK; j++)
			if (j != i)
				T *= (1.0 - DBK [j]);
		// Sum of weights for normalization
		W += T;
		A += (BK [i] -> Attenuation) * T;
		S += (BK [i] -> Sigma) * T;
		D += vlength (BK [i] -> SDP) * T;
		trc ("in[%1d] no: W=%g|%g, A=%g|%g, S=%g|%g, D=%g|%g",
			i, T, W, BK[i]->Attenuation * T, A,
			BK[i]->Sigma * T, S,
			vlength (BK[i]->SDP), D);
	}

			// Averages:
	A /= W;		// 	- attenuation
	S /= W;		// 	- sigma
	D /= W;		// 	- vector length

	trc ("in av: A=%g, S=%g, D=%g", A, S, D);

	DI = (dict_item_t*) malloc (sizeof (dict_item_t));
	assert (DI != NULL, "RFSampled: out of memory for dictionary items");

	// Normalize
	DI->SMP.SDP = sdp;
	DI->SMP.Sigma = (float) S;

	// Adjust for distance difference, use dB values
	DI->SMP.Attenuation = A + ATTB->setvalue (vlength (sdp), NO) -
		ATTB->setvalue (D, NO);

	trc ("in ad: ATT=%g = A + AS=%g - AD=%g", DI->SMP.Attenuation,
		ATTB->setvalue (vlength (sdp), NO), ATTB->setvalue (D, NO));

	return DI;
}

// ============================================================================

double RFSampled::RFC_att (const SLEntry *xp, double d, Transceiver *src) {

	double res;
	sdpair_t SDP;
	unsigned short pow, cha;

	// Channel crosstalk
	res = Channels->ifactor (cha = tagToCh (xp->Tag),
		tagToCh (TheTransceiver->getTag ()));
	trc ("RFC_att (ct) = %g [%08x %08x]", res, xp->Tag,
		TheTransceiver->getTag ());

	if (res == 0.0)
		// No need to worry
		return res;

#if ZZ_R3D
	src->getRawLocation (SDP.XA, SDP.YA, SDP.ZA);
	TheTransceiver->getRawLocation (SDP.XB, SDP.YB, SDP.ZB);
	trc ("RFC_att (sd) = <%1d,%1d,%1d> - <%1d,%1d,%1d>",
		SDP.XA, SDP.YA, SDP.ZA,
		SDP.XB, SDP.YB, ADP.ZB);
#else
	src->getRawLocation (SDP.XA, SDP.YA);
	TheTransceiver->getRawLocation (SDP.XB, SDP.YB);
	trc ("RFC_att (sd) = <%1d,%1d> - <%1d,%1d>",
		SDP.XA, SDP.YA,
		SDP.XB, SDP.YB);
#endif

	res = res * xp->Level * attenuate (SDP);

	if (gain)
		res *= gain (src, TheTransceiver);

	trc ("RFC_att (sl) = [%g,%g,%d,%d] -> %g [%g dBm]",
		xp->Level, d, src->getSID (), TheTransceiver->getSID (), res,
			linTodB (res));
	return res;
}

Boolean RFSampled::RFC_act (double sl, const SLEntry *sn) {

/*
 * Note: sn (receiver sensitivity) is a linear multiplier for the
 * received signal. I.e., 0dB means "standard sensitivity" of 1.
 */
	trc ("RFC_act (st) = %g [%g %g] -> %c", sl, sn->Level, BThrs,
		sl * sn->Level > BThrs ? 'Y' : 'N');
	return sl * sn->Level > BThrs;
}

double RFSampled::RFC_cut (double xp, double rp) {

/*
 * The cut-off distance d is the minimum distance at which the attenuated
 * signal (without the Gaussian component) is considered irrelevant
 */
	double d;

	if (COSL == 0.0)
		return Distance_inf;

	d = ATTB->getvalue (COSL / (xp * rp));
	trc ("RFC_cut (ds) = %g [%g %g] (%g)", d, xp, rp, COSL);
	return d;
}

Long RFSampled::RFC_erb (RATE tr, const SLEntry *sl, const SLEntry *rs,
							double ir, Long nb) {
/*
 * Number of error bits in the run of nb bits.
 */
	Long res;

	if (sl->Tag != rs->Tag) {
		trc ("RFC_erb (nb) = %1d [%08x != %08x]", nb, sl->Tag, rs->Tag);
		// Different channels or rates, nothing is receivable
		return nb;
	}

	// We account for the rate. Both the receieved signal level and the
	// interference (not the background noise) are boosted by the receiver
	// sensitivity. But the received signal level is additionally boosted
	// by the rate-specific factor.

	trc ("RFC_erb (ir) = %g %g %g", ir, rs->Level, BNoise);
	ir = ir * rs->Level + BNoise;

	trc ("RFC_erb (sl) = [%08x %08x] %g %g", sl->Tag, rs->Tag,
		sl->Level, rateBoost (sl->Tag));
	res = (ir == 0.0) ? 0 : lRndBinomial (ber ((sl->Level * rs->Level *
		rateBoost (sl->Tag)) / ir), nb);

	trc ("RFC_erb (nb) = %1d/%1d", res, nb);
	return res;
}

Long RFSampled::RFC_erd (RATE tr, const SLEntry *sl, const SLEntry *rs,
	double ir, Long nb) {
/*
 * This determines the expected number of bits until the first error bit. We
 * assume (somewhat simplistically) that this event will trigger an illegal
 * symbol and thus abort the reception. Note: if this isn't accurate, you can
 * always adjust the BER to match the real-life behavior.
 */
	double er;
	Long res;

	if (sl->Tag != rs->Tag) {
		// Nothing is receivable
		trc ("RFC_erd (nb) = 0 [%08x != %08x]", sl->Tag, rs->Tag);
		return 0;
	}

	trc ("RFC_erd (ir) = %g %g %g", ir, rs->Level, BNoise);
	ir = ir * rs->Level + BNoise;

	if (ir == 0.0)
		return MAX_Long;

	er = ber ((sl->Level * rs->Level * rateBoost (sl->Tag)) / ir);
	trc ("RFC_erd (be) = [%08x %08x] %g -> %g", sl->Tag, rs->Tag,
		rateBoost (sl->Tag), er);

	er = dRndPoisson (1.0/er);

	res = (er > (double) MAX_Long) ? MAX_Long : (Long) er;
	trc ("RFC_erd (nb) = %1d/%1d", res, nb);
	return res;
}

Boolean RFSampled::RFC_bot (RATE r, const SLEntry *sl, const SLEntry *sn,
	const IHist *h) {
/*
 * BOT assessment, i.e., deciding whether we have tuned to the beginning of
 * packet. We assume that we are fine, if the received preamble has at least
 * so many correctly received bits at the end.
 */

	if (h->bits (r) < MinPr) {
		trc ("RFC_bot (st) = [%g] -> (short)", sl->Level);
		return NO;
	}

	if (error (r, sl, sn, h, -1, MinPr)) {
		trc ("RFC_bot (st) = [%g] -> perr", sl->Level);
		return NO;
	}

	trc ("RFC_bot (st) = [%g] -> OK", sl->Level);
	return YES;
}

Boolean RFSampled::RFC_eot (RATE r, const SLEntry *sl, const SLEntry *sn,
	const IHist *h) {
/*
 * EOT assessment. In this model, EOT is always fine. This is because we are
 * going to abort a reception in progress on the first error bit. Thus, if
 * EOT is ever reached, it is bound to be OK.
 */
	trc ("RFC_eot (st) = %1d", TheTransceiver->isFollowed (ThePacket));
	return TheTransceiver->isFollowed (ThePacket);
}

#endif
