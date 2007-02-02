/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "adc_sampler_sys.h"

interrupt (ADC12_VECTOR) adc_smp_int () {

	if ((ADC12IFG & ADCS_INT_BIT) == 0)
		return;

	ADC12IFG = 0;

#include "irq_adc_sampler.h"

}
