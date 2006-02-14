/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include <ecog.h>
#include <ecog1.h>
#include "kernel.h"

#include "irq_timer_headers.h"

//+++ "gpioirq.c"

extern 			pcb_t	*zz_curr;
extern 			word  	zz_mintk;
extern 	volatile 	word 	zz_lostk;
extern 			address	zz_utims [MAX_UTIMERS];

// This must be the same as the stack fill pattern used in cstartup.asm
#define	STACK_SENTINEL	0xB779
#define	ISTACK_SENTINEL	0xB77A

void	zz_malloc_init (void);

/* ========================== */
/* Device driver initializers */
/* ========================== */
#if	UART_DRIVER
static void	devinit_uart (int);
#endif
#if	LCD_DRIVER
static void	devinit_lcd (int);
#endif
#if	ETHERNET_DRIVER
void		devinit_ethernet (int);
#endif
#if	RADIO_DRIVER
void		devinit_radio (int);
#endif

const static devinit_t devinit [MAX_DEVICES] = 	{
/* === */
#if	UART_DRIVER
		{ devinit_uart,	 0 },
#else
		{ NULL, 0 },
#endif
/* === */
#if	UART_DRIVER > 1
		{ devinit_uart,	 1 },
#else
		{ NULL, 0 },
#endif
/* === */
#if	LCD_DRIVER
		{ devinit_lcd,	 0 },
#else
		{ NULL, 0 },
#endif
/* === */
#if	ETHERNET_DRIVER
	      	{ devinit_ethernet,   0 },
#else
		{ NULL, 0 },
#endif
/* === */
#if	RADIO_DRIVER
		{ devinit_radio,   0 },
#else
		{ NULL, 0 },
#endif
	 };

static void ssm_init (void), cnf_init (void), rtc_init (void),
	    mem_init (void), ios_init (void);

void powerdown (void) {
/* =================================================================== */
/* Note: I don't think I can do that while I am waiting for UART input */
/* because it will mess up the UART's clock. I may try later to drive  */
/* the UART from a low REF source.                                     */
/*                                                                     */
/* OK, low-power operation still doesn't work,  but I think I know how */
/* to do it, so it will be done some day.                              */
/* =================================================================== */

	/* Assert low reference as source until it is accepted. */
	while (fd.ssm.cpu.sts != SSM_CPU_STS_LOW_REF_CLK) {
		fd.ssm.cfg.clk_sel = SSM_CFG_CLK_SEL_LOW_REF_CLK;
	}
	rg.ssm.clk_dis =
		SSM_CLK_DIS_HIGH_OSC_MASK |
		SSM_CLK_DIS_HIGH_PLL_MASK;
}

void powerup (void) {

	rg.ssm.clk_en =
		SSM_CLK_EN_HIGH_OSC_MASK |
		SSM_CLK_EN_HIGH_PLL_MASK;

	/* Assert high reference as source until it is accepted. */
	while (fd.ssm.cpu.sts != SSM_CPU_STS_HIGH_PLL_CLK) {
		fd.ssm.cfg.clk_sel = SSM_CFG_CLK_SEL_HIGH_PLL_CLK;
	}
}

void clockdown (void) { /* Stub */ }
void clockup (void) { /* Stub */ }

void reset (void) {

	cli_tim;
	resetcpu;
	while (1);
}

void halt (void) {

	cli_tim;
	diag ("PicOS halted");
	while (1)
		SLEEP;
}

#if	ADC_PRESENT

static word	adc_ticks, adc_delay, adc_modelist [4], adc_values [4];
static int	adc_cmode, adc_nmodes;

void adc_start (int mode, int which, int interval) {

	word mo;

	cli_tim;
	/* Switch it off */
	fd.ssm.cfg.adc_en = 0;
	/* Reset */
	fd.ssm.ex_ctrl.adc_rst_set = 1;
	adc_ticks = adc_delay = (word) interval;

	switch (mode) {

	    case ADC_MODE_TEMP:
		/*
		 * Ignore 'which' in this case. There is a single value, so
		 * we have no alternatives.
		 */
		adc_nmodes = 1;
		adc_modelist [0] =
			ADC_CFG_TEMP_EN_MASK |
			ADC_CFG_TEMP_SEL_MASK;
		break;

	    case ADC_MODE_VOLTAGE:

		adc_nmodes = 1;
		adc_modelist [0] =
			ADC_CFG_VSENS_EN_MASK |
			ADC_CFG_VSENS_SEL_MASK;
		break;

	    case ADC_MODE_INTREF:
		/*
		 * Up to four inputs, internal reference
		 */
		adc_nmodes = 0;
		for (mo = 0; mo < 4; mo++)
			if ((which & (1 << mo)) != 0)
				adc_modelist [adc_nmodes++] = mo;

		break;

	    case ADC_MODE_OUTREF:
		/*
		 * Up to three inputs, internal reference goes out on A3
		 */
		adc_nmodes = 0;
		for (mo = 0; mo < 3; mo++)
			if ((which & (1 << mo)) != 0)
				adc_modelist [adc_nmodes++] = mo | 0x4;
		break;

	    case ADC_MODE_EXTREF:
		/*
		 * Up to three inputs, external reference on A3
		 */
		adc_nmodes = 0;
		for (mo = 0; mo < 3; mo++)
			if ((which & (1 << mo)) != 0)
				adc_modelist [adc_nmodes++] = mo | 0x8;
		break;

	    case ADC_MODE_DIFFER:
		/*
		 * Up to two differentials: A0 - A1, A2 - A3
		 */
		adc_nmodes = 0;
		for (mo = 0; mo < 2; mo++)
			if ((which & (1 << mo)) != 0)
				adc_modelist [adc_nmodes++] = mo | 0xc;
		break;

	}

	if (adc_nmodes < 1)
		syserror (EREQPAR, "adc nmodes");

	adc_cmode = 0;
	/* Start it up */
	fd.ssm.ex_ctrl.adc_rst_clr = 1;
	rg.adc.cfg = adc_modelist [0];
	fd.ssm.cfg.adc_en = 1;
	sti_tim;
}

int adc_read (int ix) {

	word v;

	sysassert (ix >= 0 && ix < 4, "adc read index");

	if ((v = adc_values [ix]) & 0x0800)
		v |= 0xf000;

	return (int) v;
}

void adc_stop (void) {

	rg.adc.cfg = 0;
	fd.ssm.cfg.adc_en = 0;
	adc_ticks = adc_delay = 0;
}

#endif


void udelay (word n) {
/* =================================== */
/* n should be roughly in microseconds */
/* =================================== */
	while (n) {
#if	ECOG_SIM == 0
		nop (); nop (); nop (); nop (); nop ();
		nop (); nop (); nop (); nop (); nop ();
		nop (); nop (); nop (); nop (); nop ();
		nop (); nop (); nop (); nop (); nop ();
#endif
		n--;
	}
}

void mdelay (word n) {

/* ============ */
/* milliseconds */
/* ============ */
	while (n--)
		udelay (995);
}

void zzz_sched (void) {

	/* Initialization */
	ssm_init ();
	mdelay (100);
	cnf_init ();
	mem_init ();
	rtc_init ();
	mdelay (100);
	ios_init ();
#if	TCV_PRESENT
	tcv_init ();
#endif
	/* The event loop */
	zz_curr = (pcb_t*) fork (root, NULL);
	/* Delay root startup until the drivers have been initialized */
#if ECOG_SIM ==0
	delay (2048, 0);
#else
	delay (64, 0);
#endif

	// Run the scheduler

#include "scheduler.h"

}

#if SWITCHES
word switches (void) {
	return 	 (fd.io.gp0_7_sts.sts4   == 0) |
		((fd.io.gp0_7_sts.sts5   == 0) << 1) |
		((fd.io.gp8_15_sts.sts13 == 0) << 2) |
		((fd.io.gp8_15_sts.sts14 == 0) << 3) ;
}
#endif

#if DIAG_MESSAGES > 1
void zzz_syserror (int ec, const char *m) {
	diag ("SYSTEM ERROR: %x, %s", ec, m);
#else
void zzz_syserror (int ec) {
	diag ("SYSTEM ERROR: %x", ec);
#endif
	print_al (0);
	while (1) {
#if LEDS_DRIVER
		leds (0, 1); leds (1, 1); leds (2, 1); leds (3, 1);
		mdelay (100);
		leds (0, 0); leds (1, 0); leds (2, 0); leds (3, 0);
		mdelay (100);
#else
		SLEEP;
#endif
	}
}

static void ssm_init () {
/* ===================================================================== */
/* This I have creatively copied from the examples (and annotated a bit) */
/* ===================================================================== */

	/* Remove the reset from the ripple counters. */
	rg.ssm.rst_clr =
		SSM_RST_CLR_HIGH_PLL_DIV_CHN_MASK
#ifdef EMI_USED
		| SSM_RST_CLR_EMI_MASK
#endif
		| SSM_RST_CLR_LOW_REF_DIV_CHN_MASK

		;

	/* Enable high reference and high PLL. */
	rg.ssm.clk_en =
		SSM_CLK_EN_HIGH_OSC_MASK |
#ifdef EMI_USED
		SSM_CLK_EN_EMI_MASK |
#endif
		SSM_CLK_EN_HIGH_PLL_MASK;
	/* ============================================================== */
	/* PG: this enables the HF crystal clock and HF PLL (phase locked */
	/* loop).  According to the docs,  there are two crystals:  5 MHz */
	/* and 32.768 kHz.   PLL is used to obtain  high  frequency  (the */
	/* manual is a piece of crap - I couldn't find a word about this) */
	/* reference clocks that are then divided to obtain all the other */
	/* clocks needed by the CPU.                                      */
	/* ============================================================== */

	/* Add a wait state because the CPU frequency is above 10 MHz. */
	fd.mmu.flash_ctrl.wait_states = 1;

	/* Set CPU divider for maximum frequency. (High PLL/ 4) */
	fd.ssm.cpu.prescaler = 7;
	/* ============================================== */
	/* The 7 means 'divide by 2'. This yields 50 MHz. */
	/* ============================================== */
	fd.ssm.cpu.cpu_clk_div = 6;
	/* Divide by 2 to produce 25MHz CPU clock */

	/* Assert high PLL as source until it is accepted. */
	while (fd.ssm.cpu.sts != SSM_CPU_STS_HIGH_PLL_CLK)
	{
		fd.ssm.cfg.clk_sel =
			SSM_CFG_CLK_SEL_HIGH_PLL_CLK
				| SSM_CFG_PLL_STEPUP_MASK
				;
	}

	/* Configure operation of clocks during sleep and wakeup modes. */
	rg.ssm.clk_sleep_dis =
		SSM_CLK_SLEEP_DIS_UARTA_MASK |
		SSM_CLK_SLEEP_DIS_UARTB_MASK |
		SSM_CLK_SLEEP_DIS_TIMEOUT_MASK;
	rg.ssm.clk_wake_en =
		SSM_CLK_WAKE_EN_UARTA_MASK |
		SSM_CLK_WAKE_EN_UARTB_MASK;

	/* Remove the reset. */
	rg.ssm.rst_clr = SSM_RST_CLR_TMR_MASK;

#if	ADC_PRESENT || CC1000
	/* Enable ADC */

	rg.ssm.ex_ctrl =
		SSM_EX_CTRL_ADC_HIGH_REF_CLK_MASK |
		SSM_EX_CTRL_ADC_RST_CLR_MASK;
	/* We are not using interrupts for this */
	fd.adc.ctrl.int_dis = 1;
	/* Starts disabled */
	fd.ssm.cfg.adc_en = 0;
#endif

}

static void cnf_init () {
/* =================================================================== */
/* Port configuration - must be before mem_init or SDRAM won't work !! */
/* =================================================================== */

#define	PORT_A_ENABLE	0
#define	PORT_B_ENABLE	0
#define	PORT_C_ENABLE	0
#define	PORT_D_ENABLE	0
#define	PORT_E_ENABLE	0
#define	PORT_F_ENABLE	0
#define	PORT_G_ENABLE	0
#define	PORT_H_ENABLE	0
#define	PORT_I_ENABLE	0
#define	PORT_J_ENABLE	0
#define	PORT_K_ENABLE	0
#define	PORT_L_ENABLE	0

/*
 * GPIO assignment:
 *
 *            00-07     PortA (LEDs 0-3 + XEMICS 0-3 + Ethernet 6-7 + SW1-2 4-5
 *                      4-5 available if switch is OFF -> piggy radio?)
 *            08-10     PortB (LCD 3-5)
 *            11-15     PortL (RADIO 3-7)
 *            16        Free  (C3?)
 *            17-20     PortJ (LCD 2-5)
 *            21-22     PortD (RADIO 2-3)
 * 
 */

/* ====== */
/* PORT A */
/* ====== */
#if	SWITCHES || LEDS_DRIVER || ETHERNET_DRIVER || RADIO_TYPE == RADIO_XEMICS || CC1000
	/*
	 * Port A as GPIO 0-7, needed for the LEDs on A0-A3, side effect:
	 * A4-A7 -> GPIO 4-7 (used by the Ethernet chip). The LEDs ports
	 * (configured as output) are used to drive some pins on the
	 * XEMICS.
	 */
#undef	PORT_A_ENABLE
#define	PORT_A_ENABLE		PORT_EN_A_MASK
	fd.port.sel1.a = 11;
#endif

#if	LCD_DRIVER || CC1000
	/*
	 * B3-B5 as GPIO 8-10, B0-B2 -> SPI (SCLK, MOSI, MISO), B6,B7 ->
	 * USART (DATA_IN, DATA_OUT); B3-B5 needed by LCD (see also Port J)
	 * CHICPON doesn't want to work without it, although it doesn't
	 * need Port B. Strange.
	 */
#undef	PORT_B_ENABLE
#define	PORT_B_ENABLE		PORT_EN_B_MASK
	fd.port.sel1.b = 3;
#endif

#if 	SWITCHES
	/*
	 * Port C (4 signals) as GPIO13-16. Conflicts with Port L, except
	 * for C3 (GPIO16).
	 */
#undef	PORT_C_ENABLE
#define	PORT_C_ENABLE		PORT_EN_C_MASK
	fd.port.sel1.c = 7;
#endif

#if	UART_DRIVER == 2 || RADIO_TYPE == RADIO_XEMICS
	/*
	 * Port D as DUART B (D0-D1), side effect: D2-D3 -> GPIO 21-22. D2 is
	 * used by XEMICS.
	 */
#undef	PORT_D_ENABLE
#define	PORT_D_ENABLE		PORT_EN_D_MASK
	fd.port.sel1.d = 1;
#endif

#if	EMI_USED
	/*
	 * Ports E-I are used by EMI
	 */
#undef	PORT_E_ENABLE
#define	PORT_E_ENABLE		PORT_EN_E_MASK
	fd.port.sel1.e = 0;
#endif

#if	EMI_USED
	/* Port F as A8-15 (address lines that is). */
#undef	PORT_F_ENABLE
#define	PORT_F_ENABLE		PORT_EN_F_MASK
	fd.port.sel1.f = 0;
#endif

#if	EMI_USED
	/* Port G as D0-7 (data lines that is). */
#undef	PORT_G_ENABLE
#define	PORT_G_ENABLE		PORT_EN_G_MASK
	fd.port.sel2.g = 0;
#endif

#if	EMI_USED
	/* Port H as D8-D15 (more data lines). */
#undef	PORT_H_ENABLE
#define	PORT_H_ENABLE		PORT_EN_H_MASK
	fd.port.sel2.h = 0;
#endif

#if	EMI_USED
	/* Port I as EMI control lines. */
#undef	PORT_I_ENABLE
#define	PORT_I_ENABLE		PORT_EN_I_MASK
	fd.port.sel2.i = 0;
#endif

#if	UART_DRIVER || LCD_DRIVER
	/*
	 * Port J as a DUART A (J0-J1), side effect: J2-J5 -> GPIO 17-20, with
	 * GPIO 17-20 being used by the LCD.
	 * J6-J7 -> PWM 1-2
	 */
#undef	PORT_J_ENABLE
#define	PORT_J_ENABLE		PORT_EN_J_MASK
	fd.port.sel2.j = 3;
#endif

#if 0
#undef	PORT_K_ENABLE
#define	PORT_K_ENABLE		PORT_EN_K_MASK
	/*
	 * PIOB on port K
	 */
	fd.port.sel2.k = 1;
#endif

#if 	RADIO_DRIVER || CC1000
/*
 * L3-7 on GPIO_11-15 (not used by anything else) L0-2 on GPIO8-10 conflict
 * with Port B.
 */
#undef	PORT_L_ENABLE
#define	PORT_L_ENABLE		PORT_EN_L_MASK
	fd.port.sel2.l = 2;
#endif
	/* Enable the ports */
	rg.port.en =	PORT_A_ENABLE |
			PORT_B_ENABLE |
			PORT_C_ENABLE |
			PORT_D_ENABLE |
			PORT_E_ENABLE |
			PORT_F_ENABLE |
			PORT_G_ENABLE |
			PORT_H_ENABLE |
			PORT_I_ENABLE |
			PORT_J_ENABLE |
			PORT_K_ENABLE |
			PORT_L_ENABLE;

#if 	SWITCHES
	// Use switches, make GPIO 4,5,13,14 input
	rg.io.gp4_7_out = IO_GP4_7_OUT_DIS4_MASK | IO_GP4_7_OUT_DIS5_MASK;
	rg.io.gp12_15_out = IO_GP12_15_OUT_DIS13_MASK |
		IO_GP12_15_OUT_DIS14_MASK;
#endif

#if	LEDS_DRIVER
	// Make GPIO 0-3 output
	rg.io.gp0_3_out =
		IO_GP0_3_OUT_EN0_MASK | IO_GP0_3_OUT_SET0_MASK |
		IO_GP0_3_OUT_EN1_MASK | IO_GP0_3_OUT_SET1_MASK |
		IO_GP0_3_OUT_EN2_MASK | IO_GP0_3_OUT_SET2_MASK |
		IO_GP0_3_OUT_EN3_MASK | IO_GP0_3_OUT_SET3_MASK ;
#endif

}

static void rtc_init () {

	/* Configure the clock input source. */
	fd.ssm.tap_sel3.tmr = 1; /* 32 */
	fd.ssm.div_sel.tmr = 0;
	fd.ssm.div_sel.low_clk_timer = 1;

	/* =========================================================== */
	/* PG: this selects the low frequency clock source as the RTC, */
	/* whatever that is. rtc = 0 mean reference counter as opposed */
	/* to PLL counter.                                             */
	/* =========================================================== */

	/* Flag no processes waiting for timer */
	zz_lostk = zz_mintk = 0;

	/* Enable the clock input. */
	rg.ssm.clk_en = SSM_CLK_EN_TMR_MASK;

	rg.tim.ctrl_en =
		TIM_CTRL_EN_TMR_CNT_MASK |
		TIM_CTRL_EN_TMR_AUTO_RE_LD_MASK;

	rg.tim.tmr_ld = 3;
	fd.tim.cmd.tmr_ld = 1;
	sti_tim;
	/* ========================================== */
	/* System clock runs at 1024 ticks per second */
	/* ========================================== */
}

/* =============== */
/* Timer interrupt */
/* =============== */

void __irq_entry timer_int () {

	/* Clear the interrupt condition */
	fd.tim.int_clr1.tmr_exp = 1;

#if	STACK_GUARD
	if (*((word*)estk_) != STACK_SENTINEL) {
		/* We cannot call a function here */
		print_al (0);
		while (1)
			SLEEP;
	}
#endif
	/* This code assumes that MAX_UTIMERS is 4 */
	if (zz_utims [0]) {
		if (*(zz_utims [0]))
			(*(zz_utims [0]))--;
		if (zz_utims [1]) {
			if (*(zz_utims [1]))
				(*(zz_utims [1]))--;
			if (zz_utims [2]) {
				if (*(zz_utims [2]))
					(*(zz_utims [2]))--;
				if (zz_utims [3]) {
					if (*(zz_utims [3]))
						(*(zz_utims [3]))--;
				}
			}
		}
	}

	zz_lostk++;

#if	ADC_PRESENT
	if (adc_ticks && (--adc_ticks == 0)) {
		adc_ticks = adc_delay;
		adc_values [adc_cmode] =
			fd.adc.sts.data;
		if (adc_nmodes > 1) {
			if (++adc_cmode >= adc_nmodes)
				adc_cmode = 0;
			rg.adc.cfg = adc_modelist [adc_cmode];
		}
	}
#endif

	// For extras
#include "irq_timer.h"

	if (zz_lostk & 1024) {
		// Run the scheduler at least once every second - to keep the
		// second clock up to date
		RISE_N_SHINE;
		return;
	}

	if (zz_mintk && zz_mintk <= zz_lostk) {
		RISE_N_SHINE;
	}
}

#if	SDRAM_PRESENT

void zz_sdram_test (void);

#define	SDRAM_MBSIZE	((word)(1 << (SDRAM_SIZE - 9)))

static void mem_map (word blk) {

	/* ================================================================ */
	/* We are not very flexible here.   As we are mapping starting from */
	/* the middle of the logical address space (at 0x4000), and we want */
	/* to map 32K  (max of what we sensibly can),  we have to use  both */
	/* translators,  which  makes copying across different pages tricky */
	/* and  messy.   But  at least we are able to access all this large */
	/* memory somehow.                                                  */
	/* ================================================================ */

	/* 1/2 of actual size / 256 */
	blk <<= (SDRAM_SIZE - 8);
	rg.mmu.ext_cs0_data0_log =
					((word)SDRAM_ADDR >> 8);
	rg.mmu.ext_cs0_data0_phy = blk;
	fd.mmu.ext_cs0_data0_size.ext_cs0_data0_size = (SDRAM_MBSIZE - 1);

	/* Second half */
	rg.mmu.ext_cs0_data1_log =
					((word)SDRAM_ADDR >> 8) + SDRAM_MBSIZE;
	rg.mmu.ext_cs0_data1_phy = blk + SDRAM_MBSIZE;
	fd.mmu.ext_cs0_data1_size.ext_cs0_data1_size = (SDRAM_MBSIZE - 1);
	//mdelay (100);
	/* ================================================ */
	/* The first reference to SDRAM causes problems.    */
	/* Checked it again and the problem is still there. */
	/* ================================================ */
	*((volatile word*)SDRAM_ADDR) = *((volatile word*)SDRAM_ADDR);
}

/* =================================================== */
/* SDRAM copy buffer - must be located in short memory */
/* =================================================== */
static word rambuf [SDRAM_CPBUFSIZE];

void ramget (address to, lword from, int nw) {

	word i, lw, nb, off;

	if (nw < 0 || (from + nw) > SDRAM_SPARE)
		syserror (EREQPAR, "ramget");

	/* Block number (from zero) */
	nb = (word) (from >> SDRAM_SIZE);
	/* Offset */
	off = (word) (from & (SDRAM_BLKSIZE - 1));

	lw = (word)to + nw;
	/* ======================================================= */
	/* Formally,  we are allowed to wrap around, so let us not */
	/* worry about details. But check if the destination falls */
	/* into SDRAM,  in which case we will have to remap and go */
	/* through the intermediate buffer.                        */
	/* ======================================================= */
	if (
	     ( ((word)to >= (word)SDRAM_ADDR) &&
	       ((word)to <  (word)(SDRAM_ADDR + SDRAM_BLKSIZE)) ) ||
	     ( (      lw >= (word)SDRAM_ADDR) &&
	       (      lw <  (word)(SDRAM_ADDR + SDRAM_BLKSIZE)) ) ) {

		/* We have to do it the tricky way */
		while (1) {
			lw = (nw > SDRAM_CPBUFSIZE) ? SDRAM_CPBUFSIZE : nw;
			/* Map the block in */
			mem_map (nb+1);
			/* Make sure we don't cross block boundaries */
			if (off + lw > SDRAM_BLKSIZE)
				lw = SDRAM_BLKSIZE - off;
			for (i = 0; i < lw; i++)
				rambuf [i] = sdram [off++];
			/* Re-map block zero */
			mem_map (0);
			/* Copy buffer to destination */
			for (i = 0; i < lw; i++) {
				*to++ = rambuf [i];
			}
			nw -= lw;
			if (nw == 0)
				break;
			if (off == SDRAM_BLKSIZE) {
				off = 0;
				nb++;
			}
		}
		return;
	}

	/* The destination is in short memory */
	mem_map (nb+1);
	while (1) {
		lw = (off + nw > SDRAM_BLKSIZE) ? SDRAM_BLKSIZE - off : nw;
		for (i = 0; i < lw; i++)
			*to++ = sdram [off++];
		nw -= lw;
		if (nw == 0)
			break;
		off = 0;
		nb++;
		mem_map (nb+1);
	}
	mem_map (0);
}

void ramput (lword to, address from, int nw) {

	word i, lw, nb, off;

	if (nw < 0 || (to + nw) > SDRAM_SPARE)
		syserror (EREQPAR, "ramput");

	/* Block number (from zero) */
	nb = (word) (to >> SDRAM_SIZE);
	/* Offset */
	off = (word) (to & (SDRAM_BLKSIZE - 1));
	lw = (word)from + nw;
	if (
	     ( ((word)from >= (word)SDRAM_ADDR) &&
	       ((word)from <  (word)(SDRAM_ADDR + SDRAM_BLKSIZE)) ) ||
	     ( (      lw   >= (word)SDRAM_ADDR) &&
	       (      lw   <  (word)(SDRAM_ADDR + SDRAM_BLKSIZE)) ) ) {

		/* Source in SDRAM */
		while (1) {
			lw = (nw > SDRAM_CPBUFSIZE) ?  SDRAM_CPBUFSIZE : nw;
			if (off + lw > SDRAM_BLKSIZE)
				lw = SDRAM_BLKSIZE - off;
			for (i = 0; i < lw; i++)
				rambuf [i] = *from++;
			mem_map (nb+1);
			for (i = 0; i < lw; i++)
				sdram [off++] = rambuf [i];
			/* Re-map block zero */
			mem_map (0);
			nw -= lw;
			if (nw == 0)
				break;
			if (off >= SDRAM_BLKSIZE) {
				off = 0;
				nb++;
			}
		}
		return;
	}

	/* The destination is in short memory */

	mem_map (nb+1);
	while (1) {
		lw = (off + nw > SDRAM_BLKSIZE) ? SDRAM_BLKSIZE - off : nw;
		for (i = 0; i < lw; i++)
			sdram [off++] = *from++;
		nw -= lw;
		if (nw == 0)
			break;
		off = 0;
		nb++;
		mem_map (nb+1);
	}
	mem_map (0);
}

#endif

#if	STACK_GUARD
extern	word STACK_SIZE;

word zzz_stackfree () {
	
	word p;

	for (p = 0; p < STACK_SIZE; ) {
		// On the eCOG, STACK_SIZE is in words
		p++;
		if (*(((address)estk_) + p) != STACK_SENTINEL)
			break;
	}
	return p;
}
#endif


static void mem_init () {

#if	SDRAM_PRESENT

	/* Chip select for SDRAM */
	rg.emi.sdram_cfg = 0x940d;
	rg.emi.sdram_refr_per = 0x330c;
	rg.emi.sdram_refr_cnt = 0x0010;
	rg.emi.ctrl_sts = 0x0804;
	fd.emi.sdram_cust_adr.adr = 0x400;
	rg.emi.sdram_cust_cmd = 0x7c85;
	fd.emi.sdram_cust_adr.adr = 0x030;
	rg.emi.sdram_cust_cmd = 0x7c01;
	rg.emi.sdram_cust_cmd = 0x0c63;
#endif

#if	ETHERNET_DRIVER
	/* Second chip select for the Ethernet chip, if it works at all */
	fd.emi.bus_cfg1.cs_en = 1;
	fd.emi.bus_cfg1.word = 1;
	fd.emi.bus_cfg1.wait_en = 1;
	fd.emi.bus_cfg1.ds_en = 1;
	fd.emi.bus_cfg1.ah_en = 1;
	fd.emi.ctrl_sts.bus_en = 1;
#endif

	/* Enable Translators. */
#if	SDRAM_PRESENT
	fd.mmu.translate_en.ext_cs0_data0 = 1;
	fd.mmu.translate_en.ext_cs0_data1 = 1;
#endif

#if	ETHERNET_DRIVER
	fd.mmu.translate_en.ext_cs1_data0 = 1;
#endif

#if	SDRAM_PRESENT
	mem_map (0);
#endif

#if	ETHERNET_DRIVER
	rg.mmu.ext_cs1_data0_log = (ETHER_ADDR >> 8);
	rg.mmu.ext_cs1_data0_phy = 0x0;
	fd.mmu.ext_cs1_data0_size.ext_cs1_data0_size = 0x03; /* 1K */
#endif
}

static void ios_init () {

	int i;
	pcb_t *p;

	for_all_tasks (p)
		/* Mark all task table entries as available */
		p->code = NULL;

#if	UART_DRIVER
	/* UART_A is initialized first, to enable diagnostic output */
	devinit [UART_A] . init (devinit [UART_A] . param);
	diag ("");
#ifdef	BANNER
	diag (BANNER);
#else
	diag ("\r\nPicOS v" SYSVERSION ", "
        	"Copyright (C) Olsonet Communications, 2002-2005");
	diag ("Leftover RAM: %d words", (word)estk_ - (word)evar_);
#endif
#if	SDRAM_PRESENT
	zz_sdram_test ();
#endif
#endif
	/* ========================================================== */
	/* Initialize malloc.   We cannot have it in mem_init because */
	/* SDRAM requires rtc_init to work. Besides, SDRAM test would */
	/* destroy it.                                                */
	/* ========================================================== */
	zz_malloc_init ();

	/* Initialize other devices and create their drivers */
	for (i = UART_B; i < MAX_DEVICES; i++)
		if (devinit [i] . init != NULL)
			devinit [i] . init (devinit [i] . param);
#if 0
	diag ("PicOS reporting on UART_A, hit '+' to proceed");
	//
	diag_disable_int (a);
	while (1) {
		byte uc;
		if ((rg.duart.a_sts &
			DUART_A_STS_RX_1B_RDY_MASK) == 0)
				continue;
		uc = rg.duart.a_rx;
		diag_wait (a);
		diag_wchar (uc, a);
		if (uc == '+')
			break;
	}
/* */
#endif
#if 0
	dibg ("PicOS reporting on UART_B, hit '+' to proceed");
	//
	diag_disable_int (b);
	rg.duart.b_int_clr = 0xffff;
	while (1) {
		byte uc;
		rg.duart.b_int_clr = 0xffff;
		if ((rg.duart.b_sts &
			DUART_B_STS_RX_1B_RDY_MASK) == 0)
				continue;
		/* UART_B is broken: this is how you read a byte from it */
		uc = rg.duart.b_tx16;
		diag_wait (b);
		diag_wchar (uc, b);
		if (uc == '+')
			break;
	}
/* */
#endif

#if EEPROM_DRIVER
	zz_ee_init ();
#endif

}

/* ------------------------------------------------------------------------ */
/* ============================ DEVICE DRIVERS ============================ */
/* ------------------------------------------------------------------------ */

#if	UART_DRIVER
/* ========= */
/* The UARTs */
/* ========= */

uart_t	zz_uart [UART_DRIVER];

#define uart_canwrite(u) \
	((u)->selector?(rg.duart.b_sts & \
	DUART_B_STS_TX_RDY_MASK): \
	(rg.duart.a_sts & DUART_A_STS_TX_RDY_MASK))

#define uart_write16(u,w) do { \
	if ((u)->selector) \
		rg.duart.b_tx16 = (w); \
	else \
		rg.duart.a_tx16 = (w); \
	} while (0)

#define uart_write8(u,w) do { \
	if ((u)->selector) \
		rg.duart.b_tx8 = (w); \
	else \
		rg.duart.a_tx8 = (w); \
	} while (0)

#define uart_enable_write_int(u) do { \
	if ((u)->selector) \
		uart_b_enable_write_int; \
	else \
		uart_a_enable_write_int; \
	} while (0)

#define uart_disable_int(u) do { \
	if ((u)->selector) \
		uart_b_disable_int; \
	else \
		uart_a_disable_int; \
	} while (0)

#define uart_canread(u) \
	((u)->selector?(rg.duart.b_sts & \
	DUART_B_STS_RX_1B_RDY_MASK): \
	(rg.duart.a_sts & DUART_A_STS_RX_1B_RDY_MASK))
/*
 * UART_B is badly broken: you read from the 16-bit TX register (!!!)
 */
#define	uart_read(u) \
		((u)->selector?rg.duart.b_tx16:rg.duart.a_rx)

#define uart_enable_read_int(u) do { \
	if ((u)->selector) \
		uart_b_enable_read_int; \
	else \
		uart_a_enable_read_int; \
	} while (0)

#define	inccbp(p)	do { \
	if (++(p) == UART_BUF_SIZE) (p) = 0; \
	} while (0)

static void uart_lock (uart_t*), uart_unlock (uart_t*);

/* ======================= */
/* Common request function */
/* ======================= */
static int ioreq_uart (uart_t *u, int operation, char *buf, int len) {

	int ii, count = 0;

	switch (operation) {

		case READ:

			if (u->lock) {
				while (len && uart_canread (u)) {
					buf [count++] = uart_read (u);
					len--;
				}
				/* May return zero */
			} else {
				while (len > 0 && u->iou != u->iin) {
					*buf++ = u->ibuf [u->iou];
					inccbp (u->iou);
					count++;
					len--;
				}
				iotrigger (UART_BASE + u->selector, REQUEST);
				if (count == 0)
					/* Block */
					return -1;
			}
			return count;

		case WRITE:

			if (u->lock) {
				while (len && uart_canwrite (u)) {
					uart_write8 (u, buf [count]);
					count++;
					len--;
				}
			} else {
				while (len > 0) {
					ii = u->oin + 1;
					if (ii == UART_BUF_SIZE)
						ii = 0;
					if (ii == u->oou)
						/* The buffer is full */
						break;
					u->obuf [u->oin] = *buf++;
					u->oin = ii;
					count++;
					len--;
				}
				iotrigger (UART_BASE + u->selector, REQUEST);
				if (count == 0)
					return -1;
			}
			return count;

		case CONTROL:

			switch (len) {

				case UART_CNTRL_LCK:

					if (*buf)
						uart_lock (u);
					else
						uart_unlock (u);
					return 1;

				case UART_CNTRL_CALIBRATE:

					// Void for eCOG
					return 1;

			}
			/* Fall through */
		default:

			syserror (ENOOPER, "ioreq_uart");
			/* No return */
			return 0;
	}
}

/* ========================== */
/* Specific request functions */
/* ========================== */
static int ioreq_uart_a (int operation, char *buf, int len) {
	return ioreq_uart (&(zz_uart [0]), operation, buf, len);
}

#if	UART_DRIVER > 1
static int ioreq_uart_b (int operation, char *buf, int len) {
	return ioreq_uart (&(zz_uart [1]), operation, buf, len);
}
#endif

/* ================== */
/* The driver process */
/* ================== */
process (uart_driver, uart_t)

#define	dev data

    bool wr, ww, did;
    word wd;
    int ii;

    entry (0)

	uart_disable_int (dev);

    entry (1)

	ww = wr = did = NO;
	/* ============ */
	/* Output first */
	/* ============ */
	while (dev->oin != dev->oou) {
		/* Check if the port is available */
		if (uart_canwrite (dev)) {
			wd = dev->obuf [dev->oou];
			inccbp (dev->oou);
			did = YES;
			if (dev->oin != dev->oou) {
				/* More than one */
				wd = wd << 8 | dev->obuf [dev->oou];
				inccbp (dev->oou);
				uart_write16 (dev, wd);
			} else {
				uart_write8 (dev, wd);
			}
		} else {
			ww = YES;
			break;
		}
	}
	if (did)
		/* Wake up processes waiting for the buffer */
		iotrigger (UART_BASE + dev->selector, WRITE);
	/* ===== */
	/* Input */
	/* ===== */
	did = NO;
	while (1) {
		ii = dev->iin + 1;
		if (ii == UART_BUF_SIZE)
			ii = 0;
		if (ii == dev->iou)
			break;
		wr = YES;
		if (uart_canread (dev)) {
			dev->ibuf [dev->iin] = uart_read (dev);
			dev->iin = ii;
			did = YES;
		} else {
			wr = YES;
			break;
		}
	}

	if (did)
		iotrigger (UART_BASE + dev->selector, READ);

	/* Always wait for driver requests */
	iowait (UART_BASE + dev->selector, REQUEST, 0);

	if (wr || ww) {
		/* Expect interrupts */
		delay (UART_TIMEOUT, 0);
		iowait (UART_BASE + dev->selector, ATTENTION, 1);
		if (wr)
			uart_enable_read_int (dev);
		if (ww)
			uart_enable_write_int (dev);
	}

#undef dev

endprocess (2)

static void uart_unlock (uart_t *u) {
/* ============================================ */
/* Start up normal (interrupt-driven) operation */
/* ============================================ */
	if (u->lock) {
		u->lock = 0;
		fork (uart_driver, u);
	}
}

static void uart_lock (uart_t *u) {
/* ================================= */
/* Direct (interrupt-less) operation */
/* ================================= */
	int pid;
	if (u->lock == 0) {
		u->lock = 1;
		u->oin = u->oou = u->iin = u->iou = 0;
		uart_disable_int (u);
		pid = find (uart_driver, u);
		if (pid)
			kill (pid);
	}
}
/* =========== */
/* UART_DRIVER */
/* =========== */
#endif

/* ========================== */
/* Interrupt service routines */
/* ========================== */
void __irq_entry uart_a_int (void) {
	uart_a_disable_int;
	rg.duart.a_int_clr = 0xffff;
#if	UART_DRIVER
	/* ========================================================== */
	/* Note that some protocol is required to trigger events from */
	/* interrupt service routines.  Namely, the process issuing a */
	/* wait reauest for such an  event  must  lock  while issuing */
	/* this  request  and  all other wait requests in its current */
	/* state.                                                     */
	/* ========================================================== */
	RISE_N_SHINE;
	i_trigger (ETYPE_IO, devevent (UART_BASE, ATTENTION));
#endif
}

void __irq_entry uart_b_int (void) {
	uart_b_disable_int;
	rg.duart.b_int_clr = 0xffff;
#if	UART_DRIVER > 1
	RISE_N_SHINE;
	i_trigger (ETYPE_IO, devevent (UART_BASE+1, ATTENTION));
#endif
}

#if	UART_DRIVER
/* =========== */
/* Initializer */
/* =========== */
static void devinit_uart (int devnum) {

	if (devnum == 0) {

		/* Remove the reset. */
		rg.ssm.rst_clr = SSM_RST_CLR_DUART_MASK;

		/* Configure the clock input source. */
		fd.ssm.div_sel.low_clk_duart = 0; /* High. */
		fd.ssm.div_sel.duart = 1; /* Low PLL */
		fd.ssm.tap_sel1.duart = 0;

		/* Enable the clock input. */
		rg.ssm.clk_en =
			SSM_CLK_EN_UARTA_MASK
#if	UART_DRIVER > 1
				| SSM_CLK_EN_UARTB_MASK
#endif
		;

		/* 8 bits, no parity, 1 stop bit, tx and rx active low. */
		rg.duart.frame_cfg = 0;

#if	UART_RATE == 1200
#define	uart_baud	640
#endif
#if	UART_RATE == 2400
#define	uart_baud	320
#endif
#if	UART_RATE == 4800
#define	uart_baud	160
#endif
#if	UART_RATE == 9600
#define	uart_baud	80
#endif
#if	UART_RATE == 19200
#define	uart_baud	40
#endif
#if	UART_RATE == 32800
#define	uart_baud	20
#endif

#ifndef	uart_baud
#error "Illegal UART_RATE"
#endif

#if UART_BITS == 8
#define	uctl_char	0
#define	uctl_pena	0
#else
#define	uctl_char	1
#if UART_PARITY == 0
#define	uctl_pena	0x4
#else
#define	uctl_pena	0xc
#endif
#endif

		rg.duart.a_baud = uart_baud;
		rg.duart.frame_cfg |= (uctl_char | uctl_pena);
#if	UART_DRIVER > 1
		rg.duart.b_baud = uart_baud;
		rg.duart.frame_cfg |= ((uctl_char | uctl_pena) << 8);
#endif
		/* Enable tx and rx. */
		rg.duart.ctrl =
			DUART_CTRL_A_TX_EN_MASK |
			DUART_CTRL_A_RX_EN_MASK
#if	UART_DRIVER > 1
							     |
			DUART_CTRL_B_TX_EN_MASK |
			DUART_CTRL_B_RX_EN_MASK
#endif
			;
		/* ========================================================= */
		/* Interrupts still disabled, will enable them in the driver */
		/* ========================================================= */
		adddevfunc (ioreq_uart_a, UART_A);
#if	UART_DRIVER > 1
		adddevfunc (ioreq_uart_b, UART_B);
#endif
	}
	/* Assumes that buffer pointers are initialized to zero */
	zz_uart [devnum] . selector = devnum;
	zz_uart [devnum] . lock = 1;
	/* =============================================================== */
	/* To do it right, I would have to store pointers to UART specific */
	/* stuff,  but  in  the present circumstances,  and with the rigid */
	/* organization of the reg structure,  it  makes more sense to use */
	/* conditional code - for clarity and neatness.                    */
	/* =============================================================== */
	uart_unlock (zz_uart + devnum);
}

#endif
/* ========= */

#if	LCD_DRIVER
/* === */
/* LCD */
/* === */

static lcd_t	lcd;

void lcd_send (int data) {
/* ========================================= */
/* Copied from the examples, almost verbatim */
/* ========================================= */
	static const int gpio_hi [2] = { 0x0002, 0x0001 },
			 gpio_lo [8] = {	0x2220, 0x2210,
						0x2120, 0x2110,
						0x1220, 0x1210,
						0x1120, 0x1110 };
	word x, y;

	x = (data & 0xf0);
	y = ((data & 0x0f) << 4);

	/* Write first nibble */
	rg.io.gp20_23_out =
		gpio_hi [((x & 0x80) >> 7)];
	rg.io.gp16_19_out =
		gpio_lo [((x & 0x70) >> 4)];
	fd.io.gp8_11_out.clr10 = 1;
	fd.io.gp8_11_out.set10 = 1;

	/* Write second Nibble */
	rg.io.gp20_23_out =
		gpio_hi [((y & 0x80) >> 7)];
	rg.io.gp16_19_out =
		gpio_lo [((y & 0x70) >> 4)];

	/* set E low */
	rg.io.gp8_11_out =
		IO_GP8_11_OUT_CLR10_MASK;
}

process (lcd_out, byte)

  entry (0)
	if (data [0]) {
		/* Command */
		rg.io.gp8_11_out = IO_GP8_11_OUT_CLR8_MASK;
	} else {
		/* Data */
		rg.io.gp8_11_out = IO_GP8_11_OUT_SET8_MASK;
	}
	rg.io.gp8_11_out = IO_GP8_11_OUT_CLR9_MASK;
	/* Set E high to enable write */
	rg.io.gp8_11_out = IO_GP8_11_OUT_SET10_MASK;
	#if ! ECOG_SIM
	  delay (1, 1);	/* 2 * 0.3 */
	  release;
	#endif
  entry (1)
	/* Second stage - after the delay */
	lcd_send (data [1]);
        #if ! ECOG_SIM
	  delay (data [2], 2);
	  release;
	#endif
  entry (2)
	finish;

endprocess (1)

/* ==================== */
/* The request function */
/* ==================== */
static int ioreq_lcd (int operation, char *buf, int len) {

	int ii, count = 0;

	switch (operation) {

		case WRITE:

			if (len <= 0 || lcd.cmd > 1) {
				/* Seek in progress - return 'busy' status */
				return -1;
			}

			while (len > 0) {
				ii = lcd.in + 1;
				if (ii == LCD_BUF_SIZE)
					ii = 0;
				if (ii == lcd.ou)
					/* The buffer is full */
					break;
				lcd.buf [lcd.in] = *buf++;
				lcd.in = ii;
				count++;
				len--;
			}

			if (lcd.cmd == 0) {
				lcd.cmd = 1;
				iotrigger (LCD, REQUEST);
			}

			return count == 0 ? -1 : count;

		case CONTROL:

			switch (len) {

				case LCD_CNTRL_ERASE:

					if (lcd.cmd)
						return -1;
					lcd.cmd = 3;
					lcd.pos = 0;
					iotrigger (LCD, REQUEST);

					return 1;

				case LCD_CNTRL_POS:

					if (*buf < 0 || *buf > 31)
						syserror (EREQPAR,
							"ioreq_lcd pos");
					if (lcd.cmd)
						return -1;
					lcd.cmd = 2;
					lcd.pos = (word)(*buf);
					iotrigger (LCD, REQUEST);

					return 1;
			}

			syserror (EREQPAR, "ioreq_lcd");

		default:

			syserror (ENOOPER, "ioreq_lcd");
			return 0;
	}
}

process	(lcd_init, lcd_t)
/* =================================================== */
/* This one just initializes the device and terminates */
/* =================================================== */

  entry (0)
	delay (60, 1);	/* 120 * 0.3 msec, or so I hope */
	release;
  entry (1)
	/* =============================================================== */
	/* This is how we do a sequence of commands. Perhaps a macro would */
	/* make it look nicer.                                             */
	/* =============================================================== */
	data->par [0] = 1;	/* Command indicator */
	data->par [1] = 0x28;	/* Command code */
	data->par [2] = 7;	/* Delay */
	call (lcd_out, data->par, 2);
  entry (2)
	data->par [2] = 2;	/* Same command */
	call (lcd_out, data->par, 3);
  entry (3)
	/* ============================================================= */
	/* Same command and delay. I have copied this init sequence from */
	/* the Eval_PCB example. It doesn't seem to make a lot of sense, */
	/* but ...                                                       */
	/* ============================================================= */
	call (lcd_out, data->par, 4);
  entry (4)
	data->par [1] = 0x0c;	/* Same delay */
	call (lcd_out, data->par, 5);
  entry (5)
	data->par [1] = 0x01;	/* Same delay */
	call (lcd_out, data->par, 6);
  entry (6)
	finish;

endprocess (1)

process (lcd_seek, lcd_t)

  entry (0)
	if (data->pos >= 16)
		data->cnt = 40 + (data->pos - 16);
	else
		data->cnt = data->pos;
	data->par [0] = 1;	/* Command */
	data->par [1] = 0x02;	/* Cursor to 0,0 */
	data->par [2] = 1;
	call (lcd_out, data->par, 1);
  entry (1)
	if (data->cnt == 0)
		finish;
	data->cnt--;
	data->par [1] = 0x14;
	call (lcd_out, data->par, 1);

endprocess (1)

process (lcd_driver, lcd_t)

  entry (0)
	call (lcd_init, data, 1);
  entry (1)
	data->cmd = 0;
	iotrigger (LCD, WRITE);
	iotrigger (LCD, CONTROL);
	iowait (LCD, REQUEST, 2);
	release;
  entry (2)
	/* ======================================================== */
	/* We are interrupt-less, so there's no need to worry about */
	/* locks.                                                   */
	/* ======================================================== */
	if (data->cmd == 0) {
		/* Nothing to do */
		iowait (LCD, REQUEST, 2);
		release;
	}

	if (data->cmd == 1) {
		/* Write */
		if (data->pos == 32) {
			/* We are at the end -> ignore the buffer contents */
			data->in = data->ou = 0;
			proceed (1);
		}
		/* Write next character */
		if (data->in == data->ou) {
			/* Nothing to write - this cannot happen */
			proceed (1);
		}
		if (data->pos == 16) {
			/* Switch to the second line */
			call (lcd_seek, data, 3);
		}
		proceed (3);
	} else if (data->cmd == 2) {
		/* Seek */
		call (lcd_seek, data, 1);
	} else {
		/* Clear */
		data->pos = 0;
		data->in = data->ou = 0;
		data->par [0] = 1;	/* Command */
		data->par [1] = 1;
		data->par [2] = 2;
		call (lcd_out, data->par, 1);
	}
  entry (3)
	/* Continue writing */
	data->par [0] = 0;	/* Data */
	data->par [1] = data->buf [data->ou++];
	if (data->ou == LCD_BUF_SIZE)
		data->ou = 0;
	call (lcd_out, data->par, 4);
  entry (4)
	/* Complete */
	data->pos++;
	if (data->ou == data->in)
		/* No more characters */
		proceed (1);
	else
		/* Keep going */
		proceed (2);

endprocess (1)

static void devinit_lcd (int dummy) {

	/* Enable the GPIO outputs. */
	rg.io.gp8_11_out =
		IO_GP8_11_OUT_EN8_MASK |
		IO_GP8_11_OUT_EN9_MASK |
		IO_GP8_11_OUT_EN10_MASK;

	rg.io.gp16_19_out =
		/* IO_GP16_19_OUT_EN16_MASK | */
		IO_GP16_19_OUT_EN17_MASK |
		IO_GP16_19_OUT_EN18_MASK |
		IO_GP16_19_OUT_EN19_MASK;

	rg.io.gp20_23_out =
		IO_GP20_23_OUT_EN20_MASK;

	/* Assumes device structure zeroed out */
	lcd.cmd = 1;	/* Unavailable until initialized */
	adddevfunc (ioreq_lcd, LCD);
	fork (lcd_driver, &lcd);
}

#endif
/* ========= */

/* ============== */
/* End of drivers */
/* ============== */
