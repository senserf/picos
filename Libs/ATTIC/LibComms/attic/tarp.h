#ifndef __tarp_h
#define __tarp_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvplug.h"

// I'm missing them...
#ifndef true
#define true YES
#endif

#ifndef false
#define false NO
#endif

// options vector (who knows... now nothing make sense but strict Spd)
#define tarp_strictSpdOpt	1
//#define lifoOpt			1<<1
//#define clearColOpt		1<<2

// const, quite wild now
#define ddCacheSize			99
#define spdCacheSize		66
#define tarp_maxHops		10
#define tarp_freshTout		5
#define tarp_level			2
#define tarp_options		0


// types
// (there will be some optimization, later)
typedef struct ddCacheStruct {
  lword		snd;
  word		seq;
  word		spare;
  lword		timeStamp;
} ddCacheType;

typedef struct spdStruct {
  lword		id;
  word		hco;
  word 		spare;
  lword		timeStamp;
} spdEntryType;

typedef struct spdCacheStruct {
  word			last;
  word			free;
  spdEntryType	repos[spdCacheSize];
} spdCacheType;

typedef struct tarpCountStruct {
	word rcv;
	word snd;
	word fwd;
} tarpCountType;

// host ids
extern lword host_id, local_host;

// TARP vars (is a general plugin control data a good idea?)
extern bool tarp_urgent;
extern word tarp_retry;
extern tarpCountType tarp_count;

// caches
extern ddCacheType  * ddCache;
extern spdCacheType * spdCache;


void tarp_init (void);
int tarp_tx (address);
int tarp_rx (address, int);
bool tarp_findInSpd (lword);
word tarp_getHco (lword);
#endif
