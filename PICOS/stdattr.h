#ifndef	__picos_stdattr_h__
#define __picos_stdattr_h__

// ====================================================
// Attribute conversion for praxis programs and plugins
// ====================================================

// ============================================================================

#define	release		sleep
#define	finish		terminate

#define	entropy		_dac (PicOSNode, entropy)
#define	min_backoff	_dac (PicOSNode, min_backoff)
#define	max_backoff	_dac (PicOSNode, max_backoff)
#define	backoff		_dac (PicOSNode, backoff)
#define	lbt_delay	_dac (PicOSNode, lbt_delay)
#define	lbt_threshold	_dac (PicOSNode, lbt_threshold)

#define	phys_dm2200	_dac (PicOSNode, phys_dm2200)
#define	phys_cc1100	_dac (PicOSNode, phys_cc1100)

#define	rnd()		toss (65536)
#define	diag(...)	(  ((PicOSNode*)TheStation)->_na_diag ( __VA_ARGS__)  )

// ============================================================================

#define	reset()		(  ((PicOSNode*)TheStation)->_na_reset ()  )
#define	halt()		(  ((PicOSNode*)TheStation)->_na_halt ()  )
#define	getpid()	(  ((PicOSNode*)TheStation)->_na_getpid ()  )
#define	seconds()	(  ((PicOSNode*)TheStation)->_na_seconds ()  )
#define	actsize(a)	(  ((PicOSNode*)TheStation)->_na_actsize (a)  )
#define	delay(a,b)	(  ((PicOSNode*)TheStation)->_na_delay (a,b)  )
#define	when(a,b)	(  ((PicOSNode*)TheStation)->_na_when (__cpint (a),b)  )
#define io(a,b,c,d,e)	(  ((PicOSNode*)TheStation)->_na_io (a,b,c,d,e)  )
#define encrypt(a,b,c)	(  ((PicOSNode*)TheStation)->_na_encrypt (a,b,c)  )
#define decrypt(a,b,c)	(  ((PicOSNode*)TheStation)->_na_decrypt (a,b,c)  )
#define leds(a,b) \
	(  ((PicOSNode*)TheStation)->_na_leds ((word)(a),(word)(b))  )
#define fastblink(a) \
	(  ((PicOSNode*)TheStation)->_na_fastblink ((Boolean)(a))  )
#define	ldelay(a,b)	(  ((PicOSNode*)TheStation)->_na_ldelay (a,b)  )
#define	lhold(a,b)	(  ((PicOSNode*)TheStation)->_na_lhold (a,b)  )

// ============================================================================

#define vform(a,b,c)	(  ((PicOSNode*)TheStation)->_na_vform (a,b,c)  )
#define vscan(a,b,c)	(  ((PicOSNode*)TheStation)->_na_vscan (a,b,c)  )
#define form(a, ...) \
	(  ((PicOSNode*)TheStation)->_na_form (a, ## __VA_ARGS__)  )
#define scan(a, ...) \
	(  ((PicOSNode*)TheStation)->_na_scan (a, ## __VA_ARGS__)  )
#define ser_out(a,b)	(  ((PicOSNode*)TheStation)->_na_ser_out (a,b)  )
#define ser_outb(a,b)   (  ((PicOSNode*)TheStation)->_na_ser_outb (a,b)  )
#define ser_in(a,b,c)	(  ((PicOSNode*)TheStation)->_na_ser_in (a,b,c)  )
#define ser_outf(a, ...) \
	(  ((PicOSNode*)TheStation)->_na_ser_outf (a, ## __VA_ARGS__)  )
#define ser_inf(a, ...) \
	(  ((PicOSNode*)TheStation)->_na_ser_inf (a, ## __VA_ARGS__)  )

// ============================================================================

#define pin_read(a)	(  ((PicOSNode*)TheStation)->_na_pin_read (a)  )
#define pin_write(a,b)	(  ((PicOSNode*)TheStation)->_na_pin_write (a,b)  )
#define pin_read_adc(a,b,c,d) \
	(  ((PicOSNode*)TheStation)->_na_pin_read_adc (a,b,c,d)  )
#define pin_write_dac(a,b,c) \
	(  ((PicOSNode*)TheStation)->_na_pin_write_dac (a,b,c)  )
#define pmon_start_cnt(a,b) \
	(  ((PicOSNode*)TheStation)->_na_pmon_start_cnt (a,b)  )
#define pmon_stop_cnt() (  ((PicOSNode*)TheStation)->_na_pmon_stop_cnt ()  )
#define pmon_set_cmp(a) (  ((PicOSNode*)TheStation)->_na_pmon_set_cmp (a)  )
#define pmon_get_cnt() 	(  ((PicOSNode*)TheStation)->_na_pmon_get_cnt ()  )
#define pmon_get_cmp() 	(  ((PicOSNode*)TheStation)->_na_pmon_get_cmp ()  )
#define pmon_start_not(a) \
	(  ((PicOSNode*)TheStation)->_na_pmon_start_not (a)  )
#define pmon_stop_not() (  ((PicOSNode*)TheStation)->_na_pmon_stop_not ()  )
#define pmon_get_state() \
	(  ((PicOSNode*)TheStation)->_na_pmon_get_state ()  )
#define pmon_pending_not() \
	(  ((PicOSNode*)TheStation)->_na_pmon_pending_not ()  )
#define pmon_pending_cmp() \
	(  ((PicOSNode*)TheStation)->_na_pmon_pending_cmp ()  )
#define pmon_dec_cnt() 	(  ((PicOSNode*)TheStation)->_na_pmon_dec_cnt ()  )
#define pmon_sub_cnt(a) (  ((PicOSNode*)TheStation)->_na_pmon_sub_cnt (a)  )
#define pmon_add_cmp(a) (  ((PicOSNode*)TheStation)->_na_pmon_add_cmp (a)  )

// ============================================================================

#define ee_size(a,b)	(  ((PicOSNode*)TheStation)->_na_ee_size (a,b)  )
#define ee_read(a,b,c)	(  ((PicOSNode*)TheStation)->_na_ee_read (a,b,c)  )
#define ee_write(a,b,c,d) \
	(  ((PicOSNode*)TheStation)->_na_ee_write (a,b,c,d)  )
#define ee_erase(a,b,c) \
	(  ((PicOSNode*)TheStation)->_na_ee_erase (a,b,c)  )
#define ee_sync(a)	(  ((PicOSNode*)TheStation)->_na_ee_sync (a)  )
#define	if_write(a,b)	(  ((PicOSNode*)TheStation)->_na_if_write (a,b)  )
#define	if_read(a)	(  ((PicOSNode*)TheStation)->_na_if_read (a)  )
#define	if_erase(a)	(  ((PicOSNode*)TheStation)->_na_if_erase (a)  )

// ============================================================================

#define net_opt(a,b)	(  ((TNode*)TheStation)->_na_net_opt (a,b)  )
#define net_qera(a)	(  ((TNode*)TheStation)->_na_net_qera (a)  )
#define net_init(a,b)	(  ((TNode*)TheStation)->_na_net_init (a,b)  )
#define net_rx(a,b,c,d)	(  ((TNode*)TheStation)->_na_net_rx (a,b,c,d)  )
#define net_tx(a,b,c,d)	(  ((TNode*)TheStation)->_na_net_tx (a,b,c,d)  )
#define net_close(a)	(  ((TNode*)TheStation)->_na_net_close (a)  )

// ============================================================================

#define	getSpdCacheSize() \
	(  ((TNode*)TheStation)->_na_getSpdCacheSize ()  )
#define	getDdCacheSize() \
	(  ((TNode*)TheStation)->_na_getDdCacheSize ()  )
#define	getDd(a,b,c) \
	(  ((TNode*)TheStation)->_na_getDd (a,b,c)  )
#define	getSpd(a,b,c) \
	(  ((TNode*)TheStation)->_na_getSpd (a,b,c)  )
#define	getDdM(a) \
	(  ((TNode*)TheStation)->_na_getDdM (a)  )
#define	getSpdM(a) \
	(  ((TNode*)TheStation)->_na_getSpdM (a)  )

// ============================================================================

#define	tcv_endp(a) \
	(  ((PicOSNode*)TheStation)->_na_tcv_endp (a)  )
#define	tcv_open(a,b,c, ...) \
	(  ((PicOSNode*)TheStation)->_na_tcv_open (a,b,c, ## __VA_ARGS__)  )
#define	tcv_close(a,b) \
	(  ((PicOSNode*)TheStation)->_na_tcv_close (a,b)  )
#define	tcv_plug(a,b) \
	(  ((PicOSNode*)TheStation)->_na_tcv_plug (a,b)  )
#define	tcv_rnp(a,b) \
	(  ((PicOSNode*)TheStation)->_na_tcv_rnp (a,b)  )
#define	tcv_qsize(a,b) \
	(  ((PicOSNode*)TheStation)->_na_tcv_qsize (a,b)  )
#define	tcv_erase(a,b) \
	(  ((PicOSNode*)TheStation)->_na_tcv_erase (a,b)  )
#define	tcv_wnpu(a,b,c) \
	(  ((PicOSNode*)TheStation)->_na_tcv_wnpu (a,b,c)  )
#define	tcv_wnp(a,b,c) \
	(  ((PicOSNode*)TheStation)->_na_tcv_wnp (a,b,c)  )
#define	tcv_read(a,b,c) \
	(  ((PicOSNode*)TheStation)->_na_tcv_read (a,b,c)  )
#define	tcv_write(a,b,c) \
	(  ((PicOSNode*)TheStation)->_na_tcv_write (a,b,c)  )
#define	tcv_drop(a) \
	(  ((PicOSNode*)TheStation)->_na_tcv_drop (a)  )
#define	tcv_left(a) \
	(  ((PicOSNode*)TheStation)->_na_tcv_left (a)  )
#define	tcv_urgent(a) \
	(  ((PicOSNode*)TheStation)->_na_tcv_urgent (a)  )
#define	tcv_isurgent(a) \
	(  ((PicOSNode*)TheStation)->_na_tcv_isurgent (a)  )
#define	tcv_control(a,b,c) \
	(  ((PicOSNode*)TheStation)->_na_tcv_control (a,b,c)  )

#define	tcvp_control(a,b,c) \
	(  ((PicOSNode*)TheStation)->_na_tcvp_control (a,b,c)  )
#define	tcvp_assign(a,b) \
	(  ((PicOSNode*)TheStation)->_na_tcvp_assign (a,b)  )
#define	tcvp_attach(a,b) \
	(  ((PicOSNode*)TheStation)->_na_tcvp_attach (a,b)  )
#define	tcvp_clone(a,b) \
	(  ((PicOSNode*)TheStation)->_na_tcvp_clone (a,b)  )
#define	tcvp_dispose(a,b) \
	(  ((PicOSNode*)TheStation)->_na_tcvp_dispose (a,b)  )
#define	tcvp_new(a,b,c) \
	(  ((PicOSNode*)TheStation)->_na_tcvp_new (a,b,c)  )
#define	tcvp_hook(a,b) \
	(  ((PicOSNode*)TheStation)->_na_tcvp_new (a,b)  )
#define	tcvp_unhook(a) \
	(  ((PicOSNode*)TheStation)->_na_tcvp_unhook (a)  )
#define	tcvp_settimer(a,b) \
	(  ((PicOSNode*)TheStation)->_na_tcvp_settimer (a,b)  )
#define	tcvp_cleartimer(a) \
	(  ((PicOSNode*)TheStation)->_na_tcvp_cleartimer (a)  )
#define	tcvp_length(a) \
	(  ((PicOSNode*)TheStation)->_na_tcvp_length (a)  )
#define	tcvphy_reg(a,b,c) \
	(  ((PicOSNode*)TheStation)->_na_tcvphy_reg (a,b,c)  )
#define	tcvphy_rcv(a,b,c) \
	(  ((PicOSNode*)TheStation)->_na_tcvphy_rcv (a,b,c)  )
#define	tcvphy_get(a,b) \
	(  ((PicOSNode*)TheStation)->_na_tcvphy_get (a,b)  )
#define	tcvphy_top(a) \
	(  ((PicOSNode*)TheStation)->_na_tcvphy_top (a)  )
#define	tcvphy_end(a) \
	(  ((PicOSNode*)TheStation)->_na_tcvphy_end (a)  )
#define	tcvphy_erase(a) \
	(  ((PicOSNode*)TheStation)->_na_tcvphy_erase (a)  )

#define	tcv_init() \
	(  ((PicOSNode*)TheStation)->_na_tcv_init ()  )
#define	tcv_dumpqueues() \
	(  ((PicOSNode*)TheStation)->_na_tcv_dumpqueues ()  )

// ============================================================================

#define	net_id			_dac (TNode, net_id)
#define	local_host		_dac (TNode, local_host)
#define	master_host		_dac (TNode, master_host)
#define	tarp_ctrl		_dac (TNode, tarp_ctrl)

#define	tr_offset(a) 		(  ((TNode*)TheStation)->_na_tr_offset (a)  )
#define	msg_isBind(a) 		(  ((TNode*)TheStation)->_na_msg_isBind (a)  )
#define	msg_isTrace(a) 		(  ((TNode*)TheStation)->_na_msg_isTrace (a)  )
#define	msg_isMaster(a)		(  ((TNode*)TheStation)->_na_msg_isMaster (a)  )
#define	msg_isNew(a) 		(  ((TNode*)TheStation)->_na_msg_isNew (a)  )
#define	msg_isClear(a) 		(  ((TNode*)TheStation)->_na_msg_isClear (a)  )
#define	set_master_chg()	(  ((TNode*)TheStation)->_na_set_master_chg ()  )

#endif
