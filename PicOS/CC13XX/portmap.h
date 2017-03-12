#ifndef	__pg_portmap_h__
#define	__pg_portmap_h__

#include <stdint.h>
#include <driverlib/gpio.h>

// The ioid (pin number) falls into the RESERVED bits of the value, from where
// it is removed before the value is applied
#define	iocportconfig(ioid,fun,opt)	((opt) | (fun) | ((ioid) << 19))

#endif
