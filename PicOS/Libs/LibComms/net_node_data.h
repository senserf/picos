#ifndef __net_node_data_h
#define __net_node_data_h

#ifdef	__SMURPH__

// We need method headers in here
#include "net.h"

#if RADIO_DRIVER
int radio_init (word plug);
#endif
#if CC1000
int cc1000_init (word plug);
#endif
#if CC1100
int cc1100_init (word plug);
#endif
#if DM2200
int dm2200_init (word plug);
#endif
#if ETHERNET_DRIVER
int ether_init (word plug);
#endif
#if UART_TCV
int uart_init (word plug);
#endif

// For the simulator, the initialization can only be done in Node constructor

int net_fd;
int net_phys;
int net_plug;

#else	// In the real world

int net_fd   = -1;
int net_phys = -1;
int net_plug = -1;

#endif	/* __SMURPH__ */

#endif
