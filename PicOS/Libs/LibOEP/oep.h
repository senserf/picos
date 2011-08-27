#ifndef __pg_oep_h
#define __pg_oep_h

//+++ "oep.c"

#include "oep_params.h"

#ifdef	__SMURPH__
// ============================================================================
// Attribute names of the variables and methods directly accessible by the
// praxis

#define	OEP_Status	_dap (OEP_Status)
#define	OEP_LastOp	_dap (OEP_LastOp)
#define	OEP_MLID	_dap (OEP_MLID)
#define	OEP_RQN		_dap (OEP_RQN)
#define	OEP_PHY		_dap (OEP_PHY)

#else
// ============================================================================

extern byte OEP_Status, OEP_LastOp;
extern word OEP_MLID;
extern byte OEP_RQN, OEP_PHY;

#endif
// ============================================================================

//+++

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
