#ifndef __tarp_node_data_h
#define __tarp_node_data_h

// rcv, snd, fwd, |10 10 001 1|, flags
// param: |level, rte_rec, slack, routing|

#ifdef	__SMURPH__

tarpCtrlType tarp_ctrl;
__STATIC word tarp_cyclingSeq;
#if SPD_RSSI_THRESHOLD
__STATIC bool	strong_signal;
#endif

#else	/* The real world */

tarpCtrlType tarp_ctrl = {0, 0, 0, 0xA3, 0};
__STATIC word tarp_cyclingSeq = 0;
#if SPD_RSSI_THRESHOLD
__STATIC bool	strong_signal = YES;
#endif

#endif	/* __SMURPH__ */

#if SPD_RSSI_THRESHOLD == 0
#define	strong_signal	YES
#endif

/* ================================================================ */


#if TARP_CACHES_MALLOCED

#ifdef __SMURPH__
__STATIC ddcType	* ddCache;
__STATIC spdcType * spdCache;
#else
__STATIC ddcType	* ddCache = NULL;
__STATIC spdcType * spdCache = NULL;
#endif	/* __SMURPH__ */

#else	/* not MALLOCED */

__STATIC ddcType	_ddCache;
__STATIC spdcType _spdCache;

#define ddCache (&_ddCache)
#define spdCache (&_spdCache)

#endif	/* MALLOCED */

#ifdef	__SMURPH__
/*
 * Method headers
 */
#if TARP_CACHES_TEST

int getSpdCacheSize();
int getDdCacheSize();
int getDd(int i, word * host, word * seq);
int getSpd(int i, word * host, word * hop);
word getDdM(word * seq);
word getSpdM(word * hop);

#endif	/* TARP_CACHES_TEST */

word tarp_findInSpd (nid_t host);
void tarp_init();
bool dd_fresh (headerType * buffer);
void upd_spd (headerType * msg);
int check_spd (headerType * msg);
int tarp_rx (address buffer, int length, int *ses);
void setHco (headerType * msg);
int tarp_tx (address buffer);

#endif	/* __SMURPH__ */

#endif
