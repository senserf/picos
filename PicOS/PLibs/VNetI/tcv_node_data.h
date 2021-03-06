/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__tcvdata_h__
#define	__tcvdata_h__

/* #define	DUMPQUEUES	1 */

/*
 * Session descriptor pool
 */
__STATIC sesdesc_t	*descriptors [TCV_MAX_DESC];

/*
 * Phys interfaces. An interface registers by calling phys_reg (see below) and
 * providing a pointer to the options function, which is stored in this array.
 */
__STATIC ctrlfun_t	physical [TCV_MAX_PHYS];

/*
 * Phys output queues, each registered interface gets one dedicated output
 * queue.
 */
__STATIC qhead_t	*oqueues [TCV_MAX_PHYS];

/*
 * Physinfo declared when the interface is registered
 */
__STATIC int		physinfo [TCV_MAX_PHYS];

/*
 * Plugin registration table
 */
__STATIC const tcvplug_t *plugins [TCV_MAX_PLUGS];


#ifdef	__SMURPH__
/*
 * Declare all functions as methods
 */
    private:

	void deq (hblock_t *p);
#if TCV_TIMERS
	void deqtm (hblock_t *p);
#endif
#if TCV_HOOKS
	void deqhk (hblock_t *p);
#endif
	void enq (qhead_t *q, hblock_t *p);
#if TCV_LIMIT_RCV || TCV_LIMIT_XMT
	Boolean qmore (qhead_t *q, word lim);
#endif
	int empty (qhead_t *oq, pktqual_t);
	void dispose (hblock_t *p, int dv);
#if DUMP_MEMORY
	void dmpq (qhead_t *q);
#endif
	void rlp (hblock_t *p);
	hblock_t *apb (word size);

    public:

#if TCV_TIMERS
	word runtq ();
#endif
	void    	_da (tcv_endp) (address p);
	int  		_da (tcv_open) (word state, int phy, int plid, ... );
	int 		_da (tcv_close) (word state, int fd);
	int 		_da (tcv_plug) (int ord, const tcvplug_t *pl);
	address 	_da (tcv_rnp) (word state, int fd);
	int 		_da (tcv_qsize) (int fd, int disp);
	int 		_da (tcv_erase) (int fd, int disp);
	address 	_da (tcv_wnps) (word state, int fd, int length,
				Boolean urg);
	int 		_da (tcv_read) (address p, byte *buf, int len);
	int 		_da (tcv_write) (address p, const byte *buf, int len);
	void 		_da (tcv_drop) (address p);
	int 		_da (tcv_control) (int fd, int opt, address arg);

	int 		_da (tcvp_control) (int phy, int opt, address arg);
	void 		_da (tcvp_assign) (address p, int ses);
	void 		_da (tcvp_attach) (address p, int phy);
	address 	_da (tcvp_clone) (address p, int disp);
	void 		_da (tcvp_dispose) (address p, int dsp);
	address 	_da (tcvp_new) (int size, int dsp, int ses);
#if TCV_HOOKS
	void 		_da (tcvp_hook) (address p, address *h);
	void 		_da (tcvp_unhook) (address p);
#endif
#if TCV_TIMERS
	void 		_da (tcvp_settimer) (address p, word del);
	void 		_da (tcvp_cleartimer) (address p);
	friend class	TCVTimerService;
	TCVTimerService *tcv_tservice;
#endif
	int 		_da (tcvphy_reg) (int phy, ctrlfun_t ps, int info);
	int 		_da (tcvphy_rcv) (int phy, address p, int len);
	address 	_da (tcvphy_get) (int phy, int *len);
	address 	_da (tcvphy_top) (int phy);
	void 		_da (tcvphy_end) (address pkt);
	int 		_da (tcvphy_erase) (int phy, pktqual_t qua);

	void		_da (tcv_init) ();

#if DUMP_MEMORY
	void 		_da (tcv_dumpqueues) (void);
#endif

#endif	/* __SMURPH__ (method definitions) */

#if TCV_TIMERS
/*
 * This is the timer queue, and the time when the timer was last set.
 */
__STATIC thead_t tcv_q_tim;

#endif

#endif
