#ifndef	__ab_params_h__
#define	__ab_params_h__

// These can be in principle redefined, this is why the ifndefs

#ifndef	AB_PL_CUR
#define	AB_PL_CUR	0
#endif

#ifndef	AB_PL_EXP
#define	AB_PL_EXP	1
#endif

#ifndef	AB_PL_MAG
#define	AB_PL_MAG	2
#endif

#ifndef	AB_PL_LEN
#define	AB_PL_LEN	3
#endif

#ifndef	AB_PL_PAY
#define	AB_PL_PAY	4
#endif

#ifndef	AB_PMAGIC
#define	AB_PMAGIC	0xAB
#endif

#define	AB_PF_CUR	(((byte*)ab_xrs_pac)[AB_PL_CUR])
#define	AB_PF_EXP	(((byte*)ab_xrs_pac)[AB_PL_EXP])
#define	AB_PF_MAG	(((byte*)ab_xrs_pac)[AB_PL_MAG])
#define	AB_PF_LEN	(((byte*)ab_xrs_pac)[AB_PL_LEN])
#define	AB_PF_PAY	(((byte*)ab_xrs_pac)+AB_PL_PAY )

#ifndef	AB_MINPL
#define	AB_MINPL	(4+2)	// Header + checksum
#endif

#ifndef	AB_DELAY_SHORT
#define	AB_DELAY_SHORT	512
#endif

#ifndef	AB_DELAY_LONG
#define	AB_DELAY_LONG	2048
#endif

#ifndef	AB_XTRIES
#define	AB_XTRIES	3
#endif

#define	AB_EVENT_IN	((void*)(&ab_xrs_cin))
#define	AB_EVENT_OUT	((void*)(&ab_xrs_cou))
#define	AB_EVENT_RUN	((void*)(&ab_xrs_new))

#define __dcx_dcl__
#include "ab_data.h"
#undef	__dcx_dcl__

#endif
