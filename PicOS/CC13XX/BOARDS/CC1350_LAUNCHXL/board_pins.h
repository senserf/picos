/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2017                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	IOCPORTS { \
		iocportconfig (IOID_3, IOC_PORT_MCU_UART0_TX, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0), \
		iocportconfig (IOID_2, IOC_PORT_MCU_UART0_RX, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0) \
	}

#define	PIN_LIST { \
		PIN_DEF (IOID_1),  \
		PIN_DEF (IOID_4),  \
		PIN_DEF (IOID_5),  \
		PIN_DEF (IOID_8),  \
		PIN_DEF (IOID_9),  \
		PIN_DEF (IOID_10), \
		PIN_DEF (IOID_11), \
		PIN_DEF (IOID_12), \
		PIN_DEF (IOID_13), \
		PIN_DEF (IOID_14), \
		PIN_DEF (IOID_15), \
		PIN_DEF (IOID_16), \
		PIN_DEF (IOID_17), \
		PIN_DEF (IOID_18), \
		PIN_DEF (IOID_19), \
		PIN_DEF (IOID_20), \
		PIN_DEF (IOID_21), \
		PIN_DEF (IOID_22), \
		PIN_DEF (IOID_23), \
		PIN_DEF (IOID_24), \
		PIN_DEF (IOID_25), \
		PIN_DEF (IOID_26), \
		PIN_DEF (IOID_27), \
		PIN_DEF (IOID_28), \
		PIN_DEF (IOID_29), \
		PIN_DEF (IOID_30)  \
	}

#define	PIN_MAX	26
