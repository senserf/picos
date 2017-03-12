#ifndef	__pg_pins_sys_out_h
#define	__pg_pins_sys_out_h

#define	__port_out_value(p,v)	do { \
		if ((v) ^ (p)->edge) \
			GPIO_setDio ((p)->pnum); \
		else \
			GPIO_clearDio ((p)->pnum); \
	} while (0)
#endif
