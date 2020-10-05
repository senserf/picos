/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "kernel.h"
#include "adc_sampler_sys.h"

interrupt (ADC12_VECTOR) adc_smp_int () {

	if ((ADC12IFG & ADCS_INT_BIT) != 0) {
		ADC12IFG = 0;

#include "irq_adc_sampler.h"

	}
	RTNI;
}
