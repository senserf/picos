#ifndef __pg_modsyms_h__
#define	__pg_modsyms_h__

// Here we keep the symbols that can be (if only in principle) redefined from
// options.sys, or rather "virtual" options.sys as presented to the program by
// picomp. For legacy reasons, we in fact leave a provision for options.sys to
// be included as a regular header file.

// Note: this file is scanned by picomp, so keep it simple. The idea is that 
// picomp will fish for the symbols mentioned here in the various headers
// included by the praxis. Those headers may be coming from module-specific
// headers, like tarp.h, for example, or from the virtual options.sys files,
// e.g., board-specific options. The actual definitions of those symbols will
// be put together into a special virtual header to be included in front of 
// the VUEE glue, i.e., the simulator engine shared by all programs of the
// praxis. Note that conflicts are possible (different definitions of the same
// symbol in different board-specific options). Such conflicts are resolved in
// an ad-hoc manner. They are not supposed to be harmful.

// These are special sequences to mark those symbols that are only defined, not
// set. These symbols represent optional modules

//->VUEE_LIB_PLUG_NULL
//->VUEE_LIB_PLUG_TARP
//->VUEE_LIB_PLUG_BOSS
//->VUEE_LIB_XRS
//->VUEE_LIB_OEP
//->VUEE_LIB_LCDG

#ifndef	CODE_LONG_INTS
#define	CODE_LONG_INTS		1
#endif

#ifndef	UART_TCV
#define	UART_TCV		0
#endif

#ifndef	UART_TCV_MODE
#define	UART_TCV_MODE		UART_TCV_MODE_N
#endif

#ifndef	CC1000
#define	CC1000 			0
#endif

#ifndef	CC1100
#define	CC1100 			0
#endif

#ifndef	DM2200
#define	DM2200 			0
#endif

#ifndef	ETHERNET_DRIVER
#define	ETHERNET_DRIVER		0
#endif

#ifndef	RADIO_OPTIONS
#define RADIO_OPTIONS		0
#endif

#ifndef	RADIO_CRC_MODE
#define	RADIO_CRC_MODE		0
#endif

#ifndef RADIO_LBT_RETRY_LIMIT
#define	RADIO_LBT_RETRY_LIMIT	0
#endif

#ifndef DUMP_MEMORY
#define	DUMP_MEMORY		0
#endif

#ifndef	TCV_LIMIT_RCV
#define	TCV_LIMIT_RCV		0
#endif

#ifndef	TCV_LIMIT_XMT
#define	TCV_LIMIT_XMT		0
#endif

#ifndef	TCV_HOOKS
#define	TCV_HOOKS		0
#endif

#ifndef	TCV_TIMERS
#define	TCV_TIMERS		0
#endif

#ifndef	TCV_OPEN_CAN_BLOCK
#define	TCV_OPEN_CAN_BLOCK	0
#endif

#ifndef	TARP_RTR
#define	TARP_RTR		0
#endif

#endif
