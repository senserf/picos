#ifndef	__pg_ccfg_h
#define	__pg_ccfg_h

#include "kernel.h"

#if CC1350_RF
#include "cc1350.h"
#if RADIO_DEFAULT_POWER > 7
#define CCFG_FORCE_VDDR_HH	0x1
#endif
#endif

#endif
