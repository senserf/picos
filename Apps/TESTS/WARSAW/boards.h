#ifndef __pg_boards_h__
#define	__pg_boards_h__

// ============================================================================

#ifdef	BOARD_WARSAW

#define EPR_TEST

#include "storage.h"

#endif

// ============================================================================

#ifdef	BOARD_WARSAW_ILS

#define EPR_TEST

#include "storage.h"

#endif

// ============================================================================

#ifdef	BOARD_WARSAW_NEW

#define	LCD_TEST
#define	RTC_TEST
#define	RTC_REG
#define	SDC_TEST
#define EPR_TEST
#define	GPS_TEST

#include "rtc_s35390.h"
#include "lcd_st7036.h"
#include "storage.h"

#endif

// ============================================================================

#ifdef	BOARD_WARSAW_NEW_REVISED

#define	LCD_TEST
#define	RTC_TEST
#define	RTC_REG
#define	SDC_TEST
#define EPR_TEST
#define	GPS_TEST

#include "rtc_s35390.h"
#include "lcd_st7036.h"
#include "storage.h"

#endif

// ============================================================================

#ifdef	BOARD_ARTURO_AGGREGATOR

#define	SDC_TEST
#define EPR_TEST

#include "storage.h"

#endif

// ============================================================================

#ifdef	BOARD_ARTURO

#define EPR_TEST

#include "storage.h"

#endif

// ============================================================================

#ifdef	BOARD_ARTURO_PMTH

#define EPR_TEST

#include "storage.h"

#endif

// ============================================================================

#ifdef	BOARD_ARTURO_PVR

#define EPR_TEST

#include "storage.h"

#endif

// ============================================================================

#ifdef	BOARD_XCC430

#define	RTC_TEST
#include "rtc_cc430.h"

#endif

// ============================================================================

#ifdef	BOARD_CC430W
//#define	RTC_TEST
#define	EPR_TEST
#include "rtc_cc430.h"
#include "storage.h"

#endif

#ifdef	BOARD_OLIMEX_CCRFLCD
#define	RTC_TEST
#include "rtc_cc430.h"
#include "lcd_ccrf.h"
#define	LCD_TEST
#endif

#ifdef	BOARD_OLIMEX_CCRF
#define	RTC_TEST
#include "rtc_cc430.h"
#endif

#ifdef	BOARD_OLIMEX_CCRF_BMA250
#define	RTC_TEST
#include "rtc_cc430.h"
#endif

#ifdef	BOARD_ALPHATRONICS_PANIC
#define	RTC_TEST
#include "rtc_cc430.h"
#endif

#ifdef	BOARD_OLIMEX_CCRF_EETEST
#define	RTC_TEST
#define	EPR_TEST
#include "rtc_cc430.h"
#include "storage.h"
#endif

#ifdef	BOARD_CC1350_LAUNCHXL
#define	EPR_TEST
#include "storage.h"
#endif

#ifdef	BOARD_CC1350_SENSORTAG
#define	EPR_TEST
#include "storage.h"
#endif

// ============================================================================

#endif
