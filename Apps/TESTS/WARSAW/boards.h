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

#endif