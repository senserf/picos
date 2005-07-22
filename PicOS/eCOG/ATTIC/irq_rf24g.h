#ifndef __pg_irq_rf24g_h
#define __pg_irq_rf24g_h

if (rf24g_int) {
	morning;
	clr_rcv_int;
	zzv_rdbk->rxwait = 0;
	i_trigger (ETYPE_USER, rxevent);
}

#endif
