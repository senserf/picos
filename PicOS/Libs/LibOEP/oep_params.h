#ifndef __oep_params_h__
#define	__oep_params_h__

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// These are parameters that are visible to the praxis in the same way
// regardless of the mode (VUEE versus PicOS)

typedef	void (*oep_chf_t) (word, address, word);

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

#endif
