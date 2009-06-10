#ifndef __oep_node_data_h__
#define	__oep_node_data_h__

byte 	_da (OEP_Status) 	__sinit (OEP_STATUS_NOINIT),
	_da (OEP_LastOp) 	__sinit (0),
	_da (OEP_RQN) 		__sinit (1),
	_da (OEP_PHY) 		__sinit (0);

word	_da (OEP_MLID);

__STATIC void 	*_da (oep_pdata)	__sinit (NULL);
__STATIC address _da (oep_pkt);

__STATIC int 	_da (oep_sid)	__sinit (-1);

__STATIC byte	_da (oep_retries), _da (oep_plug);
__STATIC byte	_da (oep_sdesc) [TCV_MAX_PHYS];

#endif
