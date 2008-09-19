#ifndef __pg_oep_h
#define __pg_oep_h

//+++ "oep.c"

typedef	void (*oep_chf_t) (word, address, word);

extern byte OEP_Status;

#define	OEP_MAXRAWPL	60	// Maximum raw packet length, excluding checksum
#define	OEP_PKHDRLEN	4	// RLID2 + CMD1 + RQN1
#define	OEP_PAYLEN	(OEP_MAXRAWPL - OEP_PKHDRLEN)	// Payload length
#define	OEP_MAXCHUNKS	(OEP_PAYLEN / 2)	// Chunk numbers per packet
#define	OEP_CHUNKLEN	(OEP_PAYLEN - 2)	// 54
#define	OEP_CHUNKSIZE	OEP_PAYLEN

#define	OEP_STATUS_NOINIT	64	// Uninitialized
#define	OEP_STATUS_RCV		65	// Receiving
#define	OEP_STATUS_SND		66	// Sending

// End codes
#define	OEP_STATUS_DONE		1
#define	OEP_STATUS_TMOUT	2

#define	OEP_STATUS_OK		0

// Errors
#define	OEP_STATUS_NOPROC	16	// Fork failed
#define	OEP_STATUS_NOMEM	17	// malloc failed
#define	OEP_STATUS_PARAM	18	// Illegal parameter

extern word OEP_MLID;
extern byte OEP_RQN, OEP_PHY;

#define	OEP_EVENT	((void*)(&OEP_RQN))

Boolean oep_init ();
byte oep_rcv (word nch, oep_chf_t);
byte oep_snd (word ylid, byte yrqn, word nch, oep_chf_t);
byte oep_wait (word);

#define	oep_status()	OEP_Status
#define	oep_busy()	(OEP_Status >= 65)
#define oep_setlid(a)	(OEP_MLID = (word)(a))
#define	oep_getlid()	OEP_MLID
#define	oep_setrqn(a)	(OEP_RQN = (byte)(a))
#define	oep_getrqn()	OEP_RQN
#define	oep_setphy(a)	(OEP_PHY = (byte)(a))
#define	oep_getphy()	((int)OEP_PHY)

void *oep_bookdata (word size);

#endif
