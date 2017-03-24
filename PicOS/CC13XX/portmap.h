#ifndef	__pg_portmap_h__
#define	__pg_portmap_h__

#include <stdint.h>
#include <driverlib/gpio.h>

// The ioid (pin number) falls into the RESERVED bits of the value, from where
// it is removed before the value is applied; so do out [0|1 - output] and
// val [0|1 - initial output value]
#define	iocportconfig(ioid,fun,opt,out,val) \
		((opt) | (fun) | ((ioid) << 19) | ((out) << 7) | ((val) << 6))

#endif
