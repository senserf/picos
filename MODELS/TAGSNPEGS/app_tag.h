#ifndef __app_tag_h
#define __app_tag_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "common.h"

typedef struct appCountStruct {
	word rcv;
	word snd;
	word fwd;
} appCountType;

typedef struct pongParamsStruct {
	word freq_maj;
	word freq_min;
	word pow_levels;
	word rx_span;
	word rx_lev;
} pongParamsType;

typedef struct msgGetTagStruct {
	headerType	header;
	lword		passwd;
} msgGetTagType;
#define in_getTag(buf, field)   (((msgGetTagType *)(buf))->field)

typedef struct msgGetTagAckStruct {
	headerType      header;
	lword		ltime;
	lword		host_id;
	nid_t 		tag;
	word		pow_levels;
	word		freq_maj;
	word		freq_min;
	word		rx_span;
	word		ackSeq:8;
	word		spare:8;
} msgGetTagAckType;
#define in_getTagAck(buf, field)   (((msgGetTagAckType *)(buf))->field)

typedef struct msgSetTagStruct {
	headerType	header;
	lword		passwd;
	lword		npasswd;
	word		node_addr;
	word		pow_levels;
	word		freq_maj;
	word		freq_min;
	word		rx_span;
} msgSetTagType;
#define in_setTag(buf, field)   (((msgSetTagType *)(buf))->field)

typedef struct msgSetTagAckStruct {
	headerType	header;
	nid_t 		tag;
	word		ack:8;
	word		ackSeq:8;
} msgSetTagAckType;
#define in_setTagAck(buf, field)   (((msgSetTagAckType *)(buf))->field)

typedef struct msgRpcStruct {
	headerType      header;
	lword           target;
	lword           passwd;
} msgRpcType;
#define in_rpc(buf, field)   (((msgRpcType *)(buf))->field)

typedef struct msgPongStruct {
	headerType      header;
	word		level;
	word		flags;
} msgPongType;

#define in_pong(buf, field)   (((msgPongType *)(buf))->field)
#define PONG_RXON 0x0001
#define in_pong_rxon(buf)     (((msgPongType *)(buf))->flags & PONG_RXON)

typedef enum {
	msg_null, msg_getTag, msg_getTagAck, msg_setTag, msg_setTagAck,
	msg_rpc, msg_pong, msg_new, msg_newAck, msg_master
} msgType;

#endif
