#ifndef	__lcdg_dispman_params_h__
#define	__lcdg_dispman_params_h__

#include "sysio.h"

#define	LCDG_DMTYPE_IMAGE	0
#define	LCDG_DMTYPE_MENU	1
#define	LCDG_DMTYPE_TEXT	2

// Errors (compatible with OEP)
#define	LCDG_DMERR_NOMEM	OEP_STATUS_NOMEM
#define	LCDG_DMERR_GARBAGE	LCDG_IMGERR_GARBAGE
#define	LCDG_DMERR_NOFONT	40
#define	LCDG_DMERR_RECT		41

#define	LCDG_DMTYPE_MASK	0x03

// Flag == free data on deallocation
#define	LCDG_DMTYPE_FDATA	0x80

#ifndef	LCDG_DM_SHORT_ARGS
#define	LCDG_DM_SHORT_ARGS	0
#endif

#ifndef	LCDG_DM_SOFT_CREATORS
#define	LCDG_DM_SOFT_CREATORS	2
#endif

#ifndef LCDG_DM_EE_CREATORS
#define	LCDG_DM_EE_CREATORS	1
#endif

#endif
