// No need to force single inclusion

// This is the selection of Node attributes based on the configuration of
// requisite libraries

#define	__dcx_def__

#ifdef	VUEE_LIB_PLUG_NULL
#include "plug_null_node_data.h"
#endif

#ifdef	VUEE_LIB_PLUG_TARP
#include "plug_tarp_node_data.h"
#include "tarp_node_data.h"
#include "net_node_data.h"
#endif

#ifdef	VUEE_LIB_XRS
#include "ab_data.h"
#endif

#ifdef	VUEE_LIB_OEP
#include "oep_node_data.h"
#include "oep_ee_node_data.h"
#endif

#ifdef	VUEE_LIB_LCDG
#include "lcdg_images_node_data.h"
#include "lcdg_dispman_node_data.h"
#endif

#undef __dcx_def__
