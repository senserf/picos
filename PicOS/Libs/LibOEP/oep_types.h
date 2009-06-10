#ifndef __oep_types_h__
#define	__oep_types_h__

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// This file contains the definitions of types (and possibly other stuff)
// that are local to the module, but in the VUEE version must be visible to
// the Node class (because the variables/attributes require them)
//

// ============================================================================

typedef struct {
//
// List of chunks to send (as requested by the recipient)
//
	word	list [OEP_MAXCHUNKS];
	word	p, n, nc;

} oep_croster_t;

// ============================================================================

typedef struct {
//
// The tally of received chunks
//
	word	n,		// Total number of chunks
		f;		// First missing
	byte	cstat [0];	// Bitmap of chunks

} oep_ctally_t;

// ============================================================================

typedef struct {
//
// Receiver structure
//
	word oli;	// Old network ID, must be the first item
	word scr;
	oep_chf_t fun;
	oep_ctally_t cta;

} oep_rcv_data_t;

// ============================================================================

typedef struct {
//
// Sender structure
//
	word oli;	// Old network ID, must be the first item
	oep_croster_t ros;
	word scr, ylid, nch;
	oep_chf_t fun;
	byte yrqn;

} oep_snd_data_t;

#endif
