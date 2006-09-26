#ifndef	__pg_rf24l01_h
#define	__pg_rf24l01h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "phys_rf24l01.h"
#include "rf24l01_sys.h"
#include "rfleds.h"

// Register addresses
#define	REG_CONFIG		0	// Basic configuration
#define	REG_CONFIG_MASK_RX_DR	0x40	// RX Data Ready mask
#define	REG_CONFIG_MASK_TX_DS	0x20	// TX Data Sent mask
#define	REG_CONFIG_MASK_MAC_RT	0x10	// Retransmission count mask
#define	REG_CONFIG_EN_CRC	0x08	// CRC enable
#define	REG_CONFIG_CRC16	0x04	// 16-bit CRC
#define	REG_CONFIG_PWR_UP	0x02	// Power Up
#define	REG_CONFIG_PRIM_RX	0x01	// 1-PRX, 0-PTX
#define	REG_CONFIG_PRIM_TX	0x00	// 1-PRX, 0-PTX

#define	REG_EN_AA		1	// Enhanced ShockBurst enable
#define	REG_EN_AA_P5		0x20	// Enable AutoAck, pipe 5
#define	REG_EN_AA_P4		0x10	// Enable AutoAck, pipe 4
#define	REG_EN_AA_P3		0x08	// Enable AutoAck, pipe 3
#define	REG_EN_AA_P2		0x04	// Enable AutoAck, pipe 2
#define	REG_EN_AA_P1		0x02	// Enable AutoAck, pipe 1
#define	REG_EN_AA_P0		0x01	// Enable AutoAck, pipe 0

#define	REG_RXADDR		2	// RX address enable
#define	REG_RXADDR_P5		0x20	// Enable pipe 5
#define	REG_RXADDR_P4		0x10	// Enable pipe 4
#define	REG_RXADDR_P3		0x08	// Enable pipe 3
#define	REG_RXADDR_P2		0x04	// Enable pipe 2
#define	REG_RXADDR_P1		0x02	// Enable pipe 1
#define	REG_RXADDR_P0		0x01	// Enable pipe 0

#define	REG_SETUP_AW		3	// Setup address width
#define	REG_SETUP_AW_3B		1	// 3 bytes
#define	REG_SETUP_AW_4B		2	// 4 bytes
#define	REG_SETUP_AW_5B		3	// 5 bytes

#define	REG_SETUP_RETR		4	// Automatic retransmission
#define	REG_SETUP_RETR_ARD	4	// Shift count for retransmit delay
#define	REG_SETUP_RETR_ARC	0	// Shift count for retransmit count
#define	REG_SETUP_RETR_DISABLE	0	// This value disables auto retr

#define	REG_RF_CH		5	// Channel (0-127)

#define	REG_RF_SETUP		6	// RF parameters
#define	REG_RF_SETUP_2MBPS	0x08	// Select 2Mbps data rate
#define	REG_RF_SETUP_1MBPS	0x00
#define	REG_RF_SETUP_PWR	1	// Shift count for power (0-3)
#define	REG_RF_SETUP_LNA_HCURR	0x01	// LNA gain

#define	REG_STATUS		7	// Status register
#define	REG_STATUS_RX_DR	0x40	// RX Data Ready (write 1 to clear)
#define	REG_STATUS_TX_DS	0x20	// TX Data Sent (write 1 to clear)
#define	REG_STATUS_MAX_RT	0x10	// Retries int (write 1 to clear)
#define	REG_STATUS_RX_P_NO	1	// Shift count: data pipe number
#define	REG_STATUS_TX_FULL	0x01	// TX FIFO full

#define	REG_OBSERVE_TX		8	// Transmit monitor
#define	REG_OBSERVE_TX_PLOST	4	// Shift count: lost packets (0-15)
#define	REG_OBSERVE_TX_ARC	0	// Shift count: resent count (0-15)

#define	REG_CD			9	// Carrier Detect (LSB)
#define	REG_CD_CARRIER		0x01	// The flag (the rest is zero)

#define	REG_RX_ADDR_P0		10	// RX address for pipe 0 (5 bytes)
#define	REG_RX_ADDR_P1		11	// RX address for pipe 1 (5 bytes)
#define	REG_RX_ADDR_P2		12	// RX address tag for pipe 2 (1 byte)
#define	REG_RX_ADDR_P3		13	// RX address tag for pipe 3 (1 byte)
#define	REG_RX_ADDR_P4		14	// RX address tag for pipe 4 (1 byte)
#define	REG_RX_ADDR_P5		15	// RX address tag for pipe 5 (1 byte)

#define	REG_TX_ADDR		16	// Transmit address for PTX (5 bytes)

#define	REG_RX_PW_P0		17	// Number of bytes in RX pipe 0 (1-32)
#define	REG_RX_PW_P1		18	// Number of bytes in RX pipe 1 (1-32)
#define	REG_RX_PW_P2		19	// Number of bytes in RX pipe 2 (1-32)
#define	REG_RX_PW_P3		20	// Number of bytes in RX pipe 3 (1-32)
#define	REG_RX_PW_P4		21	// Number of bytes in RX pipe 4 (1-32)
#define	REG_RX_PW_P5		22	// Number of bytes in RX pipe 5 (1-32)

#define	REG_FIFO_STATUS		23	// FIFO status, what else?
#define	REG_FIFO_STATUS_TX_R	0x40	// Reuse last packet in FIFO for TX
#define	REG_FIFO_STATUS_TX_F	0x20	// TX FIFO full (not a byte available)
#define	REG_FIFO_STATUS_TX_E	0x10	// TX FIFO empty
#define	REG_FIFO_STATUS_RX_F	0x02	// RX FIFO full
#define	REG_FIFO_STATUS_RX_E	0x01	// RX FIFO empty

// Commands
#define	CMD_R_REGISTER		0x00	// To be or'red with the reg number
#define	CMD_W_REGISTER		0x20
#define	CMD_R_RX_PAYLOAD	0x61	// Read RX payload
#define	CMD_W_TX_PAYLOAD	0xA0
#define	CMD_FLUSH_TX		0xE1
#define	CMD_FLUSH_RX		0xE2
#define	CMD_REUSE_TX_PAYLOAD	0xE3
#define	CMD_NOP			0xFF
#define	CMD_READ_STATUS		CMD_NOP

#define	RADIO_DEF_XPOWER	1	// Default transmit power
#define	RADIO_MAX_POWER		3
#define	RADIO_DEF_CHANNEL	2
#define	RADIO_MAX_CHANNEL	127
					// These are reversed: LSB -> MSB
#define	MAX_PAYLOAD_LENGTH	32

#define	BACKOFF_AFTER_RECEIVE	1
#define	XMIT_SPACE		10	/* Inter packet space for xmit */

#define	BUSY_WAIT_ETX		1	/* Busy-wait for end of transmission */
#define	MULTIPLE_PIPES		1	/* Diff. pipes handle diff. PLength */
#define	PIPE1_LENGTH		7	/* Must be decreasing */
#define	PIPE2_LENGTH		13
#define	PIPE3_LENGTH		19
#define	PIPE4_LENGTH		25
#define	PIPE5_LENGTH		31

#define	LSB_NETADDR		0x01	/* LSB of pipe address */

#if MULTIPLE_PIPES

#define	RXADDR_VAL 	      (	REG_RXADDR_P1 | \
				REG_RXADDR_P2 | \
				REG_RXADDR_P3 | \
				REG_RXADDR_P4 | \
				REG_RXADDR_P5 )

#define	PW_SETUP		REG_RX_PW_P1,   PIPE1_LENGTH,  \
				REG_RX_PW_P2,   PIPE2_LENGTH,  \
				REG_RX_PW_P3,   PIPE3_LENGTH,  \
				REG_RX_PW_P4,   PIPE4_LENGTH,  \
				REG_RX_PW_P5,   PIPE5_LENGTH,  \
				REG_RX_ADDR_P2, LSB_NETADDR+1, \
				REG_RX_ADDR_P3, LSB_NETADDR+2, \
				REG_RX_ADDR_P4, LSB_NETADDR+3, \
				REG_RX_ADDR_P5, LSB_NETADDR+4

#else	/* SINGLE PIPE */

#define	RXADDR_VAL 	      	REG_RXADDR_P1
#define	PW_SETUP		REG_RX_PW_P1, MAX_PAYLOAD_LENGTH-1

#endif	/* MULTIPLE_PIPES */

#define	NETWORK_ADDRESS		{ LSB_NETADDR, 0xD6, 0xA4, 0x59, 0xE1 }

/*
 * Initial values of registers for pre-configuration
 */
#define	RF24L01_PRECONFIG		{ \
    REG_CONFIG, \
	(byte)(REG_CONFIG_MASK_TX_DS + REG_CONFIG_MASK_MAC_RT + \
					REG_CONFIG_EN_CRC + REG_CONFIG_CRC16), \
    REG_EN_AA, \
 	(byte) (0), \
    REG_RXADDR, \
  	RXADDR_VAL, \
    REG_SETUP_AW, \
	(byte) (REG_SETUP_AW_5B), \
    REG_SETUP_RETR, \
	(byte) (REG_SETUP_RETR_DISABLE), \
    REG_RF_CH, \
  	(byte) (RADIO_DEF_CHANNEL), \
    REG_RF_SETUP, \
  	(byte) (REG_RF_SETUP_1MBPS + (RADIO_DEF_XPOWER << REG_RF_SETUP_PWR)), \
    REG_STATUS, \
	(byte) (REG_STATUS_RX_DR + REG_STATUS_TX_DS + REG_STATUS_MAX_RT), \
    REG_OBSERVE_TX, \
  	(byte) (0), \
    PW_SETUP, \
    255 \
}
	
#define	gbackoff	(bckf_timer = MIN_BACKOFF + (rnd () & MSK_BACKOFF))

extern word		zzv_drvprcs, zzv_qevent;

#endif
