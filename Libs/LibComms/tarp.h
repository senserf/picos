#ifndef __tarp_h
#define __tarp_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvplug.h"
#include "msg_tarp.h"

//+++ "tarp.c"

// I'm missing them...
#ifndef true
#define true YES
#endif

#ifndef false
#define false NO
#endif

#define ddCacheSize		3
#define spdCacheSize		10
#define tarp_maxHops		10

/*
 C compilter on eCog produces wrong code if chars are mixed with words
 in arrays. In general, we should stay away from chars there. Hopefully,
 other compilers behave, so we could use chars is a more reliable fashion.
 That's why we have seq_t, for example.

 Fillers are for id_t being lword and the rest words -- I just don't trust
 the compiler...
 */

//DD entry
struct dde {
	id_t	host;
	seq_t	seq;
	word	fill;
};

// DD cache struct
typedef struct ddc {
	int  	head;
	seq_t	m_seq;
	struct dde  en[ddCacheSize];
} ddcType;

// SPD entry
struct spde {
	id_t	host;
	hop_t   hop; // MSB - hoc, LSB - attempt count
};

// SPD cache struct
typedef struct spdc {
	int  	head;
	hop_t	m_hop;
	struct	spde en[spdCacheSize];
} spdcType;

typedef struct tarpCountStruct {
	word rcv;
	word snd;
	word fwd;
} tarpCountType;

// flags
#define tarp_retry(r)   (tarp_flags & (r & 0x000F))
#define TARP_URGENT     0x0010
#endif
