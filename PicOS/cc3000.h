#ifndef	__pg_cc3000_h
#define __pg_cc3000_h

#include "cc3000_sys.h"

#ifndef	CC3000_DEBUG
#define	CC3000_DEBUG		0
#endif

#ifndef	WIFI_OPTIONS
#define	WIFI_OPTIONS		0
#endif
//
//	0x01 - TCP (instead of the default UDP)
//	0x02 - interpret NetID as for standard radio (e.g., CC1100)
#define	WIFI_OPTION_TCP		0x01
#define	WIFI_OPTION_NETID	0x02

#ifndef	WIFI_KAL_INTERVAL
// This is in units of CC3000_POLLINT_MIN
#define	WIFI_KAL_INTERVAL	(15 * 16)
#endif

// ============================================================================

#if WIFI_OPTIONS & WIFI_OPTION_TCP
#define	CC3000_SOCKTYPE		CC3000_SOCK_STREAM
#define	CC3000_RCVCMND		CC3000_HCICMD_RCV	// Cmnd to receive
#define	CC3000_EVNTSND		CC3000_HCIEV_SNT	// Packet sent event
#define	CC3000_EVNTRCV		CC3000_HCIEV_RCV	// Packet rcvd event
// Overhead on buffer size needed for reception; the extra number of bytes
// (over the maximum usable packet length) needed to accommodate extra stuff
#define	CC3000_BHDRLEN		30		// Measured as 29
#else
#define	CC3000_SOCKTYPE		CC3000_SOCK_DGRAM
#define	CC3000_RCVCMND		CC3000_HCICMD_RVF
#define	CC3000_EVNTSND		CC3000_HCIEV_STO
#define	CC3000_EVNTRCV		CC3000_HCIEV_RVF
#define	CC3000_BHDRLEN		30		// Measured as 29
#endif

// SPI ops
#define	CC3000_SPIOP_WRITE	0x01
#define	CC3000_SPIOP_READ	0x03

// HCI packet types
#define	CC3000_HCITYPE_CMD	0x01
#define	CC3000_HCITYPE_DATA	0x02
#define	CC3000_HCITYPE_PATCH	0x03
#define	CC3000_HCITYPE_EVENT	0x04

// HCI commands
#define	CC3000_HCICMD_SLS	0x4000		// Simple link start
#define	CC3000_HCICMD_RBS	0x400B		// Read buffer size
#define	CC3000_HCICMD_SCP	0x0004		// Set WLAN connection policy
#define	CC3000_HCICMD_WCN	0x0001		// WLAN connect
#define	CC3000_HCICMD_SOK	0x1001		// Create socket
#define	CC3000_HCICMD_CON	0x1007		// Connect to server
#define	CC3000_HCICMD_SND	0x81		// Send
#define	CC3000_HCICMD_STO	0x83		// Send To
#define	CC3000_HCICMD_RCV	0x1004		// Receive
#define CC3000_HCICMD_VER	0x0207		// Read version from NVRAM
#define CC3000_HCICMD_SEL	0x1008		// Select
#define	CC3000_HCICMD_CLO	0x100B		// Close socket
#define CC3000_HCICMD_RVF	0x100D		// Receive from

// HCI events
#define CC3000_HCIEV_UNSOL_B 	0x8000
#define	CC3000_HCIEV_KEEPALIVE	(CC3000_HCIEV_UNSOL_B + 0x0200)
#define	CC3000_HCIEV_WUCN	(CC3000_HCIEV_UNSOL_B + 0x0001)
#define	CC3000_HCIEV_WUDCN	(CC3000_HCIEV_UNSOL_B + 0x0002)
#define	CC3000_HCIEV_INIT	(CC3000_HCIEV_UNSOL_B + 0x0004)
#define	CC3000_HCIEV_UDHCP	(CC3000_HCIEV_UNSOL_B + 0x0010)
#define	CC3000_HCIEV_CLOSEWAIT	(CC3000_HCIEV_UNSOL_B + 0x0800)

#define	CC3000_HCIEV_FREEBUF	0x4100

#define	CC3000_HCIEV_WCN	CC3000_HCICMD_WCN
#define	CC3000_HCIEV_SOK	CC3000_HCICMD_SOK
#define	CC3000_HCIEV_CON	CC3000_HCICMD_CON
#define	CC3000_HCIEV_RBS	CC3000_HCICMD_RBS
#define	CC3000_HCIEV_RCV	CC3000_HCICMD_RCV
#define	CC3000_HCIEV_SLS	CC3000_HCICMD_SLS
#define	CC3000_HCIEV_SCP 	CC3000_HCICMD_SCP
#define	CC3000_HCIEV_VER 	CC3000_HCICMD_VER
#define	CC3000_HCIEV_SEL 	CC3000_HCICMD_SEL
#define	CC3000_HCIEV_CLO 	CC3000_HCICMD_CLO
#define CC3000_HCIEV_RVF	CC3000_HCICMD_RVF
#define	CC3000_HCIEV_SNT	0x1003		// Packet sent
#define	CC3000_HCIEV_STO	0x100F
#define	CC3000_DATA_RCV		0x85
#define	CC3000_DATA_RVF		0x84

// HCI data packet offsets
#define	CC3000_HCIPO_SD		5
#define	CC3000_HCIPO_NBYTES	(C3000_HCIPO_SD + 4)
#define	CC3000_HCIPO_FLAGS	(C3000_HCIPO_NBYTES + 4)

#define	CC3000_HCIPO_SELSTAT	5
#define	CC3000_HCIPO_SELRD	(CC3000_HCIPO_SELSTAT + 4)
#define	CC3000_HCIPO_SELWR	(CC3000_HCIPO_SELRD + 4)
#define	CC3000_HCIPO_SELEX	(CC3000_HCIPO_SELWR + 4)

// Sockets/domains

#define	CC3000_AF_INET 		2
#define	CC3000_AF_INET6 	23

#define	CC3000_SOCK_STREAM	1
#define	CC3000_SOCK_DGRAM 	2
#define	CC3000_SOCK_RAW   	3 
#define	CC3000_SOCK_RDM		4
#define	CC3000_SOCK_SEQPACKET	5

#define	CC3000_IPPROTO_IP  	0		// dummy for IP
#define	CC3000_IPPROTO_ICMP	1		// control message protocol
#define	CC3000_IPPROTO_IPV4	IPPROTO_IP  	// IP inside IP
#define	CC3000_IPPROTO_TCP 	6		// tcp
#define	CC3000_IPPROTO_UDP 	17          	// user datagram protocol
#define	CC3000_IPPROTO_IPV6	41		// IPv6 in IPv6
#define	CC3000_IPPROTO_NONE	59		// No next header
#define	CC3000_IPPROTO_RAW 	255		// raw IP packet

extern sint cc3000_event_thread;

#define	CC3000_DEFPLEN		128

// Timeouts
#define	CC3000_TIMEOUT_SHORT	(5*1024)
#define	CC3000_TIMEOUT_LONG	(15*1024)

#define	CC3000_DELAY_EVENT	0

// Minimum poll interval (in 1/15.46 of a second)
#ifndef	CC3000_POLLINT_MIN
#define CC3000_POLLINT_MIN	1
#endif
// Maximum poll interval (same units)
#ifndef	CC3000_POLLINT_MAX
#define	CC3000_POLLINT_MAX	15
#endif

#endif
