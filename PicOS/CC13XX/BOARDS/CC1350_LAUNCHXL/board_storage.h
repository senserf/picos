/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "pins.h"
#include "storage_mx25r8035.h"

#define	ee_bring_up	CNOP
#define	ee_bring_down	CNOP

#if 0
#define	ee_start	do { \
				GPIO_setOutputEnableDio (IOID_20,\
					GPIO_OUTPUT_ENABLE); \
				udelay (2); \
			} while (0)
#endif

#define	ee_start	GPIO_setOutputEnableDio (IOID_20, GPIO_OUTPUT_ENABLE)

#define	ee_stop		GPIO_setOutputEnableDio (IOID_20, GPIO_OUTPUT_DISABLE)

#define	ee_clkh		GPIO_setDio (IOID_10)
#define	ee_clkl		GPIO_clearDio (IOID_10)

#define	ee_outh		GPIO_setDio (IOID_9)
#define	ee_outl		GPIO_clearDio (IOID_9)

#define	ee_inp		GPIO_readDio (IOID_8)

// This should properly power down the flash chip
#define	EXTRA_INITIALIZERS	do { ee_open (); ee_close (); } while (0)
