#ifndef __pg_modsyms_h
#define	__pg_modsyms_h		1

/* ======================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2016                        */
/* All rights reserved.                                                     */
/* ======================================================================== */

/* ======================================================================== */
/*              C O N F I G U R A T I O N    O P T I O N S                  */
/*              ==========================================                  */
/*                                                                          */
/* We make sure that the configuration symbols receive default values, even */
/* if they are not mentioned in options.sys.                                */
/* ======================================================================== */

// As of PG121225A, this file is shared by VUEE. First of all, it is included
// by VUEE's version of sysio.h, so it may define those things that should be
// rightfully shared by the two. Second, the part starting from the first
// occurrence of //-> (at the begining of a line) to #ifndef __SMURPH__ (also
// at the beginning of a line) is parsed by picomp (when running for VUEE) to
// find symbols whose local re-definitions (as determined by the options.sys
// files seen by the praxis) should be passed to VUEE modules. The symbols
// marked by //-> are looked up in the include files requested by the praxis
// for identification of the (optional) modules to be compiled into the
// model.

/* ======================================================================== */

#ifdef	BOARD_TYPE
#define	SYSVER_B	stringify (BOARD_TYPE)
#endif

#define	__sgfy(a)	#a
#define	stringify(a)	__sgfy(a)

// ============================================================================

#define	UART_TCV_MODE_N		0	// Non-persisitent packets
#define	UART_TCV_MODE_P		1	// Built-in ACKs
#define	UART_TCV_MODE_L		2	// Lines
#define	UART_TCV_MODE_E		3	// STX/ETX/DLE with one-byte parity
#define	UART_TCV_MODE_F		4	// STX/ETX/DLE with one-byte checksum

// RADIO_OPTIONS bits (see cc1100.h); I put them in here to make them uniformly
// visible for PicOS as well as VUEE
#define	RADIO_OPTION_RBKF	0x001
#define	RADIO_OPTION_DIAG	0x002
#define	RADIO_OPTION_STATS	0x004
#define	RADIO_OPTION_CCHIP	0x008
#define	RADIO_OPTION_NOCHECKS	0x010
#define	RADIO_OPTION_WORPARAMS	0x020
#define	RADIO_OPTION_REGWRITE	0x040
#define	RADIO_OPTION_ENTROPY	0x080
#define	RADIO_OPTION_PXOPTIONS	0x100

// These are special sequences to identify (for picomp) those symbols that are
// only defined, not set. These symbols represent optional modules.

// ============================================================================

//->VUEE_LIB_PLUG_NULL
//->VUEE_LIB_PLUG_TARP
//->VUEE_LIB_PLUG_BOSS
//->VUEE_LIB_XRS
//->VUEE_LIB_OEP
//->VUEE_LIB_LCDG

// Here is the convention for marking MODSYMS symbols in comments:
//
//	+	- can be redefined, higher value always wins, issue warning
//	++	- can be redefined, higher value always wins, no warning
//
// Nothing, or anything else, means that the symbol cannot be redefined in
// different programs of the same praxis. For now, we need nothing else, we
// shall see if this is enough in the longer run.
//

#ifndef	CODE_LONG_INTS
#define	CODE_LONG_INTS		1	// +
#endif

#ifndef	UART_TCV
#define	UART_TCV		0	// +
#endif

#ifndef	UART_TCV_MODE
#define	UART_TCV_MODE		UART_TCV_MODE_N	// ++
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

#ifndef	CC3000
#define	CC3000			0	// +
#endif

#ifndef	ETHERNET_DRIVER
#define	ETHERNET_DRIVER		0	// +
#endif

#ifndef	RADIO_OPTIONS
#define RADIO_OPTIONS		0
#endif

// RF modules blink LEDs on transmission, reception, and so on ...
#ifndef	RADIO_USE_LEDS
#define	RADIO_USE_LEDS		0	// +
#endif

// LEDs for UART over TCV
#ifndef	UART_USE_LEDS
#define	UART_USE_LEDS		0	// +
#endif

#ifndef	RADIO_CRC_MODE
#define	RADIO_CRC_MODE		0
#endif

#ifndef DUMP_MEMORY
#define	DUMP_MEMORY		0	// +
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

#ifndef	TCV_MAX_DESC
#define	TCV_MAX_DESC		8	// Maximum number of sessions
#endif

#ifndef	TCV_MAX_PHYS
#define	TCV_MAX_PHYS		3	// Maximum number of physical interfaces
#endif

#ifndef	TCV_MAX_PLUGS
#define	TCV_MAX_PLUGS		3	// Maximum number of plugins
#endif

#ifndef	TCV_LIMIT_RCV
#define	TCV_LIMIT_RCV		0	// No limit for RCV queue
#endif

#ifndef	TCV_LIMIT_XMT
#define	TCV_LIMIT_XMT		0	// No limit for OUT queue
#endif

// ============================================================================
// TARP
// ============================================================================

#ifndef	TARP_CACHES_MALLOCED
#define	TARP_CACHES_MALLOCED	0
#endif

#ifndef	TARP_CACHES_TEST
#define	TARP_CACHES_TEST	0
#endif

#ifndef	TARP_DDCACHESIZE
#define	TARP_DDCACHESIZE	20
#endif

#ifndef	TARP_SPDCACHESIZE
#define	TARP_SPDCACHESIZE	20
#endif

#ifndef	TARP_RTRCACHESIZE
#define	TARP_RTRCACHESIZE	10
#endif

#ifndef	TARP_MAXHOPS
#define	TARP_MAXHOPS		10
#endif

#ifndef	TARP_RTR
#define	TARP_RTR		0
#endif

#ifndef	TARP_RTR_TOUT
#define	TARP_RTR_TOUT		1024
#endif

#ifndef	TARP_DEF_RSSITH
#define	TARP_DEF_RSSITH		100
#endif

#ifndef	TARP_DEF_PXOPTS
#define	TARP_DEF_PXOPTS		0x7000
#endif

#ifndef	TARP_DEF_PARAMS
// 				lvrrrwssf
#define	TARP_DEF_PARAMS		0xA3
#endif

// ============================================================================

#ifndef	DIAG_MESSAGES
#define	DIAG_MESSAGES		2		// +
#endif

#ifndef	dbg_level
#define	dbg_level		0		// +
#endif

/* ======================================================================== */
/* End of the part parsed by picomp                                         */
/* ======================================================================== */

#ifndef	__SMURPH__

// This one indicates whether we are running under the simulator (0/1) [eCOG]
#ifndef ECOG_SIM
#define	ECOG_SIM		0
#endif

#ifndef	NESTED_INTERRUPTS
#define	NESTED_INTERRUPTS	0
#endif

#ifndef	SPIN_WHEN_HALTED
#define	SPIN_WHEN_HALTED	0
#endif

// The maximum number of tasks in the system: size of the PCB table; if <= 0,
// the PCBT is linked, i.e,. PCBs are umalloc'ed; if < 0, new PCBs are
// allocated from the front, so new processes get higher priority
#ifndef	MAX_TASKS
#define	MAX_TASKS		0
#endif

#ifdef __ECOG1__
#ifdef	STACK_SIZE
#if STACK_SIZE != 256
#error "S: cannot redefine STACK_SIZE for ECOG1 this way!!"
#endif
#endif
#else
#ifndef	STACK_SIZE
#define	STACK_SIZE		256
#endif
#endif

// Detect stack overrun
#ifndef	STACK_GUARD
#define	STACK_GUARD		0
#endif

// The maximum number of simultaneously awaited events (timer excluded) per task
#ifndef	MAX_EVENTS_PER_TASK
#define	MAX_EVENTS_PER_TASK	4
#endif

// Use switches for PIO input [eCOG]
#ifndef	SWITCHES
#define	SWITCHES		0
#endif

// Indicates whether the UART driver is present at all (0/1/2, 2 == both UARTs,
// i.e., A and B)
#ifndef	UART_DRIVER
#define	UART_DRIVER		0
#endif

// LEDs driver present (0/1)
#ifndef	LEDS_DRIVER
#define	LEDS_DRIVER		0
#endif

#ifndef	LEDS_BLINKING
#define	LEDS_BLINKING		0
#endif

// LCD driver present (0/1) [eCOG]; note: this is obsolete. For new LCD's we
// shall use the lcd_... interface (see sysio.h)
#ifndef	LCD_DRIVER
#define	LCD_DRIVER		0
#endif

/* ======================================================================= */

// Ethernet chip enabled [eCOG]
#ifndef	ETHERNET_DRIVER
#define	ETHERNET_DRIVER		0
#endif

// Indicates whether VNETI (TCV) is present
#ifndef	TCV_PRESENT
#define	TCV_PRESENT		0
#endif

#if TCV_PRESENT
//+++ "tcv.c"
#endif

// Enables 'freeze' (low power hibernation) [MSP430]
#ifndef	GLACIER
#define	GLACIER			0
#endif

// If this is 1, SDRAM is configured in. Dynamic memory is allocated within
// the first 32K page of SDRAM. THe remaining SDRAM is available through
// ramget / ramput. (0/1) [eCOG]
#ifndef	SDRAM_PRESENT
#define	SDRAM_PRESENT		0
#endif

// Configures the ADC interface: 0/1 [eCOG, deprecated]
#ifndef	ADC_PRESENT
#define	ADC_PRESENT		0
#endif

// Use a single memory pool for malloc (may make better sense for tight
// memory boards)
#ifndef	MALLOC_SINGLEPOOL
#define	MALLOC_SINGLEPOOL	0
#endif

// Keep track of free memory
#ifndef	MALLOC_STATS
#define	MALLOC_STATS		0
#endif

// Safe malloc (safer anyway)
#ifndef	MALLOC_SAFE
#define	MALLOC_SAFE		0
#endif

// Doubleword alignment of malloc'ed memory
#ifndef	MALLOC_ALIGN4
#define	MALLOC_ALIGN4		0
#endif

#ifndef	DM2100
#define	DM2100			0
#endif

#ifndef	RF24G
#define	RF24G			0
#endif

#ifndef	RF24L01
#define	RF24L01			0
#endif

/* ========================================================================= */
/* End of RF modules. Note that UART is an honorary RF module, if configured */
/* through VNETI (TCV).                                                      */
/* ========================================================================= */

// CRC calculation for those radio modules that require software CRC. If this
// is zero, we calculate the CRC in a rather naive, but fast, way. 1 selects
// the "official" 16-bit ISO3309 CRC.
#ifndef	CRC_ISO3309
#define CRC_ISO3309		1
#endif

// Entropy collection enabled, if possible
#ifndef	ENTROPY_COLLECTION
#define	ENTROPY_COLLECTION	0
#endif

// rnd () enabled if > 0. If > 1, rnd has a 32-bit long cycle and is of
// considerably better quality, at the expense of speed and one extra word
// of memory.
#ifndef	RANDOM_NUMBER_GENERATOR
#define	RANDOM_NUMBER_GENERATOR	0
#endif

/* =============== */
/* UART parameters */
/* =============== */

#ifndef	UART_RATE
#define	UART_RATE		9600
#endif

#ifndef	UART_RATE_SETTABLE
#define	UART_RATE_SETTABLE	0
#endif

#ifndef UART_INPUT_BUFFER_LENGTH
#define	UART_INPUT_BUFFER_LENGTH	0
#endif

#ifndef	ADC_SAMPLER
#define	ADC_SAMPLER			0
#endif

// ============================================================================

// EEPROM/FLASH drivers =======================================================
#ifndef	STORAGE_M95XXX
#define	STORAGE_M95XXX		0
#endif
#ifndef	STORAGE_AT45XXX
#define	STORAGE_AT45XXX		0
#endif
#ifndef	STORAGE_MT29XXX
#define	STORAGE_MT29XXX		0
#endif
#ifndef	STORAGE_SDCARD
#define	STORAGE_SDCARD		0
#endif
// ============================================================================

// Real Time Clock ============================================================
#ifndef	RTC_S35390
#define	RTC_S35390		0
#endif
// ============================================================================

// Operations for accessing INFO flash on MSP430
#ifndef	INFO_FLASH
#define	INFO_FLASH		0
#endif

// System error resets the micro
#ifndef	RESET_ON_SYSERR
#define	RESET_ON_SYSERR		0
#endif

// Persistent lack of heap memory resets the micro
#ifndef	RESET_ON_MALLOC
#define	RESET_ON_MALLOC		0
#endif

// Enables dmp_mem (). Dump memory to UART on system error.
#ifndef	DUMP_MEMORY
#define	DUMP_MEMORY		0
#endif

#ifndef	dbg_level
#define	dbg_level		0
#endif

#ifndef	emul
#define	emul(a,b,...)		CNOP
#endif

#endif /* !__SMURPH__ */

/* ======================================================================== */
/*        E N D    O F    C O N F I G U R A T I O N     O P T I O N S       */
/* ======================================================================== */

/* ======================================================= */
/* General control options for interfaces and their models */
/* ======================================================= */
#define	PHYSOPT_PLUGINFO	(-1)	/* These two are kind of special */
#define	PHYSOPT_PHYSINFO	(-2)

#define	PHYSOPT_STATUS		0	/* Get device status */
#define	PHYSOPT_TXON		1	/* Transmitter on */
#define	PHYSOPT_TXOFF		2	/* Transmitter off */
#define	PHYSOPT_TXHOLD		3	/* OFF + queue */
#define	PHYSOPT_HOLD		3	/* General hold */
#define	PHYSOPT_RXON		4	/* Receiver on */
#define	PHYSOPT_ON		4	/* General on */
#define	PHYSOPT_RXOFF		5	/* Receiver off */
#define	PHYSOPT_OFF		5	/* General off */
#define	PHYSOPT_CAV		6	/* Set collision avoidance 'vector' */
#define	PHYSOPT_SETPOWER 	7	/* Transmission power */
#define	PHYSOPT_GETPOWER	8	/* Last reception power */
#define	PHYSOPT_ERROR		9	/* Return/clear error code */
#define	PHYSOPT_SETSID		10	/* Set station (network) ID */
#define	PHYSOPT_GETSID		11	/* Return station Id */
#define	PHYSOPT_SENSE		12	/* Return channel status */

#define	PHYSOPT_SETPAYLEN	14	/* Set payload length */
#define	PHYSOPT_GETPAYLEN	15

#define	PHYSOPT_SETGROUP	16	/* Set station group */
#define	PHYSOPT_GETGROUP	17

#define	PHYSOPT_SETCHANNEL	18	/* Set RF channel */
#define	PHYSOPT_GETCHANNEL	19

#define	PHYSOPT_SETMODE		20
#define	PHYSOPT_GETMODE		21

#define	PHYSOPT_SETRATE		22
#define	PHYSOPT_GETRATE		23

#define	PHYSOPT_GETMAXPL	24	/* Get the maximum packet length */

#define	PHYSOPT_RESET		25	/* Reset the radio */
#define	PHYSOPT_SETPARAMS	26	/* Generic request to set misc params */

#define	PHYSOPT_TRANSPARENT	27

/* ========================================== */
/* Registered identifiers of plugins and phys */
/* ========================================== */
#define	INFO_PLUG_NULL		0x0001	/* NULL plugin */
#define INFO_PLUG_TARP          0x0002  /* TARP plugin */
#define	INFO_PLUG_BOSS		0x00E1

#define	INFO_PHYS_UART    	0x0100	/* Non-persistent UART */
#define	INFO_PHYS_UARTB    	0x1100	/* Non-persistent UART + BlueTooth */
#define	INFO_PHYS_UARTP		0x4100	/* Persistent UART */
#define	INFO_PHYS_UARTL		0x8100	/* Line-mode UART over TCV */
#define	INFO_PHYS_UARTLB	0x9100	/* Line-mode UART + BlueTooth */
#define	INFO_PHYS_ETHER    	0x0200	/* Raw Ethernet */
#define INFO_PHYS_CC1000        0x0400  /* CC1000 radio */
#define	INFO_PHYS_DM2100	0x0500	/* DM2100 */
#define	INFO_PHYS_CC1100	0x0600	/* CC1100 */
#define	INFO_PHYS_DM2200	0x0700  /* VERSA 2 */
#define	INFO_PHYS_RF24L01	0x0800
#define	INFO_PHYS_CC3000	0x0201

#endif 
