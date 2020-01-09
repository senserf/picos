#ifndef __netparams_h
#define	__netparams_h

#include "sysio.h"

#ifndef __SMURPH__

#if ETHERNET_DRIVER

#include "phys_ether.h"

// 7 words, 15 = 14 head + 1 tail bytes (needed), 62 is the shortest frame
#define ether_min_frame		62
#define ether_offset(len)	((len) + 7)
#define ether_ptype		0x6007
#define ether_len(len)		((62 >= ((len) + 15)) ? 62 : ((len) + 15))

#endif

#if UART_TCV
#include "phys_uart.h"
#endif

#if CC1000
#include "cc1000.h"
#include "phys_cc1000.h"
#endif

#if CC1100
#include "cc1100.h"
#include "phys_cc1100.h"
#endif

#if CC2420
#include "cc2420.h"
#include "phys_cc2420.h"
#endif

#if DM2200
#include "dm2200.h"
#include "phys_dm2200.h"
#define	net_get_rssi(pkt,len)	((byte)(((address)(pkt))[((len) >> 1) - 1]))
#endif

#if CC1350_RF
#include "cc1350.h"
#include "phys_cc1350.h"
#endif

#endif	/* if not the simulator */

#if CC1100
#define	NET_MAXPLEN		CC1100_MAXPLEN
#endif

#if CC1350_RF
#define	NET_MAXPLEN		CC1350_MAXPLEN
#endif

#ifndef	NET_MAXPLEN
#define	NET_MAXPLEN		60
#endif

// The formula to determine the physical packet length (to request from VNETI)
// given the payload length (PG)

#if CC1100 || CC2420 || CC1350_RF

// NetId (2) + Entropy (2) + RSSI (2) [do we still need the entropy??]
// 2 - station id    , 2 - entropy, 2 - rssi, even
#define	NET_HEADER_LENGTH	2
#define	NET_TRAILER_LENGTH	4
#define payload_to_phys(len)  (((len) + NET_FRAME_LENGTH + 1) & 0xfffe)
#define	phys_to_payload(len)  ((len) - NET_FRAME_LENGTH)
#else
// 2 - header, 4 - crc, + 3 mod 4 == 0
#define	NET_HEADER_LENGTH	2
#define	NET_TRAILER_LENGTH	4
#define payload_to_phys(len)  (((len) + NET_FRAME_LENGTH + 3) & 0xfffc)
#define	phys_to_payload(len)  ((len) - NET_FRAME_LENGTH)
#endif

#define	NET_FRAME_LENGTH	(NET_HEADER_LENGTH + NET_TRAILER_LENGTH)
#define	NET_MAXPAYLEN		(NET_MAXPLEN - NET_FRAME_LENGTH)

#ifndef	net_get_rssi
#define	net_get_rssi(pkt,len)	((byte)(((address)(pkt))[((len) >> 1) - 1] >> 8))
#endif

#endif
