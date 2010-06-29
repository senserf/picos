#ifndef __net_node_data_h
#define __net_node_data_h

#ifdef	__SMURPH__

// We need method headers in here

int _da (net_opt)   (int opt, address arg);
int _da (net_qera)  (int d);
int _da (net_qsize) (int d);
int _da (net_init)  (word phys, word plug);
int _da (net_rx)    (word state, char ** buf_ptr, address rssi_ptr, byte encr);
int _da (net_tx)    (word state, char * buf, int len, byte encr);
int _da (net_close) (word state);

// This one to be provided by the praxis
__VIRTUAL Boolean _da (msg_isClear) (byte) __ABSTRACT;

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
