#ifndef	__pg_pins_sys_in_h
#define	__pg_pins_sys_in_h

//+++ pins_sys_in.c

byte __port_in (word);

#define	__port_in_value(p)  (((__port_in ((p)->poff) & \
					(1 << (p)->pnum)) != 0) ^ (p)->edge)


#endif
