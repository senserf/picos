#ifndef __pg_oep_h
#define __pg_oep_h

//+++ "oep.cc"

#include "oep_params.h"

// ============================================================================

extern byte OEP_Status, OEP_LastOp;
extern word OEP_MLID;
extern byte OEP_RQN, OEP_PHY;

// ============================================================================

Boolean oep_init ();
byte oep_rcv (word nch, oep_chf_t);
byte oep_snd (word ylid, byte yrqn, word nch, oep_chf_t);
byte oep_wait (word);
void *oep_bookdata (word size);

#define	oep_status()	OEP_Status
#define	oep_busy()	(OEP_Status >= 65)
#define oep_setlid(a)	(OEP_MLID = (word)(a))
#define	oep_getlid()	OEP_MLID
#define	oep_setrqn(a)	(OEP_RQN = (byte)(a))
#define	oep_getrqn()	OEP_RQN
#define	oep_setphy(a)	(OEP_PHY = (byte)(a))
#define	oep_getphy()	((int)OEP_PHY)
#define oep_lastop()	OEP_LastOp

#define	OEP_EVENT	((void*)(&OEP_RQN))

#endif
