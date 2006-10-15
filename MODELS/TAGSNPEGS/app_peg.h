#ifndef __app_peg_h__
#define __app_peg_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "common.h"

typedef enum {
	noTag, newTag, reportedTag, confirmedTag,
	fadingReportedTag, fadingConfirmedTag, goneTag, sumTag
} tagStateType;

typedef struct tagDataStruct {
	lword	id;
	word	state;
	word	count;
	lword	evTime;
	lword	lastTime;
} tagDataType;

typedef struct appCountStruct {
	word rcv;
	word snd;
	word fwd;
} appCountType;

// waiting room
typedef struct wroomStruct {
	char * buf;
	lword  tstamp;
} wroomType;

#define tag_lim	16

#define OSS_HT 	0
#define OSS_TCL	1
#define oss_fmt	OSS_TCL

// ===========================================================================

#define PEG_STR_LEN 16

typedef struct msgNewStruct {
	headerType      header;
	lword           passwd;
} msgNewType;

#define in_new(buf, field)   (((msgNewType *)(buf))->field)

typedef struct msgNewAckStruct {
	headerType      header;
	word            master;
	word		    hco:8;
	word		    spare:8;
	lword           mtime;
} msgNewAckType;

#define in_newAck(buf, field)   (((msgNewAckType *)(buf))->field)

typedef struct msgMasterStruct {
	headerType      header;
	lword           mtime;
} msgMasterType;

#define in_master(buf, field)   (((msgMasterType *)(buf))->field)

typedef struct msgReportStruct {
	headerType	header;
	lword		tStamp;
	lword		tagId;
	word		state;
	word 		count;
} msgReportType;

#define in_report(buf, field)   (((msgReportType *)(buf))->field)

typedef struct msgReportAckStruct {
	headerType	header;
	lword		tagId;
	word		state;
	word		count; // may be useless here...
} msgReportAckType;

#define in_reportAck(buf, field)   (((msgReportAckType *)(buf))->field)

typedef struct msgFwdStruct {
	headerType      header;
	lword		    target;
	lword           passwd;
} msgFwdType;

#define in_fwd(buf, field)   (((msgFwdType *)(buf))->field)

typedef struct msgAlrmStruct {
	headerType      header;
	word		    level;
	word		    spare;
} msgAlrmType;

#define in_alrm(buf, field)   (((msgAlrmType *)(buf))->field)

typedef struct msgFindTagStruct {
	headerType	header;
	lword		target;
} msgFindTagType;

#define in_findTag(buf, field)   (((msgFindTagType *)(buf))->field)

typedef struct msgSetPegStruct {
	headerType      header;
	word		    level;
	word		    new_id;
	char            str[PEG_STR_LEN];
} msgSetPegType;

#define in_setPeg(buf, field)   (((msgSetPegType *)(buf))->field)

// ===========================================================================

typedef enum {
	msg_null, msg_getTag, msg_getTagAck, msg_setTag, msg_setTagAck,
	msg_rpc, msg_pong, msg_new, msg_newAck, msg_master, msg_report,
	msg_reportAck, msg_fwd, msg_alrm, msg_findTag, msg_setPeg
} msgType;

// ===========================================================================

typedef struct msgGetTagStruct {
	headerType	header;
	lword		passwd;
} msgGetTagType;

#define in_getTag(buf, field)   (((msgGetTagType *)(buf))->field)

typedef struct msgGetTagAckStruct {
	headerType  header;
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
	headerType  header;
	word		level;
	word		flags;
} msgPongType;

#define in_pong(buf, field)   (((msgPongType *)(buf))->field)
#define PONG_RXON 0x0001
#define in_pong_rxon(buf)     (((msgPongType *)(buf))->flags & PONG_RXON)

#endif
