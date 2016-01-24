#ifndef	__wchansd_h__
#define	__wchansd_h__

// Sample-driven channel model

#include "wchan.h"

#define	RFSMPL_HASHSIZE		8192
#define	RFSMPL_HASHMASK		(RFSMPL_HASHSIZE - 1)
#define	RFSMPL_MAXK		32

// Indexes into the XVMapper table
#define	XVMAP_ATT		4
#define	XVMAP_SIGMA		5

// ============================================================================

typedef	struct {

	DISTANCE XA, YA
#if ZZ_R3D
		   , ZA
#endif
	       , XB, YB
#if ZZ_R3D
		   , ZB
#endif
			;
} sdpair_t;

typedef struct {
//
// A merged sample stored in the runtime database
//
	sdpair_t	SDP;		// S-D pair
	float		Attenuation,	// in dB, derived from RSS and power
			Sigma;		// Standard deviation of Attenuation
} rss_sample_t;

typedef struct {
//
// This is a dictionary item stored in the hash table
//
	rss_sample_t	SMP;
	void		*Rehash;

} dict_item_t;

// ============================================================================

rfchannel RFSampled : RadioChannel {
//
// This is a sample-driven channel model. I will write some description here
// (or maybe elsewhere) as soon as things have clarified
//
	double	BThrs,	// Channel busy signal threshold
	      	COSL,	// Cut-off signal level
		Sigma,	// Default lognormal random component of attenuation
		EAF;	// Exponential averaging factor

	Long  	MinPr;	// Minimum number of preamble bits for reception

	int		K;	// Samples to average

	DVMapper	*ATTB,	// Attenuation table
			*SIGMA;	// Sigma table

			// Optional gain function (e.g., for directional ant.)
	double 	(*gain) (Transceiver*, Transceiver*);

	Boolean	Symmetric;

	// Assessment methods
	double  RFC_att (const SLEntry*, double, Transceiver*);
	Boolean RFC_act (double, const SLEntry*);
	Boolean RFC_bot (RATE, const SLEntry*, const SLEntry*, const IHist*);
	Boolean RFC_eot (RATE, const SLEntry*, const SLEntry*, const IHist*);
	double  RFC_cut (double, double);
	Long    RFC_erb (RATE, const SLEntry*, const SLEntry*, double, Long);
	Long    RFC_erd (RATE, const SLEntry*, const SLEntry*, double, Long);

	dict_item_t	**HTable;
	rss_sample_t	*Samples;
	Long		NSamples;

	inline double sigma (double d) {
		return (SIGMA == NULL) ? Sigma : SIGMA->setvalue (d);
	};

	inline Long hash (sdpair_t &sdp) {
		if (Symmetric)
			// Make sure we do not differentiate between
			// directions
			return	(
			    (sdp.XA ^ sdp.XB) + ((sdp.YA ^ sdp.YB) >> 1)
#if ZZ_R3D
					      + ((sdp.ZA ^ sdp.ZB) << 1)
#endif
			    	)
					& RFSMPL_HASHMASK;
		else
			// Make sure we do differentiate between directions
			return	(
			     sdp.XA + (sdp.XB >> 1) + (sdp.YA >> 2)
					+ (sdp.YB >> 3)
#if ZZ_R3D
					+ (sdp.ZA << 1) + (sdp.ZB << 2)
#endif
				)
					& RFSMPL_HASHMASK;
	};

	inline Boolean sdpcmp (sdpair_t &a, sdpair_t &b) {

		if (	a.XA == b.XA
		     && a.YA == b.YA
		     && a.XB == b.XB
		     && a.YB == b.YB
#if ZZ_R3D
		     && a.ZA == b.ZA
		     && a.ZB == b.ZB
#endif
				       )
						return YES;
		if (	Symmetric
		     && a.XB == b.XA
		     && a.YB == b.YA
		     && a.XA == b.XB
		     && a.YA == b.YB
#if ZZ_R3D
		     && a.ZB == b.ZA
		     && a.ZA == b.ZB
#endif
					)
						return YES;

		return NO;
	};

	inline double sdpdist (sdpair_t &sa, sdpair_t &sb) {
	//
	// Calculates the "distance" between SD pairs expressed as the sum
	// of the Euclidean distances between the points
	//
		double d0, d1, d2, d3, D0, D1;
#if ZZ_R3D
		double d4, d5;
#endif
		d0 = (double)(sa.XA - sb.XA);
		d1 = (double)(sa.YA - sb.YA);
		d2 = (double)(sa.XB - sb.XB);
		d3 = (double)(sa.YB - sb.YB);
#if ZZ_R3D
		d4 = (double)(sa.ZA - sb.ZA);
		d5 = (double)(sa.ZB - sb.ZB);
#endif
		D0 = sqrt (
				(d0 * d0) + (d1 * d1)
#if ZZ_R3D
			      + (d4 * d4)
#endif
						) +
		     sqrt (
				(d2 * d2) + (d3 * d3)
#if ZZ_R3D
			      + (d5 * d5)
#endif
						);

		if (Symmetric) {
			// Try the other way around
			d0 = (double)(sa.XA - sb.XB);
			d1 = (double)(sa.YA - sb.YB);
			d2 = (double)(sa.XB - sb.XA);
			d3 = (double)(sa.YB - sb.YA);
#if ZZ_R3D
			d4 = (double)(sa.ZA - sb.ZB);
			d5 = (double)(sa.ZB - sb.ZA);
#endif
			D1 = sqrt (
					(d0 * d0) + (d1 * d1)
#if ZZ_R3D
				      + (d4 * d4)
#endif
							) +
			     sqrt (
					(d2 * d2) + (d3 * d3)
#if ZZ_R3D
				      + (d5 * d5)
#endif
							);

			if (D1 < D0)
				D0 = D1;
		}

		return D0;
	};

	void read_samples (const char*);

	double attenuate (sdpair_t&);

	dict_item_t *interpolate (sdpair_t&);

	void setup (
		Long,			// The number of transceivers
		const sir_to_ber_t*,	// SIR to BER conversion table
		int,			// Length of the conversion table
		int,			// The number of samples to average
		double,			// Exponential averaging factor
		double,			// Gaussian lognormal component
		double,			// Background noise (dBm)
		double,			// Channel busy signal threshold dBm
		double,			// Cut off signal level
		Long,			// Minimum received preamble length
		int,			// Bits per byte
		int,			// Packet frame (extra physical bits)
		IVMapper **ivcc, 	// Value converters
		const char *sfname,	// File with sampled RSSI data
		Boolean symm,		// Symmetric flag
		MXChannels *mxc = NULL,	// Channels
		double (*g) (Transceiver*, Transceiver*) = NULL
	);
};

#endif
