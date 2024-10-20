#ifndef	__pg_rf_h
#define	__pg_rf_h

#include "sysio.h"
#include "cc1350.h"
#include "phys_cc1350.h"

// ============================================================================
// Packet structure:
//	NID (network or group Id) inserted automatically by the PHY, 2 bytes
//	DID (destination Id), 2 bytes
//	SID (sender Id), 2 bytes
//	CMD (packet types), 1 byte
//	PLE (payload length), 1 byte
//	PAY (payload, depending on type, PLE bytes
//	CRC (at the end), 2 bytes, irrelevant to the app
// ============================================================================

// Max (total) packet length
#define	MAX_PACKET_LENGTH		64
// Minimum packet legth (frame around the payload): NID+DID+SID+CMD+PLE+CRC
#define	FRAME_LENGTH			(2+2+2+1+1+2)
// Maximum payload length
#define	MAX_PAYLOAD_LENGTH		(MAX_PACKET_LENGTH - FRAME_LENGTH)
// Access to packet fileds
#define	pkt_nid(a)			(((address)(a))[0])
#define	pkt_did(a)			(((address)(a))[1])
#define	pkt_sid(a)			(((address)(a))[2])
#define	pkt_cmd(a)			(((byte*)(a))[6])
#define	pkt_ple(a)			(((byte*)(a))[7])
#define	pkt_pay(a)			(((byte*)(a))+8)

// The sanity criterion for a received packet
#define	pkt_issane(a)			(tcv_left (a) >= FRAME_LENGTH && \
						pkt_ple (a) >= tcv_left (a) - \
							FRAME_LENGTH)

#define	GROUP_ID			((word)(host_id >> 16))
#define	NODE_ID				((word)host_id)

// ============================================================================
// Packet types
// ============================================================================

#define	PKT_RUN				0x01
#define	PKT_STOP			0x02
#define	PKT_REPORT			0x01

// Report is the same type as Run; they can be because they go in opposite
// directions

#endif
