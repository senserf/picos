#ifndef __msg_teehop_h
#define __msg_teehop_h

typedef enum {
	noTag, newTag, reportedTag, confirmedTag,
	fadingReportedTag, fadingConfirmedTag, goneTag
} tagStateType;

typedef enum {
	prxMsg, rtiMsg, reqMsg, sumMsg, specMsg, rtiAck
} msgTypeType; // mo sum, spec yet

typedef enum {
	tagNode, holeNode, masterNode
} nodeTypeType; 

typedef struct tagDataStruct {
	word	id;
	byte	state;
	byte	count;
	long	eventTime;
	long	lastTime;
} tagDataType;

typedef struct prxStruct {
	byte	msgType;
	byte	seqNo;
	word	snd;
} prxType;

#define prxLen sizeof(prxType)

typedef prxType anyType;

typedef struct routedStruct {
	byte	msgType;
	byte	seqNo;
	word	snd;
	word	rcv;
	byte	hoc;     // # of hops so far
	byte	hco;     // last one from the destination to me
} routedType;

// msgType | proxyMask <-> don't route
#define proxyMask 8

typedef struct rtiStruct {
	byte	msgType;
	byte	seqNo;
	word	snd;
	word	rcv;
	byte	hoc;     // # of hops so far
	byte	hco;     // last one from the destination to me
	word	tagId;
	byte	state; // **** na odwrot?
	long	timeStamp; //****
} rtiType;

#define rtiLen sizeof(rtiType)

typedef struct reqStruct {
	byte	msgType;
	byte	seqNo;
	word	snd;
	word	rcv;
	byte	hoc;
	byte	hco;
	word	tagId;      // if for a particular tag
	long	timeStamp;  // maybe we'll sync the nodes
} reqType;

#define reqLen sizeof(reqType)

typedef union msgStruct {
    anyType    anyMsg;
    prxType    prxMsg;
    routedType routedMsg;
    rtiType    rtiMsg;
    reqType    reqMsg;
} msgType;

#define maxMsgLen sizeof(msgType)

#endif
