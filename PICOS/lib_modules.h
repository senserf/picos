#ifndef __lib_modules_h__
#define __lib_modules_h__

#ifdef	VUEE_LIB_PLUG_NULL
#include "plug_null.cc"
#endif

#ifdef	VUEE_LIB_PLUG_TARP
#include "net.cc"
#include "plug_tarp.cc"
#include "tarp.cc"
#endif

#ifdef	VUEE_LIB_XRS
#include "ab.cc"
#endif

#ifdef VUEE_LIB_OEP
#include "oep.cc"
#include "oep_ee.cc"
#endif

#ifdef VUEE_LIB_OL
#include "ee_ol.cc"
#endif

#ifdef VUEE_LIB_LCDG
#include "lcdg_images.cc"
#include "lcdg_dispman.cc"
#endif

#endif
