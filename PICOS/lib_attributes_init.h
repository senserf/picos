// No need to force single inclusion

// This is the selection of Node attributes based on the configuration of
// requisite libraries

#define __dcx_ini__

#ifdef	VUEE_LIB_PLUG_NULL
#include "plug_null_node_data_init.h"
#endif

#ifdef	VUEE_LIB_PLUG_TARP
#include "net_node_data_init.h"
#include "plug_tarp_node_data_init.h"
#include "tarp_node_data_init.h"
#endif

#ifdef	VUEE_LIB_PLUG_BOSS
#include "plug_boss_node_data_init.h"
#endif

#ifdef	VUEE_LIB_XRS
#include "ab_data.h"
#endif

#ifdef	VUEE_LIB_OEP
#include "oep_node_data_init.h"
#include "oep_ee_node_data_init.h"
#endif

#ifdef	VUEE_LIB_LCDG
#include "lcdg_images_node_data_init.h"
#include "lcdg_dispman_node_data_init.h"
#endif

#undef	__dcx_ini__
