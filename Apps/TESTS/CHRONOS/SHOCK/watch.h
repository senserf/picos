#ifndef	__pg_watch_h
#define	__pg_watch_h

#include "sysio.h"
#include "rtc_cc430.h"

#define	MAX_TIME_CALLBACKS	4
#define	MAX_ACC_PACK		10

typedef void (*time_callback_f) ();

void	watch_start ();
void	watch_queue (time_callback_f, Boolean);
void	watch_dequeue (time_callback_f);

extern	rtc_time_t RTC;

//+++ "watch.cc"

#endif
