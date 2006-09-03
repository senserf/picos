#ifndef	__tcvdata_h__
#define	__tcvdata_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

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


#ifdef	__SMURPH__
/*
 * Declare all functions as methods
 */
    private:

	void deq (hblock_t *p);
	void enq (qhead_t *q, hblock_t *p);
#if TCV_LIMIT_RCV || TCV_LIMIT_XMT
	bool qmore (qhead_t *q, word lim);
#endif
	int empty (qhead_t *oq);
	void dispose (hblock_t *p, int dv);
#if TCV_TIMERS
	word runtq ();
#endif
	void rlp (hblock_t *p);
	hblock_t *apb (word size);

    public:

	void tcv_endp (address p);
	int tcv_open (word state, int phy, int plid, ... );
	int tcv_close (word state, int fd);
	void tcv_plug (int ord, const tcvplug_t *pl);
	address tcv_rnp (word state, int fd);
	int tcv_qsize (int fd, int disp);
	int tcv_erase (int fd, int disp);
	address tcv_wnpu (word state, int fd, int length);
	address tcv_wnp (word state, int fd, int length);
	int tcv_read (address p, char *buf, int len);
	int tcv_write (address p, const char *buf, int len);
	void tcv_drop (address p);
	int tcv_left (address p);
	void tcv_urgent (address p);
	bool tcv_isurgent (address p);
	int tcv_control (int fd, int opt, address arg);

	int tcvp_control (int phy, int opt, address arg);
	void tcvp_assign (address p, int ses);
	void tcvp_attach (address p, int phy);
	address tcvp_clone (address p, int disp);
	void tcvp_dispose (address p, int dsp);
	address tcvp_new (int size, int dsp, int ses);
#if TCV_HOOKS
	void tcvp_hook (address p, address *h);
	void tcvp_unhook (address p);
#endif
#if TCV_TIMERS
	void tcvp_settimer (address p, word del);
	void tcvp_cleartimer (address p);
#endif
	int tcvp_length (address p);

	int tcvphy_reg (int phy, ctrlfun_t ps, int info);
	int tcvphy_rcv (int phy, address p, int len);
	address tcvphy_get (int phy, int *len);
	address tcvphy_top (int phy);
	void tcvphy_end (address pkt);
	int tcvphy_erase (int phy);

	void tcv_init ();

	void dmpq (qhead_t *q);
	void tcv_dumpqueues (void);

#endif	/* __SMURPH__ (method definitions) */

#if TCV_TIMERS
/*
 * This is the timer queue, and the time when the timer was last set.
 */
__STATIC thead_t tcv_q_tim;
__STATIC unsigned long tcv_tim_set;

#endif

#endif
