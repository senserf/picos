/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include <ecog.h>
#include <ecog1.h>
#include "sysio.h"

#define	BP_INIT	0
#define	BP_STOP	1

strand (__beeper, address)
/* ================ */
/* Beeps the buzzer */
/* ================ */

#define	tone		(((word)data) & 0xf)
#define	duration	(((((word)data) >> 8) & 0x7f) * 1024)

  entry (BP_INIT)

	fd.ssm.div_sel.pwm1 = 0;
	fd.ssm.tap_sel2.pwm1 = tone;
	rg.tim.pwm1_ld = 2;
	rg.tim.pwm1_val = 1;

	fd.tim.pwm1_cfg.pol = 1;
	fd.tim.pwm1_cfg.sw_reload = 1;
	fd.tim.cmd.pwm1_ld = 1;

	fd.tim.ctrl_en.pwm1_cnt = 1;
	fd.tim.ctrl_en.pwm1_auto_re_ld = 1;
	fd.ssm.rst_clr.pwm1 = 1;
	fd.ssm.clk_en.pwm1 = 1;

	delay (duration, BP_STOP);
	release;

  entry (BP_STOP)

	fd.ssm.rst_set.pwm1 = 1;
	finish;

#undef	tone
#undef	duration

endstrand
