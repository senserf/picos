#ifndef __pg_options_h
#define	__pg_options_h		1

/* ======================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                        */
/* All rights reserved.                                                     */
/* ======================================================================== */

/* ======================================================================== */
/*              C O N F I G U R A T I O N    O P T I O N S                  */
/*              ==========================================                  */
/*                                                                          */
/* We make sure that the configuration symbols receive default values, even */
/* if they are not mentioned in options.sys.                                */
/* ======================================================================== */

// The list of known board types =============================================
#define	BOARD_CYAN		0	/* Cyan evaluation board */
#define	BOARD_GEORGE		1

#define	BOARD_DM2100		100	/* DM2100 from RFM */
#define	BOARD_GENESIS		101	/* Genesis board from RFM */
#define	BOARD_VERSA2		102
#define	BOARD_SFU_PROTOTYPE	103	/* Prototype board for Bozena's Lab */
// ===========================================================================

// Default board
#ifndef	TARGET_BOARD
#define	TARGET_BOARD		BOARD_GENESIS
#endif

// This one indicates whether we are running under the simulator (0/1) [eCOG]
#ifndef ECOG_SIM
#define	ECOG_SIM		0
#endif

// Watchdog
#ifndef	WATCHDOG_ENABLED
#define	WATCHDOG_ENABLED	0
#endif

// Prioritizing scheduler
#ifndef	SCHED_PRIO
#define	SCHED_PRIO		0
#endif

// The maximum number of tasks in the system: size of the PCB table
#ifndef	MAX_TASKS
#define	MAX_TASKS		16
#endif

// Detect stack overrun
#ifndef	STACK_GUARD
#define	STACK_GUARD		0
#endif

// The maximum number of simultaneously awaited events (timer excluded) per task
#ifndef	MAX_EVENTS_PER_TASK
#define	MAX_EVENTS_PER_TASK	4
#endif

// This one makes system messages go to UART_A. Also 'diag' becomes enabled
// and useful for debugging (0/1). If DIAG_MESSAGES > 1, system error
// messages are verbose. If DIAG_MESSAGES > 2, diag messages go to EEPROM
// (which must be present and handled).
#ifndef	DIAG_MESSAGES
#define	DIAG_MESSAGES		2
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

// LCD driver present (0/1) [eCOG]
#ifndef	LCD_DRIVER
#define	LCD_DRIVER		0
#endif

/* ====================================================================== */
/* These are old and unsupported.  They were used in the old days when RF */
/* modules were formal devices. We don't do this any more. RF modules are */
/* handled by TCV (VNETI) these days.                                     */
/* ====================================================================== */

// RADIO driver present (0/1)
#ifndef	RADIO_DRIVER
#define	RADIO_DRIVER		0
#endif
// radio options
#define	RADIO_NONE              0
#define	RADIO_RFMI		2
#define	RADIO_XEMICS		3
// Select the radio type in options.sys if present
#ifndef RADIO_TYPE
#define RADIO_TYPE              RADIO_NONE
#endif
// If this is nonzero, the radio driver is assisted by a helper simulating
// interrupts (triggering events) when an activity is sensed
#define	RADIO_INTERRUPTS	4

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
// These select TCV configuration options
#ifndef	TCV_TIMERS
#define	TCV_TIMERS		0	// 0/1
#endif
#ifndef	TCV_HOOKS
#define	TCV_HOOKS		0	// 0/1
#endif
#ifndef	TCV_MAX_DESC
#define	TCV_MAX_DESC		16	// Maximum number of sessions
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

//+++ "tcv.c"

#endif	/* TCV_PRESENT */

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

// Makes long encoding/decoding formats (%ld, %lu, %lx) available in form
// and scan (library functions) (0/1)
#ifndef	CODE_LONG_INTS
#define	CODE_LONG_INTS		0
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

/* ========================================================================= */
/* Radio module selection: note that XEMICS and RFMI are not here. They have */
/* not been attached to MSP430 yet, and they operate as "devices", which way */
/* is considered deprecated. They still do work with eCOG, and we will do it */
/* when (and if) we interface them to MSP430.                                */
/* ========================================================================= */

#ifndef CC1000
#define CC1000                 	0
#endif

#ifndef CC1100
#define CC1100                 	0
#endif

#ifndef	DM2100
#define	DM2100			0
#endif

#ifndef	DM2200
#define	DM2200			0
#endif

#ifndef	RF24G
#define	RF24G			0
#endif

#ifndef	RF24L01
#define	RF24L01			0
#endif

#ifndef	UART_TCV
#define	UART_TCV		0
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

#ifndef	LBT_DELAY
// Listen Before Transmit delay (0 disables)
#define	LBT_DELAY		8
#endif

#ifndef	LBT_THRESHOLD
// Listen Before Transmit RSSI threshold (quantized into levels 0-16)
#define	LBT_THRESHOLD		2
#endif

#ifndef	MIN_BACKOFF
// Minimum backoff of the transmitter (whenever it concludes that a transmission
// would not be appropriate at the moment)
#define	MIN_BACKOFF		8
#endif

#ifndef	MSK_BACKOFF
// Backoff mask for the randomized component. Random backoff is generated as
// MIN_BACKOFF + (random & MSK_BACKOFF)
#define	MSK_BACKOFF		0xff
#endif

// RF modules blink LEDs on transmission, reception, and so on ...
#ifndef	RADIO_USE_LEDS
#define	RADIO_USE_LEDS		0
#endif

#if	CC1000
#ifndef	CC1000_FREQ
// Default CC1000 frequency. 868 is another option.
#define	CC1000_FREQ		433
#endif
#endif

// Pulse monitor function enabled (see Libs/Lib/pin_read.c)
#ifndef	PULSE_MONITOR
#define	PULSE_MONITOR		0
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

#ifndef	UART_PARITY
#define	UART_PARITY		0	// 0-even, 1-odd
#endif

#ifndef	UART_BITS
#define	UART_BITS		8
#endif

#ifndef	UART_INPUT_FLOW_CONTROL
#define	UART_INPUT_FLOW_CONTROL		0
#endif

#ifndef	UART_OUTPUT_FLOW_CONTROL
#define	UART_OUTPUT_FLOW_CONTROL	0
#endif

#ifndef UART_INPUT_BUFFER_LENGTH
#define	UART_INPUT_BUFFER_LENGTH	0
#endif

// EEPROM operations available (see eeprom.c)
#ifndef	EEPROM_DRIVER
#define	EEPROM_DRIVER		0
#endif

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

// Selects FCC test mode (continuous transmission) of RF modules. Don't set it!
#ifndef	FCC_TEST_MODE
#define	FCC_TEST_MODE		0
#endif

// Enables dmp_mem (). Dump memory to UART on system error.
#ifndef	DUMP_MEMORY
#define	DUMP_MEMORY		0
#endif

/* ======================================================================== */
/*        E N D    O F    C O N F I G U R A T I O N     O P T I O N S       */
/* ======================================================================== */

#endif 