#include "kernel.h"
#include "pwm_driver.h"
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#if !defined(pwm_output_on) || !defined(pwm_output_off)
#error "constants pwm_output_on and pwm_output_off must be defined for pwm_driver to work!!!"
#endif

static word Width = 1024, Pulse = 0;

#define	PWM_D_ON	0
#define	PWM_D_OFF	1

thread (__pi_pwm_driver)

	entry (PWM_D_ON)

		if (Pulse) {
			pwm_output_on;
			delay (Pulse, PWM_D_OFF);
			release;
		}

	entry (PWM_D_OFF)

		if (Width > Pulse)
			pwm_output_off;

		delay (Width - Pulse, PWM_D_ON);
endthread

void pwm_driver_start () {

	if (!running (__pi_pwm_driver))
		runthread (__pi_pwm_driver);
}

void pwm_driver_stop () {

	killall (__pi_pwm_driver);
	pwm_output_off;
}

void pwm_driver_write (word st, const byte *junk, address val) {

	Pulse = (*val > Width) ? Width : *val;
}

void pwm_driver_setwidth (word w) {

	Width = w;
}
