#ifndef __tarp_node_data_h
#define __tarp_node_data_h

// rcv, snd, fwd, |10 10 0 01 1|, flags, rssi_th, ssignal
// param: |level, rte_rec, slack, routing|

#ifdef	__SMURPH__

tarpCtrlType _da (tarp_ctrl);

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
word _da (tarp_pxopts);
#endif

__STATIC word tarp_cyclingSeq;

nid_t	_da (net_id); 
nid_t	_da (local_host);
nid_t   _da (master_host);

#else	/* The real world */

tarpCtrlType _da (tarp_ctrl) = {0, 0, 0, 0xA3, 0,
	DEFAULT_RSSI_THOLD, YES};

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
word _da (tarp_pxopts) = DEF_TARP_PXOPTS;
#endif

__STATIC word tarp_cyclingSeq = 0;

nid_t	_da (net_id) = 85;
nid_t	_da (local_host) = 97;
nid_t   _da (master_host) = 1;

#endif	/* SMURPH or PicOS */


/* ================================================================ */


#if TARP_CACHES_MALLOCED

#ifdef __SMURPH__
__STATIC ddcType	* ddCache;
__STATIC spdcType 	* spdCache;

#if TARP_RTR
__STATIC rtrcType	* rtrCache;
#endif

#else
__STATIC ddcType	* ddCache = NULL;
__STATIC spdcType 	* spdCache = NULL;

#if TARP_RTR
__STATIC rtrcType 	* rtrCache = NULL;
#endif

#endif	/* __SMURPH__ */

#else	/* not MALLOCED */

__STATIC ddcType	_ddCache;
__STATIC spdcType 	_spdCache;

#if TARP_RTR
__STATIC rtrcType	_rtrCache;
#endif

#define ddCache  (&_ddCache)
#define spdCache (&_spdCache)

#if TARP_RTR
#define rtrCache (&_rtrCache)
#endif

#endif	/* MALLOCED */

#ifdef	__SMURPH__
/*
 * Method headers
 */
#if TARP_CACHES_TEST

int _da (getSpdCacheSize) ();
int _da (getDdCacheSize) ();
int _da (getDd) (int i, word * host, word * seq);
int _da (getSpd) (int i, word * host, word * hop);
word _da (getDdM) (word * seq);
word _da (getSpdM) (word * hop);

#endif	/* TARP_CACHES_TEST */

void _da (tarp_init) (void);
int _da (tarp_rx) (address buffer, int length, int *ses);
int _da (tarp_tx) (address buffer);

word findInSpd (nid_t host);

#if TARP_RTR
word findInRtr (nid_t sndr, seq_t seqn, address pkt);
void ackForRtr (headerType * b, int * ses);
int _da (tarp_xmt) (address buffer);
#endif

Boolean dd_fresh (headerType * buffer);
void upd_spd (headerType * msg);
int check_spd (headerType * msg);
void setHco (headerType * msg);

#include "tarp_hooks.h"

#endif	/* __SMURPH__ */

#endif
