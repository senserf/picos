#ifndef	__pg_cc1000_h
#define	__pg_cc1000_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "cc1000_sys.h"
#include "rfleds.h"

#ifndef	RADIO_LBT_DELAY
#define	RADIO_LBT_DELAY		8
#endif

#ifndef	RADIO_LBT_MIN_BACKOFF
#define	RADIO_LBT_MIN_BACKOFF	8
#endif

#ifndef	RADIO_LBT_BACKOFF_EXP
#define	RADIO_LBT_BACKOFF_EXP	8
#endif

#ifndef	RADIO_LBT_BACKOFF_RX
#define	RADIO_LBT_BACKOFF_RX	6
#endif

#ifndef	RADIO_DEFAULT_POWER
#define	RADIO_DEFAULT_POWER	1
#endif

#ifndef	RADIO_DEFAULT_BITRATE
#define	RADIO_DEFAULT_BITRATE	38400
#endif

#ifndef RADIO_LBT_THRESHOLD
#define RADIO_LBT_THRESHOLD     50
#endif

#ifndef	CC1000_FREQ
#define	CC1000_FREQ		433
#endif

#define	RADIO_DEF_BUF_LEN	48	/* Default buffer length */
#define	PREAMBLE_LENGTH		48	/* Preamble bits */
#define	MINIMUM_PACKET_LENGTH	8	/* Minimum legitimate packet length */

/*
 * Configuration registers
 */
#define CC1000_MAIN            0x00
#define CC1000_FREQ_2A         0x01
#define CC1000_FREQ_1A         0x02
#define CC1000_FREQ_0A         0x03
#define CC1000_FREQ_2B         0x04
#define CC1000_FREQ_1B         0x05
#define CC1000_FREQ_0B         0x06
#define CC1000_FSEP1           0x07
#define CC1000_FSEP0           0x08
#define CC1000_CURRENT         0x09
#define CC1000_FRONT_END       0x0A
#define CC1000_PA_POW          0x0B
#define CC1000_PLL             0x0C
#define CC1000_LOCK            0x0D
#define CC1000_CAL             0x0E
#define CC1000_MODEM2          0x0F
#define CC1000_MODEM1          0x10
#define CC1000_MODEM0          0x11
#define CC1000_MATCH           0x12
#define CC1000_FSCTRL          0x13
#define CC1000_FSHAPE7         0x14
#define CC1000_FSHAPE6         0x15
#define CC1000_FSHAPE5         0x16
#define CC1000_FSHAPE4         0x17
#define CC1000_FSHAPE3         0x18
#define CC1000_FSHAPE2         0x19
#define CC1000_FSHAPE1         0x1A
#define CC1000_FSDELAY         0x1B
#define CC1000_PRESCALER       0x1C
#define CC1000_TEST6           0x40
#define CC1000_TEST5           0x41
#define CC1000_TEST4           0x42
#define CC1000_TEST3           0x43
#define CC1000_TEST2           0x44
#define CC1000_TEST1           0x45
#define CC1000_TEST0           0x46

#define	HSTAT_SLEEP	0	// Chip status: sleeping
#define	HSTAT_RCV	1	// Receiver enabled
#define	HSTAT_XMT	2	// Transmitter enabled

/*
 * The preamble is an alternating sequence of zeros and ones ending with
 * two ones
 */
#define	RCV_PREAMBLE	0xAAAB

/* States of the IRQ automaton */
#define	IRQ_OFF		0	// Interrupts off
#define	IRQ_XPR		1	// Transmitting preamble
#define	IRQ_XPT		2	// Transmitting preamble trailer
#define	IRQ_XMP		3	// Transmitting the packet
#define	IRQ_XEW		4	// Transmitting end-of-word trailer
#define	IRQ_XMT		5	// Transmitting packet trailer
#define	IRQ_XTE		6	// Terminating the packet
#define	IRQ_RPR		7	// Waiting for preamble
#define	IRQ_RCV		8	// Receiving packet
#define	IRQ_RTR		9	// Receiving end-of-word trailer

// Note: e is a constant, so the condition will be optimized out
#define	gbackoff(e) 	do { if (e) __pi_x_backoff = RADIO_LBT_MIN_BACKOFF + \
				(rnd () & ((1 << (e)) - 1)); } while (0)
//
// Any pin-actions required by the physical radio setup, e.g., antenna switching
//
#ifndef	pins_rf_disable
#define	pins_rf_disable		CNOP
#endif

#ifndef	pins_rf_enable_rcv
#define	pins_rf_enable_rcv	CNOP
#endif

#ifndef	pins_rf_enable_xmt
#define	pins_rf_enable_xmt	CNOP
#endif

#define	start_rcv	do { \
				__pi_v_prmble = 0; \
				__pi_v_curbit = 0; \
				__pi_r_length = 0; \
				__pi_v_istate = IRQ_RPR; \
				__pi_v_status = GP_INT_RCV; \
			} while (0)

#define	start_xmt	do { \
				LEDI (1, 1); \
				__pi_v_curbit = 0; \
				__pi_v_prmble = PREAMBLE_LENGTH; \
				__pi_v_istate = IRQ_XPR; \
				__pi_v_status = GP_INT_XMT; \
			} while (0)

#define	end_rcv		CNOP

#define receiver_busy	(__pi_v_istate == IRQ_RCV || __pi_v_istate == IRQ_RTR)
#define	receiver_active	(__pi_v_status == GP_INT_RCV)
#define	xmitter_active	(__pi_v_status == GP_INT_XMT)

extern word	*__pi_r_buffer, *__pi_r_buffp, *__pi_r_buffl, __pi_r_length,
		*__pi_x_buffer, *__pi_x_buffp, *__pi_x_buffl, __pi_v_curbit,
		__pi_v_status, __pi_v_prmble, __pi_v_istate,
		__pi_v_qevent, __pi_v_physid, __pi_v_statid,
		__pi_x_backoff;

extern byte 	__pi_v_rxoff, __pi_v_txoff, __pi_v_hstat, __pi_x_power;

#define	rxevent	((word)&__pi_r_buffer)
#define	txevent	((word)&__pi_x_buffer)

#endif
