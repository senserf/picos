#include <ecog.h>
#include <ecog1.h>
#include "kernel.h"
#include "radio.h"

/* ============================================================================ */
/*                       PicOS                                                  */
/*                                                                              */
/* Radio driver                                                                 */
/*                                                                              */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002, 2003                 */
/*                                                                              */
/* Permission is hereby granted, free of charge, to any person obtaining a copy */
/* of this software and associated documentation files (the "Software"), to     */
/* deal in the Software without restriction, including without limitation the   */
/* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  */
/* sell copies of the Software, and to permit persons to whom the Software is   */
/* furnished to do so, subject to the following conditions:                     */
/*                                                                              */
/* The above copyright notice and this permission notice shall be included in   */
/* all copies or substantial portions of the Software.                          */
/*                                                                              */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS */
/* IN THE SOFTWARE.                                                             */
/*                                                                              */
/* ============================================================================ */

extern pcb_t     *zz_curr;

#if	RADIO_DRIVER

static void xcv_disable (void);

#if ! ECOG_SIM
static void xcv_restore (void);
#endif

/*
 * This one is needed by the interrupt helper, so it is better to keep it
 * handy.
 */
word	zzz_radiostat = 0;

static	word	*drvdata;

#if     RADIO_INTERRUPTS

pcb_t *zzr_xwait;
int zzr_xstate, zzz_last_sense;
/*
* These functions are somewhat clumsy helpers for implementing
* semi-interrupts for dumb radio boards. Now it turns out that the
* radio board that is likely to become "official" (XEMICS 1202) actually
* makes true interrupts possible, so perhaps I will revise this code
* when the situation becomes clear. For now, the same interface is also
* used by the XEMICS (which really appears clumsy).
*/

bool rcvwait (word state) {

	int j = nevents (zz_curr);

	if (j == MAX_EVENTS_PER_TASK)
		syserror (ENEVENTS, "rcvwait");

		setestatus (zz_curr->Events [j], ETYPE_SYSTEM, state);
		zz_curr->Events [j] . Event = (word) &zzr_xwait;

		cli_radio;
		if (zzr_xwait != NULL) {
			sti_radio;
			return NO;
		}
		zzr_xwait = zz_curr;
		zzr_xstate = state;
		incwait (zz_curr);
		sti_radio;

		return YES;
}

void rcvcancel (void) {
	zzr_xwait = NULL;
}

int rcvlast (void) {
	return zzz_last_sense;
}

#endif

static lword w_chk (const char *b, int nb) {
/*
 * Calculate checksum
 */
	lword chs = 0;
	lword *wa = (lword*)b;

	nb >>= 2;

	while (nb--)
		chs += *wa++;

	return chs;
}

#if	RADIO_TYPE == RADIO_XEMICS
/* ================================================= */
/* XEMICS is completely different from the other two */
/* ================================================= */

//+++ "gpioirq.c"

#define	rd_bcount	(drvdata [0])
#define	rd_prlen	(drvdata [1])
#define	rd_flags	(drvdata [2])
#define	xe_regs		((byte*)(drvdata + 3))
#define	xe_regsw	(drvdata + 3)

#define	xmt_enable_warm()	do { cli_radio; xmt_start (); } while (0)
#define	rcv_enable_warm()	rcv_start ()

typedef	struct {

	word rate;
	word cntr;
} xe_rate_t;

static const xe_rate_t xe_rates [] =   {

				{ (word)  4800, 250 },
				{ (word)  9600, 125 },
				{ (word) 19200,  62 },
				{ (word) 38400,  31 },
				{     0,   0 }

					};
static void xmt_inicntr (void) {
/*
 * Initialize the baud counter for the transmitter
 */
	rg.ssm.rst_clr = SSM_RST_CLR_LOW_PLL_DIV_CHN_MASK;
	rg.ssm.clk_en = SSM_CLK_EN_LOW_PLL_MASK;
	/*
	 * We are using CNT1 driven from low-ref PLL. The low-ref crystal
	 * runs at 32kHz with the the PLL multiplier of 150. This gives us
	 * the running rate of 4,800,000 Hz. The lowest divider (0) is 2^2,
	 * which brings the rate down to 1,200,000 Hz. To obtain, say, the
 	 * bit rate of 19.2kbps, we divide that by 19200, which yields
	 * 62.5. So with 62 we get less than 1% error, which should be
	 * fine because XEMICS accepts up to 5% deviation.
	 */
	rg.ssm.rst_clr = SSM_RST_CLR_CNT1_MASK;		/* Out of reset */
	fd.ssm.tap_sel2.cnt1 = 0;			/* The divider (0-15) */
	fd.ssm.div_sel.cnt1 = 1;			/* PLL vs REF */

	rg.ssm.clk_en = SSM_CLK_EN_CNT1_MASK;		/* Enable */

	rg.tim.cnt1_cfg = 0;				/* Timer vs counter */

	rg.tim.int_dis1 =				/* No interrupts */
		TIM_INT_DIS1_CNT1_EXP_MASK |
		TIM_INT_DIS1_CNT1_MATCH_MASK;
}

#define xmt_clearcntr	rg.tim.int_clr1 = TIM_INT_CLR1_CNT1_EXP_MASK
#define	xmt_expcntr	((rg.tim.int_sts1 & TIM_INT_STS1_CNT1_EXP_MASK) != 0)
#define	xmt_runcntr	((rg.tim.int_sts1 & TIM_INT_STS1_CNT1_EXP_MASK) == 0)


#define _ECOG_SIM 0

#if ! ECOG_SIM

#define	xmt_waitcntr	do { do { } while (xmt_runcntr); xmt_clearcntr; } \
				while (0)
#else //Tx baud modelling in ecogsim seems to slow the system down by too much

#define	xmt_waitcntr	do { } \
				while (0)
#endif

static void xmt_startcntr (void) {
/*
 * Start the baud counter
 */
	rg.tim.ctrl_en = 				/* En + auto reload */
		TIM_CTRL_EN_CNT1_CNT_MASK |
		TIM_CTRL_EN_CNT1_AUTO_RE_LD_MASK;

	rg.tim.cnt1_ld = rd_bcount;			/* Reload value */
	fd.tim.cmd.cnt1_ld = 1;				/* Load the RLD value */

	/* Reset the expired status */
	xmt_clearcntr;
}

static void xmt_stopcntr (void) {
/*
 * Stop/disable the baud counter
 */
	rg.tim.ctrl_dis =
		TIM_CTRL_DIS_CNT1_CNT_MASK |
		TIM_CTRL_DIS_CNT1_AUTO_RE_LD_MASK;
}

static void xcv_coldstart (void) {
/*
 * Start the chip after sleep (stage 1: sleep -> standby, oscillator)
 */
	xem_enlow;
	xem_clrmode2;
	xem_clrmode1;
	xem_setmode0;
	udelay (10);
	xem_enhigh;
	xem_dismodes;
	udelay (3072);
}

static void xmt_warmup (void) {
/*
 * Transmitter warmup (stage 2: standby -> oscillator + syntesizer running)
 */
	xem_enlow;
	xem_setmode2;
	xem_setmode1;
	xem_clrmode0;
	udelay (10);
	xem_enhigh;
	xem_dismodes;
	udelay (300);
}

static void xmt_start (void) {
/*
 * Transmitter running (stage 3)
 */
	xem_enlow;
	xem_setmode2;
	xem_setmode1;
	xem_setmode0;
	xem_setant_xmt;
	xem_clrant_rcv;
	udelay (10);
	xem_enhigh;
	xem_dismodes;
	udelay (200);
	xmt_startcntr ();
}

static void rcv_warmup (void) {
/*
 * Receiver coldstart, stages 2 and 3
 */
	xem_enlow;
	xem_clrmode2;
	xem_setmode1;
	xem_clrmode0;		/* Oscillator + baseband running */
	udelay (10);
	xem_enhigh;
	xem_dismodes;
	udelay (300);

	xem_enlow;
	xem_clrmode2;
	xem_setmode0;
	xem_setmode1;		/* Oscillator + baseband + synthesizer */
	udelay (10);
	xem_enhigh;
	xem_dismodes;
	udelay (300);
}

static void rcv_start (void) {
/*
 * Receiver start (stage4: Receiver running)
 */
	xmt_stopcntr ();
	xem_enlow;
	xem_setmode2;
	xem_clrmode1;
	xem_clrmode0;
	xem_setant_rcv;
	xem_clrant_xmt;
	udelay (10);
	xem_enhigh;
	xem_dismodes;
	udelay (700);
	sti_radio;
}

static void xmt_enable_cold (void) {
	cli_radio;
	xcv_coldstart ();
	xmt_warmup ();
	xmt_start ();
}

static void rcv_enable_cold (void) {
	xcv_coldstart ();
	rcv_warmup ();
	rcv_start ();
}

static void xcv_sleep (void) {
	xmt_stopcntr ();
	xem_enlow;
	xem_clrmode0;
	xem_clrmode1;
	xem_clrmode2;
	xem_clrant_rcv;
	xem_clrant_xmt;
	udelay (10);
	xem_enhigh;
	xem_dismodes;
}

static void xe_sio (void) {
/*
 * Initialize for serial i/o
 */
	xcv_disable ();
	xem_setsi;
	xem_setsck;
	xem_enlow;
}

static void xe_eio (void) {
/*
 * Cleans up after serial i/io
 */
	xcv_disable ();
#if ! ECOG_SIM
	xcv_restore ();
#endif
}

static void xe_wrs (byte addr, byte stuff) {
/*
 * Writes a byte through the serial interface
 */
	int i;

	udelay (128);

	xem_wser0;	/* Start bit */
	xem_wser0;	/* Write command */

	/* The address */
	for (i = 4; i >= 0; i--) {
		if (((addr >> i) & 1) != 0)
			xem_wser1;
		else
			xem_wser0;
	}

	/* The stuff */
	for (i = 7; i >= 0; i--) {
		if (((1 << i) & stuff) != 0)
			xem_wser1;
		else
			xem_wser0;
	}

	for (i = 0; i < 4; i++) {
		xem_wser1;	/* Stop 1 */
		xem_wser1;	/* Stop 2 */
	}

	xem_setsck;
	udelay (128);
}

static byte xe_res (byte addr) {
/*
 * Reads a byte through the serial interface
 */
	int i;
	byte b;

	udelay (128);

	xem_wser0;	/* Start bit */
	xem_wser1;	/* Read command */

	/* Send the address */
	for (i = 4; i >= 0; i--) {
		if (((addr >> i) & 1) != 0)
			xem_wser1;
		else
			xem_wser0;
	}

	b = 0;

	for (i = 7; i >= 0; i--)
		xem_rser (b, i);

	xem_setsck; xem_pulse; xem_clrsck; xem_pulse; xem_setsck;

	udelay (128);

	return b;
}

static void ini_params (void) {
/*
 * Initialize device registers
 */
	int i;
	byte reg;

	xe_sio ();

	/*
	 * Start by writing the registers
	 */
	for (i = 0; i < 12; i++)
		xe_wrs (i, xe_regs [i]);

	xe_eio ();

	/* Just in case */
	udelay (1024);

	/* Read them back for verification */
	for (reg = 0, i = 0; i < 12; i++) {
		if (i == 5)
			continue;
		xe_sio ();
		reg = xe_res (i);
		xe_eio ();
		if (i == 0 && reg == 0) {
			/*
			 * This one should never be zero because we always
			 * operate with the bit synchronizer on. If it is,
			 * it means that the SO line is not connected, which
			 * is OK (we can set those registers blindly). But
			 * then we set a flag (using the data register for
			 * that) to indicate that we cannot retrieve info
			 * from the chip.
			 */
			xe_regs [5] = 1;
			break;
		}
		if (reg != xe_regs [i]) {
			xe_regs [5] = 2;
			break;
		}
	}

	switch (xe_regs [5]) {

		case 0:

			diag ("XEMICS 1202 initialized: %x %x %x %x %x %x",
				xe_regsw [0],
				xe_regsw [1],
				xe_regsw [2],
				xe_regsw [3],
				xe_regsw [4],
				xe_regsw [5]);

			break;

		case 1:

			diag ("XEMICS 1202 initialized (SO line disabled)");
			break;

		default:

			diag ("XEMICS 1202 failed: reg %d, %x != %x",
				i, reg, xe_regs [i]);
	}

	/* Initialize (but don't start) the baud rate counter */
	xmt_inicntr ();
}

static void tx_pkt (const char *buf, int len) {

	word n;
	byte c;

	/* Insert the checksum */

	if ((rd_flags & RF_CHECK) != 0)
		((lword*)buf) [ (len >> 2) - 1 ] = - w_chk (buf, len - 4);

	if ((zzz_radiostat & 1) == 0)
		/* The action is void if we are switched off */
		return;

	/* Send the preamble */
	for (n = 0; n < rd_prlen; n++) {

		/* Wait for the nearest counter strobe */
		xmt_waitcntr;
		xmt (1);
		xmt_waitcntr;
		xmt (0);
	}

	/* Send the preamble trailer */
	xmt_waitcntr;
	xmt (1);
	xmt_waitcntr;
	/* Two ones */

#if ECOG_SIM
	xmt (1); // need it explicit for emulation/VM
#endif

	/*
	 * This version of coder is trivial. We force a flip on every fifth
	 * bit, which is ignored.
	 */

	for (n = 0; n < len; n++) {

		c = buf [n];

		xmt_waitcntr; xmt (c & 0x80);
		xmt_waitcntr; xmt (c & 0x40);
		xmt_waitcntr; xmt (c & 0x20);
		if  (c & 0x10) {
			xmt_waitcntr; xmt (1);
			xmt_waitcntr; xmt (0);
		} else {
			xmt_waitcntr; xmt (0);
			xmt_waitcntr; xmt (1);
		}

		xmt_waitcntr; xmt (c & 0x08);
		xmt_waitcntr; xmt (c & 0x04);
		xmt_waitcntr; xmt (c & 0x02);
		if  (c & 0x01) {
			xmt_waitcntr; xmt (1);
			xmt_waitcntr; xmt (0);
		} else {
			xmt_waitcntr; xmt (0);
			xmt_waitcntr; xmt (1);
		}
	}

	/* Close the packet (low signal for one millisecond) */

	xmt_waitcntr;
	xmt (0);
	udelay (1024);

#if ECOG_SIM
	for (n = 0; n < 10 ; n++) { // as per protocol for Rxing
	  xmt (0); // need it explicit for emulation/VM
	}
#endif

}

#if ! ECOG_SIM
#define	gbit(l)	do { \
		    timcnt = 4; \
		    do { if (timcnt == 0) goto l; } while ( xem_getdclk); \
		    timcnt = 4; \
		    do { if (timcnt == 0) goto l; } while (!xem_getdclk); \
		} while (0)

#else
#define	gbit(l)	do { \
    timcnt = 4; \
    do { if (timcnt == 0) goto l; } while (!xem_getdclk); \
    rg.io.gp12_15_out |= IO_GP12_15_OUT_SET14_MASK; /*removeone from FIFO*/ \
} while (0)
#endif

extern int zzz_last_sense;

static int rx_pkt (char *buf, int mpl) {

	word timcnt, lastbit;
	int bptr; byte c, ec;

	utimer (&timcnt, YES);

	gbit (Nop);
	lastbit = rcv;

	/* Skip the preamble */
	do {
		gbit (Erp);
		if (rcv == lastbit)
			break;
		lastbit = 1 - lastbit;
	} while (1);

	if (lastbit == 0)
		/* Must end with two ones */
		goto Erp;

	xem_setrcvi;

	/* Now we are ready to receive the packet */
	bptr = 0;
	ec = 0;
	do {
		c = 0;
		gbit (End); if (rcv) c |= 0x80;
		gbit (End); if (rcv) c |= 0x40;
		gbit (End); if (rcv) c |= 0x20;
		gbit (End); if ((lastbit = rcv) != 0) c |= 0x10;

		gbit (End);
		if (lastbit == rcv) {
			if (++ec >= 2)
				/*
				 * For an exit condition, we count the number
				 * of consecutive cases when the dummy flip
				 * bit is incorrect. As soon as we reach the
				 * limit (2), we stop receiving the packet.
				 */
				goto End;
		} else {
			ec = 0;
		}

		gbit (End); if (rcv) c |= 0x08;
		gbit (End); if (rcv) c |= 0x04;
		gbit (End); if (rcv) c |= 0x02;
		gbit (End); if ((lastbit = rcv) != 0) c |= 0x01;

		gbit (End);
		if (lastbit == rcv) {
			if (++ec >= 2)
				goto End;
		} else {
			ec = 0;
		}

		if (bptr >= mpl)
			break;

		buf [bptr++] = c;
	} while (1);

End:

	zzz_last_sense = 0;

	utimer (&timcnt, NO);
	xem_clrrcvi;

	if ((rd_flags & RF_CHECK) != 0) {

		if (bptr < 8) {
			return (-1);
		}

		if (bptr & 0x3)
			bptr &= ~0x3;

		if (w_chk (buf, bptr))
			return -1;
	}

	return bptr;
Nop:
	utimer (&timcnt, NO);
	xem_clrrcvi;

	return 0;

Erp:
	utimer (&timcnt, NO);
	xem_clrrcvi;

	return -1;
}

static int xe_rdpower (void) {
/*
 * Extracts the signal strength indicator
 */
	byte reg;

	if (xe_regs [5])
		/* Unavailable */
		return 1;

	xe_sio ();
	reg = xe_res (5);
	xe_eio ();

	return (int) ((reg >> 6) & 3);
}

static void xe_setpower (word pow) {
/*
 * Sets the transmission power level
 */
	if (pow > 3)
		pow = 3;

	xe_regs [0] = (byte) ((xe_regs [0] & ~0x3) | pow);

	xe_sio ();
	xe_wrs (0, xe_regs [0]);
	xe_eio ();
}

static void w_calibrate (word btm) {
/*
 * Change the transmission rate
 */
	int i;

	for (i = 0; ; i++) {
		if (xe_rates [i] . rate == btm)
			break;
		if (xe_rates [i] . rate == 0)
			syserror (EREQPAR, "xemics calibrate");
	}

	/* Modify the register */
	xe_regs [2] = (xe_regs [2] & ~0x3) | (byte) i;
	xmt_stopcntr ();
	rd_bcount = xe_rates [i] . cntr;
	/*
	 * This also temporarily disables the transmitter and the receiver (if
	 * any of them was active)
	 */
	xe_sio ();
	xe_wrs (2, xe_regs [2]);
	xe_eio ();
	/* Just in case */
	udelay (1024);
}

	/* ========================== */
#else	/* RADIO_TYPE == RADIO_XEMICS */
	/* ========================== */

/*
 * These ones will be calibrated. They have to reside in on chip RAM for
 * efficiency and stability. We have to access them from the assembly
 * language component.
 */
word 	zzz_rc_limit,
	zzz_rcl_thrshld [4];

/*
 * This is to contain some less critical attributes, so we put them into
 * SDRAM to save on memory.
 */
static word	*drvdata;

#define	rc_last		(drvdata [0])	// Initialized
#define	txl_factor	(drvdata [1])	// Calibrated
#define	rcl_msec	(drvdata [2])	// Calibrated
#define	rd_prtries	(drvdata [3])	// Initialized
#define	rd_prwait	(drvdata [4])	// Initialized
#define	rd_prlen	(drvdata [5])	// Initialized
#define	rd_flags	(drvdata [6])	// Initialized


#define _ECOG_SIM 0

#if ECOG_SIM==0

extern	word rc_ch (void);
extern	void rc_setlev (int);
//+++ "rasm.asm"

#else

word _rc_ch (void) {
  word duration=0;

  for (duration=0; (duration <4) && (rcv ==rc_last); duration++) {
    rg.io.gp12_15_out |= IO_GP12_15_OUT_SET14_MASK;  //removeone from FIFO

    if ( (duration ==3) && (rc_last== RCS_LOW) ) { //at end of packet

      rg.io.gp12_15_out |= IO_GP12_15_OUT_SET14_MASK;
      //skip over next bit, possibly a 0
      //(otherwise, if had 5 0's in a row, the timer isr would miss the leading 1 of the next packet)
    }
  }//for

#if _ECOG_SIM
  diag ("_rc_ch for rc_last %d returning %d and RCS_LOW is %d \n", rc_last, duration,RCS_LOW );
#endif

  return duration;
}
#define rc_ch  _rc_ch

void _rc_setlev (int i) { //always sets rc_last when called !
}
#define rc_setlev _rc_setlev

#endif //ECOG_SIM


static int rc_pr () {
/*
 * Wait for a preamble
 */
	word res, nc, ntries;

	rc_setlev (rc_last = RCS_LOW);

	/* Wait for high (should show up within rd_prwait milliseconds) */
	zzz_rc_limit = rcl_msec;
	for (nc = rd_prwait; nc; nc--) {
		res = rc_ch ();
		if (res < 4)
			goto Pre;
	}
	/* Failure */
	return 0;
Pre:
	zzz_rc_limit = zzz_rcl_thrshld [3];

	for (ntries = 0; ntries < rd_prtries; ntries++) {

		rc_setlev (rc_last = RCS_HIGH);

		/* Wait for high of duration 3 */
		res = rc_ch ();
		rc_setlev (rc_last = RCS_LOW);
		if (res < 3) {
			rc_ch ();
			continue;
		}

		/*
		 * We have a high of duration 3. Now we wait for (L1H3)*L1H2
		 */

		nc = 0;
		while (1) {
			/* The signal is low - waiting for high */
			if ((res = rc_ch ()) != 1) {
				/* Error - wait for high */
				rc_ch ();
				goto Cont;
			}
			rc_setlev (rc_last = RCS_HIGH);
			/* Waiting for low */
			if ((res = rc_ch ()) < 2) {
				/* Error */
				rc_setlev (rc_last = RCS_LOW);
				if (rc_ch () > 3) {
					/* No preamble */
					return -1;
				}
				goto Cont;
			}
			if (res == 2) {
				rc_setlev (rc_last = RCS_LOW);
				if (nc < RCL_MINPR) {
					/* Too short */
					if (rc_ch () > 3) {
						/* No preamble */
						return -1;
					}
					goto Cont;
				}
				/* Start receiving */
				return 1;
			}
			if (res > 3) {
				/* Error */
				goto Cont;
			}
			rc_setlev (rc_last = RCS_LOW);
			nc++;
			/* This will end sooner or later */
		}
	Cont: ;
	}
	return -1;
}

#define	b_add	do { \
			if (bitp == 0) { \
				bitp = 7; \
				bptr++; \
				if (bptr >= mpl) \
					goto Complete; \
				buf [bptr] = 0; \
			} else \
				bitp--; \
			if (rc_last ^ sinc) \
				buf [bptr] |= (1 << bitp); \
		} while (0)

static int rx_pkt (char *buf, int mpl) {
/*
 * Receive the packet
 */
	word bitp, bitc, sinc;
	int bptr;

	buf [0] = 0;
	bitc = 2;
	sinc = (XMS_HIGH == RCS_HIGH);

	if ((bptr = rc_pr ()) <= 0) {
		return bptr;
	}

	/* We have the preamble */

	bptr = 0;
	bitp = 8;

	do {
		switch (rc_ch ()) {
			case 1:
				rc_setlev (rc_last = 1 - rc_last);
				/* A single bit */
				if (bitc == 2) {
					bitc = 0;
					/* Reverse signal interpretation */
					sinc = 1-sinc;
					/* Ignore */
					continue;
				}
				bitc++;
				/* Add the single bit */
				b_add;
				continue;
			case 2:
				rc_setlev (rc_last = 1 - rc_last);
				/* Two bits */
				switch (bitc) {
					case 0:
						/* Both are valid */
						b_add;
						b_add;
						bitc = 2;
						continue;
					case 1:
						/* Only one is valid */
						b_add;
						bitc = 0;
						sinc = 1-sinc;
						continue;
					default:
						/* Only one is valid */
						sinc = 1-sinc;
						b_add;
						bitc = 1;
						continue;
				}
				continue;
			case 3:
				rc_setlev (rc_last = 1 - rc_last);
				/* Three bits */
				switch (bitc) {
					case 0:
						/* First two */
						b_add;
						b_add;
						sinc = 1-sinc;
						/* bitc is not changed */
						continue;
					case 1:
						/* First and last */
						b_add;
						sinc = 1-sinc;
						b_add;
						continue;
					default:
						/* Last two */
						sinc = 1-sinc;
						b_add;
						b_add;
						continue;
				}
			default:
				/* Quit */
				goto Complete;
		}
	} while (1);

Complete:

	if ((rd_flags & RF_CHECK) != 0) {
		if (bptr < 8 || bptr & 0x3) {
			bptr++;
			if (bptr < 8 || bptr & 0x3) {
				/* Malformed packet */
				return -1;
			}
		}

		/* Validate the checksum */
		if (w_chk (buf, bptr)) {
			/* This must be zero */
			return -1;
		}
	}

	return bptr;
}

static void tx_ch (int sig, word dur) {
/*
 * Send a signal of a given duration
 */
	rc_last = sig;
	xmt (rc_last);
	dur *= txl_factor;

#if ECOG_SIM==0
	while (dur--);
#else //continuous-to-discrete
	while (--dur) {
	  xmt (rc_last);
	}
#endif

}

static void tx_pr () {
/*
 * Transmit a packet preamble
 */
	int nc;

	for (nc = 0; nc < rd_prlen; nc++) {

		tx_ch (XMS_HIGH, 3);
		tx_ch (XMS_LOW, 1);

	}

	tx_ch (XMS_HIGH, 2);
}

static void tx_pkt (const char *buf, int len) {
/*
 * Transmit the packet
 */
	word bptr, bitp;

	if ((rd_flags & RF_CHECK) != 0)
		/* Insert the checksum */
		((lword*)buf) [ (len >> 2) - 1 ] = - w_chk (buf, len - 4);

	/* Send the preamble */
	tx_pr ();

	bptr = 0;
	bitp = 8;

	do {
		/* Even turn */

		if (bitp == 0) {
			if (++bptr >= len)
				/* Done */
				break;
			bitp = 6;
		} else
			bitp -= 2;

		/* Balance */
		tx_ch (1 - rc_last, 1);

		switch ((buf [bptr] >> bitp) & 0x3) {

			case 0: /* 11 */
				tx_ch (XMS_HIGH, 2);
				break;
			case 1: /* 10 */
				tx_ch (XMS_HIGH, 1);
				tx_ch (XMS_LOW, 1);
				break;
			case 2: /* 01 */
				tx_ch (XMS_LOW, 1);
				tx_ch (XMS_HIGH, 1);
				break;
			default:
				/* 00 */
				tx_ch (XMS_LOW, 2);
				break;
		}

		/* Odd turn */

		bitp -= 2;
		tx_ch (1 - rc_last, 1);

		switch ((buf [bptr] >> bitp) & 0x3) {

			case 0: /* 00 */
				tx_ch (XMS_LOW, 2);
				break;
			case 1: /* 01 */
				tx_ch (XMS_LOW, 1);
				tx_ch (XMS_HIGH, 1);
				break;
			case 2: /* 10 */
				tx_ch (XMS_HIGH, 1);
				tx_ch (XMS_LOW, 1);
				break;
			default:
				/* 11 */
				tx_ch (XMS_HIGH, 2);
				break;
		}
	} while (1);

	tx_ch (1 - rc_last, 1);		// Stop marker
	tx_ch (1 - rc_last, 1);		// Stop marker
	tx_ch (1 - rc_last, 1);		// Stop marker
	tx_ch (1 - rc_last, 1);		// Stop marker
	tx_ch (XMS_LOW, 4);		// LOW
}

#if ECOG_SIM
static void w_calibrate (word btm) {

  	txl_factor= 1;
	/*
	zzz_rcl_thrshld [0] =1;
	zzz_rcl_thrshld [1] =2;
	zzz_rcl_thrshld [2] =3;
	zzz_rcl_thrshld [3] =4;
	rcl_msec = zzz_rcl_thrshld [3];
	*/
}
#else

static void w_calibrate (word btm) {
/*
 * Calibrates the timing parameters
 */
	word dcount, n, min, max;
	int left;

	if (btm > 38400 || btm < 4800)
		syserror (EREQPAR, "radio calibrate");

	/* Convert rate to bit time */
	btm = (word) ((1024L * 1024L) / btm);

	/* Disable the device */
	xcv_disable ();

	dcount = 0;
	utimer (&dcount, YES);

	min = 0;
	max = 32767;

	rc_setlev (rc_last = RCS_LOW);

	zzz_rcl_thrshld [0] =
		zzz_rcl_thrshld [1] =
			zzz_rcl_thrshld [2] =
				zzz_rcl_thrshld [3] = 0;
	while (1) {
		/* Calibrate the received bit threshold */
		zzz_rc_limit = (max + min) / 2;
	Again:
		dcount = btm * 2;
		for (n = 0; n < 1024; n++) {
			if (dcount == 0)
				break;
			if (rc_ch () != 4)
				goto Again;
		}
		left = dcount;
		if (min >= max - 1)
			break;

		if (left > btm)
			/* Increase */
			min = zzz_rc_limit;
		else if (left < btm)
			max = zzz_rc_limit;
		else
			break;
	}

	zzz_rcl_thrshld [0] = zzz_rc_limit + zzz_rc_limit/2;
	zzz_rcl_thrshld [1] =
		zzz_rcl_thrshld [0] + zzz_rc_limit + zzz_rc_limit/8;
	zzz_rcl_thrshld [2] =
		zzz_rcl_thrshld [1] + zzz_rc_limit + zzz_rc_limit/10;
	zzz_rcl_thrshld [3] =
		zzz_rcl_thrshld [2] + zzz_rc_limit + zzz_rc_limit/5;
	rcl_msec = zzz_rc_limit * (1024 / btm);

	min = 0;
	max = 32767;

	while (1) {
		/* Calibrate the transmitter bit factor */
		txl_factor = (max + min) / 2;
		dcount = btm * 2;
		for (n = 0; n < 1024; n++) {
			if (dcount == 0)
				break;
			tx_ch (XMS_LOW, 1);
		}
		left = dcount;
		if (min >= max - 1)
			break;

		if (left > btm)
			/* Increase */
			min = txl_factor;
		else if (left < btm)
			max = txl_factor;
		else
			break;
	}

	utimer (&dcount, NO);

	/* Restore device status */
	xcv_restore ();
}

#endif //ECOG_SIM

	/* ========================== */
#endif	/* RADIO_TYPE == RADIO_XEMICS */
	/* ========================== */

static void xmt_enable (void) {

	if ((zzz_radiostat & 1) != 0)
		/* Enabled already */
		return;

	if (!zzz_radiostat)
		/* Sleeping */
		xmt_enable_cold ();
	else
		/* Switch from receive */
		xmt_enable_warm ();

	zzz_radiostat |= 1;
}

static void xmt_disable (void) {

	if ((zzz_radiostat & 1) == 0)
		return;

	if ((zzz_radiostat &= ~1) == 0)
		/* Receiver disabled as well */
		xcv_sleep ();
	else
		rcv_enable_warm ();
}

static void rcv_enable (void) {

	if ((zzz_radiostat & 2) != 0)
		return;

	if (!zzz_radiostat)
		/* Sleeping */
		rcv_enable_cold ();

	// rcv_enable_warm ();
	/*
	 * Don't do that if xmitter is running, the receiver will be enabled
	 * by xmt_disable if the flag below is set.
	 */

	zzz_radiostat |= 2;
}

static void rcv_disable (void) {

	if ((zzz_radiostat & 2) == 0)
		return;

	if ((zzz_radiostat &= ~2) == 0)
		xcv_sleep ();
	else
		/* This will not happen */
		xmt_enable_warm ();
}

static void xcv_disable (void) {
	xcv_sleep ();
}

#if ! ECOG_SIM

static void xcv_restore (void) {

	/* Restore device status */
	word st = zzz_radiostat;

	zzz_radiostat = 0;

	if (st & 2)
		rcv_enable ();
	if (st & 1)
		xmt_enable ();
}

#endif

static int ioreq_radio (int operation, char *buf, int len) {

	int res;

	switch (operation) {

		case READ:

			if ((len < 8 || len & 0x3) && (rd_flags & RF_CHECK))
				syserror (EREQPAR, "ioreq_radio/read");
			if ((res = rx_pkt (buf, len)) < 0) {
				return 0;
			}
			return res;

		case WRITE:

			if ((len < 8 || len & 0x3) && (rd_flags & RF_CHECK))
				syserror (EREQPAR, "ioreq_radio/write");
			tx_pkt (buf, len);

			return len;

		case CONTROL:

			switch (len) {

				case RADIO_CNTRL_SETPRWAIT:

#if	RADIO_TYPE != RADIO_XEMICS
					rd_prwait = *((word*)buf);
#endif
					return 1;

				case RADIO_CNTRL_SETPRTRIES:

#if	RADIO_TYPE != RADIO_XEMICS
					rd_prtries = *((word*)buf);
#endif
					return 1;

				case RADIO_CNTRL_SETPOWER:

#if	RADIO_TYPE == RADIO_XEMICS
					xe_setpower (*((word*)buf));
#endif
					return 1;

				case RADIO_CNTRL_CALIBRATE:

					w_calibrate (*((word*)buf));
					return 1;

				case RADIO_CNTRL_CHECKSUM:

					if (*buf)
						/* Checksum on */
						rd_flags |= RF_CHECK;
					else
						rd_flags &= ~RF_CHECK;
					return 1;

				case RADIO_CNTRL_SETPRLEN:

					rd_prlen = *((word*)buf);
					return 1;

				case RADIO_CNTRL_READSTAT:
#if	RADIO_INTERRUPTS
					return rcvlast ();
#else
					return 0;
#endif
				case RADIO_CNTRL_READPOWER:

#if	RADIO_TYPE != RADIO_XEMICS
					return 1;
#else
					return xe_rdpower ();
#endif
				case RADIO_CNTRL_XMTCTRL:

					if (*buf)
						xmt_enable ();
					else
						xmt_disable ();
					return 1;

				case RADIO_CNTRL_RCVCTRL:

					if (*buf)
						rcv_enable ();
					else
						rcv_disable ();
					return 1;

		}
		default:
			syserror (ENOOPER, "ioreq_radio");
			/* No return */
			return 0;
	}
}

void devinit_radio (int dummy) {
/*
 * Initialize the ports
 */
	drvdata = umalloc (DRVDATA_SIZE * 2);

	if (drvdata == NULL)
		syserror (EMALLOC, "devinit_radio");

	/* Number of cycles in transmitted preamble */
	rd_prlen = TXL_DEFPRLEN;

#if	RADIO_TYPE != RADIO_XEMICS

	/* Number of retries for received preamble */
	rd_prtries = RCL_DEFPRTRIES;
	/* Waiting for preamble high in msec */
	rd_prwait = RCL_DEFPRWAIT;
	/* For consistency */
	rc_last = 0;

#else
	/*
	 * Fill in the register data. We keep them on the side to enable
	 * their modification without having to read them from the chip.
	 * This way, the SO line (from XEMICS to eCOG) is optional. Well,
	 * we don't really want to be able to modify them all, but who
	 * knows?
	 */
	xe_regs [ 0] = XEM_CFG_RTPARAM0;
	xe_regs [ 1] = XEM_CFG_RTPARAM1;
	xe_regs [ 2] = XEM_CFG_FSPARAM0;
	xe_regs [ 3] = XEM_CFG_FSPARAM1;
	xe_regs [ 4] = XEM_CFG_FSPARAM2;
	xe_regs [ 5] = 0;	/* Data register */
	xe_regs [ 6] = XEM_CFG_ADPARAM0;
	xe_regs [ 7] = XEM_CFG_ADPARAM1;
	xe_regs [ 8] = XEM_CFG_PATTERN0;
	xe_regs [ 9] = XEM_CFG_PATTERN1;
	xe_regs [10] = XEM_CFG_PATTERN2;
	xe_regs [11] = XEM_CFG_PATTERN3;

	rd_bcount = XMT_DEF_BAUD_COUNT;

#endif
	/* Device starts disabled */
	zzz_radiostat = 0;
	rd_flags = RF_CHECK;

	ini_regs;
	xcv_disable ();
	ini_params ();

#if	RADIO_TYPE != RADIO_XEMICS

	/* For XEMICS, the rate has been set already */

	w_calibrate (RADIO_DEF_BITRATE);

	diag ("RFMI calibrated transceiver timers: %u/%u", txl_factor,
		zzz_rcl_thrshld [0]);
#endif
	adddevfunc (ioreq_radio, RADIO);

}
#endif
