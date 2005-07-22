#ifndef	__pg_chipcon_h
#define	__pg_chipcon_h	1

#include "chipcon_sys.h"

#define	RADIO_DEF_BUF_LEN	32	/* Default buffer length */
/*
 * These values are merely defaults changeable with tcv_control
 */
#define	RADIO_DEF_MNBACKOFF	32	/* Minimum backoff */
#define	RADIO_DEF_XMITSPACE	8	/* Space between xmitted packets */
#define	RADIO_DEF_BSBACKOFF	0xff	/* Randomized component */
#define	RADIO_DEF_PREAMBLE	48	/* Preamble bits */
#define RADIO_DEF_CHECKSUM	1	/* Checksum present */
#define	RADIO_DEF_BITRATE	384	/* This is /100 */
#define	RADIO_DEF_XPOWER	1	/* Default transmit power */

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

#define	gbackoff	do { \
				zzv_rdbk->seed = (zzv_rdbk->seed + 1) * 6789; \
				zzv_rdbk->backoff = zzv_rdbk->delmnbkf + \
					(zzv_rdbk->seed & zzv_rdbk->delbsbkf); \
			} while (0)

#define	irq_start_rcv	do { \
				chp_pdioin; \
				zzv_prmble = 0; \
				zzv_curbit = 0; \
				zzr_length = 0; \
				zzv_istate = IRQ_RPR; \
				zzv_status = GP_INT_RCV; \
			} while (0)

#define	irq_start_xmt	do { \
				LEDI (3, 1); \
				chp_pdioout; \
				zzv_curbit = 0; \
				zzv_prmble = zzv_rdbk->preamble; \
				zzv_istate = IRQ_XPR; \
				zzv_status = GP_INT_XMT; \
			} while (0)

#define receiver_busy	(zzv_istate == IRQ_RCV || zzv_istate == IRQ_RTR)
#define	receiver_active	(zzv_status == GP_INT_RCV)
#define	xmitter_active	(zzv_status == GP_INT_XMT)

void phys_chipcon (int, int, int, int);

extern word	*zzr_buffer, *zzr_buffp, *zzr_buffl, zzr_length,
		*zzx_buffer, *zzx_buffp, *zzx_buffl, zzv_curbit,
		zzv_status, zzv_prmble,	zzv_istate;

#define	rxevent	((word)&zzr_buffer)
#define	txevent	((word)&zzx_buffer)

typedef	struct {

	byte 	rxoff, txoff, chks, hstat, xpower, rssif;
/* TEMPORARY */
	word	rssi;
	word	qevent, physid, statid;

	/* ================== */
	/* Control parameters */
	/* ================== */
	word 	delmnbkf,		// Minimum backoff
		delbsbkf,		// Backof mask for random component
		delxmspc,		// Minimum xmit packet space
		seed,			// For the random number generator
		preamble,		// Xmit preamble length in bits
		backoff;		// Calculated backoff for xmitter
} radioblock_t;

extern radioblock_t *zzv_rdbk;

#endif
