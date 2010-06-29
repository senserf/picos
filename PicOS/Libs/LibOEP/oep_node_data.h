#ifndef __oep_node_data_h__
#define	__oep_node_data_h__

byte 	OEP_Status 		__sinit (OEP_STATUS_NOINIT),
	OEP_LastOp 		__sinit (0),
	OEP_RQN 		__sinit (1),
	OEP_PHY 		__sinit (0);

word	OEP_MLID;

__STATIC void 	*oep_pdata	__sinit (NULL);

__STATIC address oep_pkt;

__STATIC int 	oep_sid	__sinit (-1);

__STATIC byte	oep_retries,
		oep_plug;
__STATIC byte	oep_sdesc [TCV_MAX_PHYS];

#endif
