#include <ecog.h>
#include <ecog1.h>
#include "sysio.h"
/* ============================================================================ */
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

#define	BP_INIT	0
#define	BP_STOP	1

process (__beeper, word)
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

endprocess (1)
