#ifndef __tarp_node_data_h
#define __tarp_node_data_h

// rcv, snd, fwd, |10 10 0 01 1|, flags, rssi_th, ssignal
// param: |level, rte_rec, slack, routing|

#ifdef	__SMURPH__

tarpCtrlType _da (tarp_ctrl);

__STATIC word tarp_cyclingSeq;

nid_t	_da (net_id); 
nid_t	_da (local_host);
nid_t   _da (master_host);

#else	/* The real world */

tarpCtrlType _da (tarp_ctrl) = {0, 0, 0, 0xA3, 0,
	DEFAULT_RSSI_THOLD, YES};

__STATIC word tarp_cyclingSeq = 0;

nid_t	_da (net_id) = 85;
nid_t	_da (local_host) = 97;
nid_t   _da (master_host) = 1;

#endif	/* SMURPH or PicOS */


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

int _da (getSpdCacheSize) ();
int _da (getDdCacheSize) ();
int _da (getDd) (int i, word * host, word * seq);
int _da (getSpd) (int i, word * host, word * hop);
word _da (getDdM) (word * seq);
word _da (getSpdM) (word * hop);

#endif	/* TARP_CACHES_TEST */

void _da (tarp_init) ();
int _da (tarp_rx) (address buffer, int length, int *ses);
int _da (tarp_tx) (address buffer);

word tarp_findInSpd (nid_t host);
Boolean dd_fresh (headerType * buffer);
void upd_spd (headerType * msg);
int check_spd (headerType * msg);
void setHco (headerType * msg);

/*
 * These ones must be provided by the praxis (and must be methods in VUEE)
 */

__VIRTUAL int _da (tr_offset) (headerType*) __ABSTRACT;
__VIRTUAL Boolean _da (msg_isBind) (msg_t) __ABSTRACT;
__VIRTUAL Boolean _da (msg_isTrace) (msg_t) __ABSTRACT;
__VIRTUAL Boolean _da (msg_isMaster) (msg_t) __ABSTRACT;
__VIRTUAL Boolean _da (msg_isNew) (msg_t) __ABSTRACT;
__VIRTUAL void _da (set_master_chg) (void) __ABSTRACT;

#endif	/* __SMURPH__ */

#endif
