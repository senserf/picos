#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "max30102.h"

#define	SAMPFREQ	25		// Sampling frequency
#define	NSAMPLES	(SAMPFREQ * 4)
#define	NAVERAGE	4		// Adjacent samples to average

#define	MIN_PEAK_THRESHOLD	30
#define	MAX_PEAK_THRESHOLD	60
#define	MIN_PEAK_DISTANCE	4
#define	MAX_NPEAKS		15

#define	ZERO_PEAK_LAG		(-1)

#define	MAX_RATIOS		5

static const byte spo2_table [184-3] = {
	// I have removed the first three entries, because we start at
	// index == 3
	                96,  96,  96,  97,  97,  97,  97,
	 97,  98,  98,  98,  98,  98,  99,  99,  99,  99, 
         99,  99,  99,  99, 100, 100, 100, 100, 100, 100,
	100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 
        100, 100, 100, 100,  99,  99,  99,  99,  99,  99,
	 99,  99,  98,  98,  98,  98,  98,  98,  97,  97, 
         97,  97,  96,  96,  96,  96,  95,  95,  95,  94,
	 94,  94,  93,  93,  93,  92,  92,  92,  91,  91, 
         90,  90,  89,  89,  89,  88,  88,  87,  87,  86,
	 86,  85,  85,  84,  84,  83,  82,  82,  81,  81, 
         80,  80,  79,  78,  78,  77,  76,  76,  75,  74,
	 74,  73,  72,  72,  71,  70,  69,  69,  68,  67, 
         66,  66,  65,  64,  63,  62,  62,  61,  60,  59,
	 58,  57,  56,  56,  55,  54,  53,  52,  51,  50, 
         49,  48,  47,  46,  45,  44,  43,  42,  41,  40,
	 39,  38,  37,  36,  35,  34,  33,  31,  30,  29, 
         28,  27,  26,  25,  23,  22,  21,  20,  19,  17,
	 16,  15,  14,  12,  11,  10,   9,   7,   6,   5, 
          3,   2,   1,   0 } ;

// ============================================================================

static  lint an_x [NSAMPLES];	// Need a copy of ir

// ============================================================================

max30102_sample_t	ir_buff [NSAMPLES], re_buff [NSAMPLES],
			s_min, s_max;

// ============================================================================

static void cupdate (word hr, word sp) {

	diag ("HR = %u, SP = %u", hr, sp);
}

static void calculate () {

	lint tha, thb, la, lb;
	lint ratios [MAX_RATIOS];
	int i, j, w;
	word hrate, spo2;
	byte peaks [MAX_NPEAKS];
	byte npeaks, nratios;
	byte b, ba, bb;

	for (tha = 0, i = 0; i < NSAMPLES; i++) 
		tha += ir_buff [i];

	tha /= NSAMPLES;

	// Remove mean and invert the signal into signed (check if we
	// actually have to store it as lint)

	for (i = 0; i < NSAMPLES; i++)
		an_x [i] = tha - (lint)ir_buff [i];

	for (tha = 0, i = 0; i < NSAMPLES - NAVERAGE; i++)
		// Average 4 consecutive samples and calculate threshold
		tha += (an_x [i] = (an_x [i  ] + an_x [i+1] +
			     	    an_x [i+2] + an_x [i+3]   ) / 4);
	while (i < NSAMPLES)
		tha += an_x [i++];

	if ((tha /= NSAMPLES) < MIN_PEAK_THRESHOLD)
		tha = MIN_PEAK_THRESHOLD;
	else if (tha > MAX_PEAK_THRESHOLD)
		tha = MAX_PEAK_THRESHOLD;

	// ====================================================================
	// Find the peaks =====================================================
	// ====================================================================

	npeaks = 0; i = 1;

	while (i < NSAMPLES - 1) {

		if (an_x [i] > tha && an_x [i] > an_x [i-1]) {
			// Start a new peak of width 1
			w = 1;
			while (i + w < NSAMPLES && an_x [i] == an_x [i + w])
			// Extend the flat part
			w++;
			if (an_x [i] > an_x [i + w] && npeaks < MAX_NPEAKS) {
				// End of peak, add if room; flat peak is
				// marked by left edge of the plateau
				peaks [npeaks++] = (byte) i;
				// Start next search
				i += w + 1;
				continue;
			}
			// Keep going with current peak
			i += w;
			continue;
		}

		i++;
	}

	// ====================================================================	
	// Order peaks from large to small ====================================
	// ====================================================================	

	for (i = 1; i < npeaks; i++) {
		b = peaks [i];
		for (j = i; j > 0 && an_x [b] > an_x [peaks [j - 1]]; j--)
			peaks [j] = peaks [j - 1];
		peaks [j] = b;
	}

	// ====================================================================
	// Remove close peaks =================================================
	// ====================================================================

	// I think this algorithm is stupid; they want to make sure that when
	// close peaks are eliminated the largest one wins, but this can be
	// done in a single pass through the unsorted peaks, can't it? Later!
	for (i = -1; i < npeaks; i++) {
		b = npeaks;
		npeaks = (byte)(i + 1);
		for (j = i + 1; j < b; j++) {
			// ZERO_PEAK_LAG is the index of the nonexistent peak
			// preceding the first peak (can't we carry it over?)
			w = peaks [j] - (i < 0 ? ZERO_PEAK_LAG : peaks [i]);
			if (w > MIN_PEAK_DISTANCE || w < -MIN_PEAK_DISTANCE)
				// Keep this peak
				peaks [npeaks++] = peaks [j];
		}
	}

	// ====================================================================
	// Re-sort the peaks back to ascending order of indices ===============
	// ====================================================================

	for (i = 1; i < npeaks; i++) {
		b = peaks [i];
		for (j = i; j > 0 && b < peaks [j - 1]; j--)
			peaks [j] = peaks [j - 1];
		peaks [j] = b;
	}

	// ====================================================================

	hrate = 0;
	spo2 = 0;

	// ====================================================================
	// Calculate the rate =================================================
	// ====================================================================

	if (npeaks < 2)
		// No way
		goto Done;

	for (w = 0, i = 1; i < npeaks; i++)
		w += (peaks [i] - peaks [i - 1]);

	hrate = (SAMPFREQ * 60 * (npeaks - 1)) / w;

	// ====================================================================
	// SPO2 ===============================================================
	// ====================================================================

	nratios = 0;

	for (i = 0; i < npeaks-1; i++) {

		// The original values (that we are working on here) are
		// nonnegative
		la = -1;	// Max ir
		lb = -1;	// Max re

		for (j = peaks [i]; j < peaks [i + 1]; j++) {
			if (ir_buff [j] > la) {
				// Max and its index
				la = ir_buff [j];
				ba = (byte) j;
			}
			if (re_buff [j] > lb) {
				// Max and its index
				lb = re_buff [j];
				bb = (byte) j;
			}
		}

		thb = (re_buff [ peaks [i + 1] ] -
		       re_buff [ peaks [i    ] ] )
				* (bb - peaks [i]);

		thb = re_buff [ peaks [i] ] + thb / (peaks [i + 1] - peaks [i]);

		thb = re_buff [bb] - thb;

		tha = (ir_buff [ peaks [i + 1] ] -
		       ir_buff [ peaks [i    ] ] )
				* (ba - peaks [i]);

		tha = ir_buff [ peaks [i] ] + tha / (peaks [i + 1] - peaks [i]);

		// Is it really "bb" rather than "ba"? !!!!!
		tha = ir_buff [bb] - tha;

		thb = (thb * la) >> 7;
		tha = (tha * lb) >> 7;

		if (tha > 0 && nratios < MAX_RATIOS && thb != 0)
			ratios [nratios++] = (thb * 100) / tha;
	}

	if (nratios == 0)
		goto Done;

	// Sort in ascending order

	for (i = 1; i < nratios; i++) {
		la = ratios [i];
		for (j = i; j > 0 && la < ratios [j - 1]; j--)
			ratios [j] = ratios [j - 1];
		ratios [j] = la;
	}

	if ((i = nratios / 2) > 1)
		tha = (ratios [i - 1] + ratios [i]) / 2;
	else
		tha = ratios [i];

	if (tha > 2 && tha < 184)
		spo2 = spo2_table [tha - 3];
Done:
	cupdate (hrate, spo2);
}

#if 0
// ===
{
	lint min, max;
	min =  1000000;
	max = -1000000;
	for (i = 0; i < NSAMPLES; i++) {
		if (min > an_x [i])
			min = an_x [i];
		if (max < an_x [i])
			max = an_x [i];
	}
	diag ("TH: %ld %ld %ld [%lu %lu] <%u %u>", min, max, tha, s_min, s_max,
		npeaks, hrate);
}
// ===
#endif


// ============================================================================

fsm collect {

	int sindex;

	state COLL_INIT:

		// Read the first 100 samples and determine the signal range
		sindex = 0;
		s_min = MAX_SAMPLE;
		s_max = 0;

	state COLL_INIT_READ:

		max30102_read_sample (COLL_INIT_READ,
			re_buff + sindex, ir_buff + sindex);

		if (re_buff [sindex] > s_max)
			s_max = re_buff [sindex];
		if (re_buff [sindex] < s_min)
			s_min = re_buff [sindex];

		if (++sindex < NSAMPLES)
			sameas COLL_INIT_READ;

		// Fall through to calculate heart rate and SPO2 after the
		// first 100 samples

	state COLL_MAIN_LOOP:

		int i;

		calculate ();

		s_min = MAX_SAMPLE;
		s_max = 0;

		// Shift out the first 25 samples
		for (i = 0; i < NSAMPLES - SAMPFREQ; i++) {
			re_buff [i] = re_buff [i + SAMPFREQ];
			ir_buff [i] = ir_buff [i + SAMPFREQ];
			if (s_min > re_buff [i])
				s_min = re_buff [i];
			if (s_max < re_buff [i])
				s_max = re_buff [i];
		}

		// Read this many new samples
		sindex = NSAMPLES - SAMPFREQ;

	state COLL_MAIN_LOOP_READ:

		max30102_read_sample (COLL_MAIN_LOOP_READ,
			re_buff + sindex, ir_buff + sindex);

		if (re_buff [sindex] > s_max)
			s_max = re_buff [sindex];
		if (re_buff [sindex] < s_min)
			s_min = re_buff [sindex];

		// min and max was needed for LED, let's keep it for a while

		if (++sindex < NSAMPLES)
			sameas COLL_MAIN_LOOP_READ;
		else
			sameas COLL_MAIN_LOOP;
}

fsm root {

	word par, val;

	state MENU:

		ser_out (MENU,
			"Commands:\r\n"
			"  o [0|1]\r\n"
			"  r nx\r\n"
			"  w nx vx\r\n"
			"  c [0|1]\r\n"
		);

	state INPUT:

		char cmd [4];

		ser_in (INPUT, cmd, 4);
		if (cmd [0] == 'o' || cmd [0] == 'c') {
			par = 1;
			scan (cmd + 1, "%u", &par);
			if (cmd [0] == 'o') {
				if (par)
					max30102_start ();
				else
					max30102_stop ();
			} else {
				if (par) {
					if (!running (collect))
						runfsm collect;
				} else {
					killall (collect);
				}
			}

			proceed SAYOK;

		} else if (cmd [0] == 'r') {
			// Register read
			par = WNONE;
			scan (cmd + 1, "%x", &par);
			if (par <= 0xff) {
				val = max30102_rreg ((byte)par);
				proceed OUTREG;
			}
		} else if (cmd [0] == 'w') {
			// Register write
			par = WNONE;
			val = 0;
			scan (cmd + 1, "%x %x", &par, &val);
			if (par <= 0xfd) {
				max30102_wreg ((byte)par, (byte)val);
				proceed OUTREG;
			}
		}

	state BAD_INPUT:

		ser_out (BAD_INPUT, "Illegal input\r\n");
		proceed MENU;

	state SAYOK:

		ser_out (SAYOK, "OK\r\n");
		proceed INPUT;

	state OUTREG:

		ser_outf (OUTREG, "Reg [%x] = %x\r\n", par, val);
		proceed INPUT;
}

