#ifndef	__pg_pins_sys_in_h
#define	__pg_pins_sys_in_h

//+++ pins_sys_in.c

#define	__port_in_value(p) (GPIO_readDio ((p)->pnum) ^ (p)->edge)

#endif
