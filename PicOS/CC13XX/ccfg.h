#ifndef	__pg_ccfg_h
#define	__pg_ccfg_h

#include "kernel.h"

#if CC1350_RF
#include "cc1350.h"
#if RADIO_DEFAULT_POWER > 7
#define CCFG_FORCE_VDDR_HH	0x1
#endif
#endif

#if USE_FLASH_CACHE
// Disable extra GPRAM
#define	SET_CCFG_SIZE_AND_DIS_FLAGS_DIS_GPRAM 	1
#else
// Enable extra GPRAM
#define	SET_CCFG_SIZE_AND_DIS_FLAGS_DIS_GPRAM 	0
#endif

#endif
