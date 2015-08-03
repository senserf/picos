// This one is solely for the simulator and provides the initialization
// statements for Node constructor

	net_fd = net_phys = net_plug = -1;

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
        net_pxopts = 0x7000; // pizda dupa DEF_NET_PXOPTS;
#endif


