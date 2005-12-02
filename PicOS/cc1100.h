#ifndef	__pg_cc1100_h
#define	__pg_cc1100_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "phys_cc1100.h"
#include "cc1100_sys.h"

#define RADIO_DEF_BUF_LEN       48      /* Default buffer length */

#define	CHANNEL_CLEAR_ON_RSSI	0	/* Only when receiving a packet */
#define	SYSTEM_IDENT		0xAB35	/* Sync word */
#define	STAY_IN_RX		1	/* Remain in RX after reception */
#define	BUSY_TRANSMIT		1	/* Busy-wait transmission */

#define	SKIP_XMT_DELAY		2	/* Milliseconds */
#define	TXEND_POLL_DELAY	4	/* Milliseconds */

// CC2500/CC1100 STROBE, CONTROL AND STATUS REGSITER
#define CCxxx0_IOCFG2       0x00        // GDO2 output pin configuration
#define CCxxx0_IOCFG1       0x01        // GDO1 output pin configuration
#define CCxxx0_IOCFG0       0x02        // GDO0 output pin configuration
#define CCxxx0_FIFOTHR      0x03        // RX FIFO and TX FIFO thresholds
#define CCxxx0_SYNC1        0x04        // Sync word, high byte
#define CCxxx0_SYNC0        0x05        // Sync word, low byte
#define CCxxx0_PKTLEN       0x06        // Packet length
#define CCxxx0_PKTCTRL1     0x07        // Packet automation control
#define CCxxx0_PKTCTRL0     0x08        // Packet automation control
#define CCxxx0_ADDR         0x09        // Device address
#define CCxxx0_CHANNR       0x0A        // Channel number
#define CCxxx0_FSCTRL1      0x0B        // Frequency synthesizer control
#define CCxxx0_FSCTRL0      0x0C        // Frequency synthesizer control
#define CCxxx0_FREQ2        0x0D        // Frequency control word, high byte
#define CCxxx0_FREQ1        0x0E        // Frequency control word, middle byte
#define CCxxx0_FREQ0        0x0F        // Frequency control word, low byte
#define CCxxx0_MDMCFG4      0x10        // Modem configuration
#define CCxxx0_MDMCFG3      0x11        // Modem configuration
#define CCxxx0_MDMCFG2      0x12        // Modem configuration
#define CCxxx0_MDMCFG1      0x13        // Modem configuration
#define CCxxx0_MDMCFG0      0x14        // Modem configuration
#define CCxxx0_DEVIATN      0x15        // Modem deviation setting
#define CCxxx0_MCSM2        0x16        // MRC State Machine configuration
#define CCxxx0_MCSM1        0x17        // MRC State Machine configuration
#define CCxxx0_MCSM0        0x18        // MRC State Machine configuration
#define CCxxx0_FOCCFG       0x19        // Freq Off Compensation configuration
#define CCxxx0_BSCFG        0x1A        // Bit Synchronization configuration
#define CCxxx0_AGCCTRL2     0x1B        // AGC control
#define CCxxx0_AGCCTRL1     0x1C        // AGC control
#define CCxxx0_AGCCTRL0     0x1D        // AGC control
#define CCxxx0_WOREVT1      0x1E        // High byte Event 0 timeout
#define CCxxx0_WOREVT0      0x1F        // Low byte Event 0 timeout
#define CCxxx0_WORCTRL      0x20        // Wake On Radio control
#define CCxxx0_FREND1       0x21        // Front end RX configuration
#define CCxxx0_FREND0       0x22        // Front end TX configuration
#define CCxxx0_FSCAL3       0x23        // Frequency synthesizer calibration
#define CCxxx0_FSCAL2       0x24        // Frequency synthesizer calibration
#define CCxxx0_FSCAL1       0x25        // Frequency synthesizer calibration
#define CCxxx0_FSCAL0       0x26        // Frequency synthesizer calibration
#define CCxxx0_RCCTRL1      0x27        // RC oscillator configuration
#define CCxxx0_RCCTRL0      0x28        // RC oscillator configuration
#define CCxxx0_FSTEST       0x29        // Frequency synth calibration control
#define CCxxx0_PTEST        0x2A        // Production test
#define CCxxx0_AGCTEST      0x2B        // AGC test
#define CCxxx0_TEST2        0x2C        // Various test settings
#define CCxxx0_TEST1        0x2D        // Various test settings
#define CCxxx0_TEST0        0x2E        // Various test settings

// Strobe commands
#define CCxxx0_SRES         0x30        // Reset chip.
#define CCxxx0_SFSTXON      0x31        // Enable and calibrate freq synth
#define CCxxx0_SXOFF        0x32        // Turn off crystal oscillator.
#define CCxxx0_SCAL         0x33        // Calibrate freq synth and turn it off
#define CCxxx0_SRX          0x34        // Enable RX
#define CCxxx0_STX          0x35        // Enable TX
#define CCxxx0_SIDLE        0x36        // Exit RX / TX
#define CCxxx0_SAFC         0x37        // Perform AFC adjustment
#define CCxxx0_SWOR         0x38        // Start automatic RX polling sequence
#define CCxxx0_SPWD         0x39        // Enter power down mode
#define CCxxx0_SFRX         0x3A        // Flush the RX FIFO buffer.
#define CCxxx0_SFTX         0x3B        // Flush the TX FIFO buffer.
#define CCxxx0_SWORRST      0x3C        // Reset real time clock.
#define CCxxx0_SNOP         0x3D        // No operation

#define CCxxx0_PARTNUM      (0x30 | 0xC0)
#define CCxxx0_VERSION      (0x31 | 0xC0)
#define CCxxx0_FREQEST      (0x32 | 0xC0)
#define CCxxx0_LQI          (0x33 | 0xC0)
#define CCxxx0_RSSI         (0x34 | 0xC0)
#define CCxxx0_MARCSTATE    (0x35 | 0xC0)
#define CCxxx0_WORTIME1     (0x36 | 0xC0)
#define CCxxx0_WORTIME0     (0x37 | 0xC0)
#define CCxxx0_PKTSTATUS    (0x38 | 0xC0)
#define CCxxx0_VCO_VC_DAC   (0x39 | 0xC0)
#define CCxxx0_TXBYTES      (0x3A | 0xC0)
#define CCxxx0_RXBYTES      (0x3B | 0xC0)

#define CCxxx0_PATABLE      0x3E
#define CCxxx0_TXFIFO       0x3F
#define CCxxx0_RXFIFO       0x3F

// This one is for power control; we set it up separately
#define FREND0		    0x10

#ifdef	DEFINE_RF_SETTINGS

// NOTE: these only work for the 868 MHz chip

const	byte	cc1100_rfsettings [] = {

// These registers have been generated by RF Studio

        CCxxx0_FSCTRL1, 0x0C,   // FSCTRL1
        CCxxx0_FSCTRL0, 0x00,   // FSCTRL0
        CCxxx0_FREQ2,   0x22,   // FREQ2 
        CCxxx0_FREQ1,   0xC4,   // FREQ1
        CCxxx0_FREQ0,   0xEC,   // FREQ0
        CCxxx0_MDMCFG4, 0xC8,   // MDMCFG4
        CCxxx0_MDMCFG3, 0x93,   // MDMCFG3
        CCxxx0_MDMCFG2, 0x03,   // MDMCFG2
        CCxxx0_MDMCFG1, 0xA2,   // MDMCFG1
        CCxxx0_MDMCFG0, 0xF8,   // MDMCFG0
        CCxxx0_DEVIATN, 0x34,   // DEVIATN
        CCxxx0_FREND1,  0x56,   // FREND1 
        CCxxx0_FOCCFG,  0x15,   // FOCCFG
        CCxxx0_BSCFG,   0x6C,   // BSCFG
        CCxxx0_AGCCTRL2,0x83,   // AGCCTRL2
	CCxxx0_AGCCTRL1,0x40,	// RFM spreadsheet
        CCxxx0_AGCCTRL0,0x91,   // AGCCTRL0
        CCxxx0_FSCAL3,  0xA9,   // FSCAL3
        CCxxx0_FSCAL2,  0x2A,   // FSCAL2
	CCxxx0_FSCAL1,	0x00,	// RFM spreadsheet
        CCxxx0_FSCAL0,  0x0D,   // FSCAL0
        CCxxx0_FSTEST,  0x59,   // FSTEST
        CCxxx0_TEST2,   0x86,   // TEST2 
        CCxxx0_TEST1,   0x3D,   // TEST1 
        CCxxx0_TEST0,   0x09,   // TEST0

// These registers I prefer to set manually

	CCxxx0_IOCFG2,	0x06,	// Unused
	CCxxx0_IOCFG1,	0x06,	// Unused
	CCxxx0_IOCFG0,	0x00,	// RX ready - to be used as an interrupt
        CCxxx0_PKTLEN,  0xFF,   // PKTLEN    Packet length.
	CCxxx0_PKTCTRL1,0x04,	// Append 2 status bytes on reception
	CCxxx0_PKTCTRL0,0x45,	// Whitening, CRC, packet length follows sync
        CCxxx0_ADDR,    0x00,   // ADDR      Device address.
        CCxxx0_CHANNR,  0x00,   // CHANNR    Channel number.
	CCxxx0_FIFOTHR,	0x00,	// 4 bytes in the RX FIFO

// Note: CCxxx0_FIFOTHR == 0x01 (the most useful one) doesn't seem to work. It
// forces me to play messy tricks to receive a complete packet without busy
// waiting in the interrupt.

	CCxxx0_SYNC1,	((SYSTEM_IDENT >> 8) & 0xff),
	CCxxx0_SYNC0,	((SYSTEM_IDENT     ) & 0xff),

        CCxxx0_MCSM0,   0x15,   // Recalibrate on exiting IDLE state

#if STAY_IN_RX
// Assume RX after completing RX
#define	MCSM1_FROM_RX	0x0C
#else
// Assume IDLE after completing RX
#define	MCSM1_FROM_RX	0x00
#endif

#if CHANNEL_CLEAR_ON_RSSI
// CCA includes RSSI threshold
#define	MCSM1_CCA	0x30
#else
// Only if currently receiving a packet
#define	MCSM1_CCA	0x20
#endif

	CCxxx0_MCSM1,	(MCSM1_FROM_RX | MCSM1_CCA),

	CCxxx0_MCSM2,	0x07,	// RFM spreadsheet

/* ========== */

        255, 255
};


#define	PATABLE { 0x03, 0x1C, 0x67, 0x60, 0x85, 0xCC, 0xC6, 0xC3 }

// This is a single-value PATABLE at 0dBm
// #define	PATABLE { 0x60 }

#else	/* DEFINE_RF_SETTINGS */

extern const byte cc1100_rfsettings [];

#endif	/* DEFINE_RF_SETTINGS */

/*
 * Chip states
 */
#define CC1100_STATE_MASK			0x70
#define CC1100_FIFO_BYTES_AVAILABLE_MASK	0x0F
#define CC1100_STATE_TX              		0x20
#define CC1100_STATE_TX_UNDERFLOW    		0x70
#define CC1100_STATE_RX              		0x10
#define CC1100_STATE_RX_OVERFLOW     		0x60
#define CC1100_STATE_IDLE            		0x00

// Transitional states
#define	CC1100_STATE_CALIBRATE			0x40
#define	CC1100_STATE_SETTLING			0x50

/*
 * RX state flag
 */
#define	XM_LOCK		0x40
#define	RC_LOCK		0x80
#define	RC_RCVI		0x08


#define	rx_rcnt		(zzr_state & 0x7)

#define	rx_rcnt_upd	do { \
				if ((zzr_state & 0x07) != 0x07) \
					zzr_state += 1; \
			} while (0)

#define	rx_rcnt_res	zzr_state &= 0xf8

#define	rx_rcnt_recalibrate	(rx_rcnt == 0x7)

#define	SPI_START	do { \
				csn_down; \
				while (so_val); \
			} while (0)

#define	SPI_END		csn_up

#define	full_reset	do { \
				sclk_down; \
				csn_up; \
        			UWAIT1; \
				csn_down; \
				UWAIT1; \
				csn_up; \
				UWAIT41; \
				warm_reset; \
			} while (0)

#define	warm_reset	do { \
				SPI_START; \
				cc1100_spi_out (CCxxx0_SRES); \
				while (so_val); \
				SPI_END; \
			} while (0)

#define	RCV_IGN			0x00	 // States of the receiver int. machine
#define	RCV_STA			0x10
#define	RCV_GET			0x20

#define	rcv_istate		(zzr_state & 0x30)

#define	set_rcv_istate(a)	(zzr_state = (zzr_state & 0xcf) | (a))

#define	lock_rcv		(zzr_state |= RC_LOCK)

#define	unlock_rcv		(zzr_state &= ~RC_LOCK)

#define	lock_xmt		(zzr_state |= XM_LOCK)

#define	unlock_xmt		(zzr_state &= ~XM_LOCK)

#define	rcv_restore_int		do { \
					if (rcv_istate != RCV_IGN) \
						rcv_enable_int; \
				} while (0)

#define	unlock_xmt_reset	do { \
					zzr_state &= ~XM_LOCK; \
					if (rcv_istate != RCV_IGN) { \
						set_rcv_istate (RCV_STA); \
						rcv_enable_int; \
					} \
					trigger (rxunlock); \
				} while (0)

#define	xmt_busy		(zzr_state & XM_LOCK)

#define rcv_busy		(zzr_state & RC_LOCK)

#define	gbackoff	do { \
				zzx_seed = (zzx_seed + entropy + 1) * 6789; \
				zzx_backoff = MIN_BACKOFF + \
					(zzx_seed & MSK_BACKOFF); \
			} while (0)

extern word	*zzr_buffer, *zzx_buffer, zzv_qevent, zzv_physid, zzv_statid,
		zzx_seed, zzx_backoff;

extern byte	zzx_paylen, zzr_buffl, zzr_state, zzr_left;

extern byte	*zzr_bptr;

byte cc1100_get_reg (byte);
void cc1100_get_reg_burst (byte, byte*, word);
int cc1100_rx_status ();


#define	rxevent		((word)&zzr_buffer)
#define	rxunlock	((word)&zzr_state)

#endif
