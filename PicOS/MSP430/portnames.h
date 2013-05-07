#ifndef __pg_portnames_h
#define	__pg_portnames_h		1

#include <io.h>
#include <signal.h>

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Transformations of port operations, such that port pins can be
// referenced uniformly as a continuous range of numbers. This sucks.
// The idea is to have something that takes a trivial amount of code if the
// pin number is a constant, but not necessarily so if it is a variable.
// It is assumed that if you ever want to use these operations for a
// variable argument, then you will implement functions referencing these
// macros.

#if defined(P1DIR) || defined(P1DIR_)

#define	_PDS_s_01(p)	case 0: _BIS (P1DIR, 1 << ((p) & 7)); break;
#define	_PDS_c_01(p)	case 0: _BIC (P1DIR, 1 << ((p) & 7)); break;
#define	_PFS_s_01(p)	case 0: _BIS (P1SEL, 1 << ((p) & 7)); break;
#define	_PFS_c_01(p)	case 0: _BIC (P1SEL, 1 << ((p) & 7)); break;
#define	_PVS_s_01(p)	case 0: _BIS (P1OUT, 1 << ((p) & 7)); break;
#define	_PVS_c_01(p)	case 0: _BIC (P1OUT, 1 << ((p) & 7)); break;
#define	_PHS_s_01(p)	case 0: _BIS (P1DS,  1 << ((p) & 7)); break;
#define	_PHS_c_01(p)	case 0: _BIC (P1DS,  1 << ((p) & 7)); break;
#define	_PPS_s_01(p)	case 0: _BIS (P1REN, 1 << ((p) & 7)); break;
#define	_PPS_c_01(p)	case 0: _BIC (P1REN, 1 << ((p) & 7)); break;
#define	_PV____01(p)	((p) >> 3) == 0 ? P1IN
#define	_PF____01(p)	((p) >> 3) == 0 ? P1SEL
#define	_PD____01(p)	((p) >> 3) == 0 ? P1DIR
#define	_PH____01(p)	((p) >> 3) == 0 ? P1DS
#define	_PP____01(p)	((p) >> 3) == 0 ? P1REN

#define	P1_0		(0+0)
#define	P1_1		(0+1)
#define	P1_2		(0+2)
#define	P1_3		(0+3)
#define	P1_4		(0+4)
#define	P1_5		(0+5)
#define	P1_6		(0+6)
#define	P1_7		(0+7)

#else

#define	_PDS_s_01(p)
#define	_PDS_c_01(p)
#define	_PFS_s_01(p)
#define	_PFS_c_01(p)
#define	_PVS_s_01(p)
#define	_PVS_c_01(p)
#define	_PHS_s_01(p)
#define	_PHS_c_01(p)
#define	_PPS_s_01(p)
#define	_PPS_c_01(p)
#define	_PV____01(p)	0 ? 0
#define	_PF____01(p)	0 ? 0
#define	_PD____01(p)	0 ? 0
#define	_PH____01(p)	0 ? 0
#define	_PP____01(p)	0 ? 0

#endif

// ============================================================================

#if defined(P2DIR) || defined(P2DIR_)

#define	_PDS_s_02(p)	case 1: _BIS (P2DIR, 1 << ((p) & 7)); break;
#define	_PDS_c_02(p)	case 1: _BIC (P2DIR, 1 << ((p) & 7)); break;
#define	_PFS_s_02(p)	case 1: _BIS (P2SEL, 1 << ((p) & 7)); break;
#define	_PFS_c_02(p)	case 1: _BIC (P2SEL, 1 << ((p) & 7)); break;
#define	_PVS_s_02(p)	case 1: _BIS (P2OUT, 1 << ((p) & 7)); break;
#define	_PVS_c_02(p)	case 1: _BIC (P2OUT, 1 << ((p) & 7)); break;
#define	_PHS_s_02(p)	case 1: _BIS (P2DS,  1 << ((p) & 7)); break;
#define	_PHS_c_02(p)	case 1: _BIC (P2DS,  1 << ((p) & 7)); break;
#define	_PPS_s_02(p)	case 1: _BIS (P2REN, 1 << ((p) & 7)); break;
#define	_PPS_c_02(p)	case 1: _BIC (P2REN, 1 << ((p) & 7)); break;
#define	_PV____02(p)	((p) >> 3) == 0 ? P2IN
#define	_PF____02(p)	((p) >> 3) == 0 ? P2SEL
#define	_PD____02(p)	((p) >> 3) == 0 ? P2DIR
#define	_PH____02(p)	((p) >> 3) == 0 ? P2DS
#define	_PP____02(p)	((p) >> 3) == 0 ? P2REN

#define	P2_0		(8+0)
#define	P2_1		(8+1)
#define	P2_2		(8+2)
#define	P2_3		(8+3)
#define	P2_4		(8+4)
#define	P2_5		(8+5)
#define	P2_6		(8+6)
#define	P2_7		(8+7)

#else

#define	_PDS_s_02(p)
#define	_PDS_c_02(p)
#define	_PFS_s_02(p)
#define	_PFS_c_02(p)
#define	_PVS_s_02(p)
#define	_PVS_c_02(p)
#define	_PHS_s_02(p)
#define	_PHS_c_02(p)
#define	_PPS_s_02(p)
#define	_PPS_c_02(p)
#define	_PV____02(p)	0 ? 0
#define	_PF____02(p)	0 ? 0
#define	_PD____02(p)	0 ? 0
#define	_PH____02(p)	0 ? 0
#define	_PP____02(p)	0 ? 0

#endif

// ============================================================================

#if defined(P3DIR) || defined(P3DIR_)

#define	_PDS_s_03(p)	case 2: _BIS (P3DIR, 1 << ((p) & 7)); break;
#define	_PDS_c_03(p)	case 2: _BIC (P3DIR, 1 << ((p) & 7)); break;
#define	_PFS_s_03(p)	case 2: _BIS (P3SEL, 1 << ((p) & 7)); break;
#define	_PFS_c_03(p)	case 2: _BIC (P3SEL, 1 << ((p) & 7)); break;
#define	_PVS_s_03(p)	case 2: _BIS (P3OUT, 1 << ((p) & 7)); break;
#define	_PVS_c_03(p)	case 2: _BIC (P3OUT, 1 << ((p) & 7)); break;
#define	_PHS_s_03(p)	case 2: _BIS (P3DS,  1 << ((p) & 7)); break;
#define	_PHS_c_03(p)	case 2: _BIC (P3DS,  1 << ((p) & 7)); break;
#define	_PPS_s_03(p)	case 2: _BIS (P3REN, 1 << ((p) & 7)); break;
#define	_PPS_c_03(p)	case 2: _BIC (P3REN, 1 << ((p) & 7)); break;
#define	_PV____03(p)	((p) >> 3) == 0 ? P3IN
#define	_PF____03(p)	((p) >> 3) == 0 ? P3SEL
#define	_PD____03(p)	((p) >> 3) == 0 ? P3DIR
#define	_PH____03(p)	((p) >> 3) == 0 ? P3DS
#define	_PP____03(p)	((p) >> 3) == 0 ? P3REN

#define	P3_0		(16+0)
#define	P3_1		(16+1)
#define	P3_2		(16+2)
#define	P3_3		(16+3)
#define	P3_4		(16+4)
#define	P3_5		(16+5)
#define	P3_6		(16+6)
#define	P3_7		(16+7)

#else

#define	_PDS_s_03(p)
#define	_PDS_c_03(p)
#define	_PFS_s_03(p)
#define	_PFS_c_03(p)
#define	_PVS_s_03(p)
#define	_PVS_c_03(p)
#define	_PHS_s_03(p)
#define	_PHS_c_03(p)
#define	_PPS_s_03(p)
#define	_PPS_c_03(p)
#define	_PV____03(p)	0 ? 0
#define	_PF____03(p)	0 ? 0
#define	_PD____03(p)	0 ? 0
#define	_PH____03(p)	0 ? 0
#define	_PP____03(p)	0 ? 0

#endif

// ============================================================================

#if defined(P4DIR) || defined(P4DIR_)

#define	_PDS_s_04(p)	case 3: _BIS (P4DIR, 1 << ((p) & 7)); break;
#define	_PDS_c_04(p)	case 3: _BIC (P4DIR, 1 << ((p) & 7)); break;
#define	_PFS_s_04(p)	case 3: _BIS (P4SEL, 1 << ((p) & 7)); break;
#define	_PFS_c_04(p)	case 3: _BIC (P4SEL, 1 << ((p) & 7)); break;
#define	_PVS_s_04(p)	case 3: _BIS (P4OUT, 1 << ((p) & 7)); break;
#define	_PVS_c_04(p)	case 3: _BIC (P4OUT, 1 << ((p) & 7)); break;
#define	_PHS_s_04(p)	case 3: _BIS (P4DS,  1 << ((p) & 7)); break;
#define	_PHS_c_04(p)	case 3: _BIC (P4DS,  1 << ((p) & 7)); break;
#define	_PPS_s_04(p)	case 3: _BIS (P4REN, 1 << ((p) & 7)); break;
#define	_PPS_c_04(p)	case 3: _BIC (P4REN, 1 << ((p) & 7)); break;
#define	_PV____04(p)	((p) >> 3) == 0 ? P4IN
#define	_PF____04(p)	((p) >> 3) == 0 ? P4SEL
#define	_PD____04(p)	((p) >> 3) == 0 ? P4DIR
#define	_PH____04(p)	((p) >> 3) == 0 ? P4DS
#define	_PP____04(p)	((p) >> 3) == 0 ? P4REN

#define	P4_0		(24+0)
#define	P4_1		(24+1)
#define	P4_2		(24+2)
#define	P4_3		(24+3)
#define	P4_4		(24+4)
#define	P4_5		(24+5)
#define	P4_6		(24+6)
#define	P4_7		(24+7)

#else

#define	_PDS_s_04(p)
#define	_PDS_c_04(p)
#define	_PFS_s_04(p)
#define	_PFS_c_04(p)
#define	_PVS_s_04(p)
#define	_PVS_c_04(p)
#define	_PHS_s_04(p)
#define	_PHS_c_04(p)
#define	_PPS_s_04(p)
#define	_PPS_c_04(p)
#define	_PV____04(p)	0 ? 0
#define	_PF____04(p)	0 ? 0
#define	_PD____04(p)	0 ? 0
#define	_PH____04(p)	0 ? 0
#define	_PP____04(p)	0 ? 0

#endif

// ============================================================================

#if defined(P5DIR) || defined(P5DIR_)

#define	_PDS_s_05(p)	case 4: _BIS (P5DIR, 1 << ((p) & 7)); break;
#define	_PDS_c_05(p)	case 4: _BIC (P5DIR, 1 << ((p) & 7)); break;
#define	_PFS_s_05(p)	case 4: _BIS (P5SEL, 1 << ((p) & 7)); break;
#define	_PFS_c_05(p)	case 4: _BIC (P5SEL, 1 << ((p) & 7)); break;
#define	_PVS_s_05(p)	case 4: _BIS (P5OUT, 1 << ((p) & 7)); break;
#define	_PVS_c_05(p)	case 4: _BIC (P5OUT, 1 << ((p) & 7)); break;
#define	_PHS_s_05(p)	case 4: _BIS (P5DS,  1 << ((p) & 7)); break;
#define	_PHS_c_05(p)	case 4: _BIC (P5DS,  1 << ((p) & 7)); break;
#define	_PPS_s_05(p)	case 4: _BIS (P5REN, 1 << ((p) & 7)); break;
#define	_PPS_c_05(p)	case 4: _BIC (P5REN, 1 << ((p) & 7)); break;
#define	_PV____05(p)	((p) >> 3) == 0 ? P5IN
#define	_PF____05(p)	((p) >> 3) == 0 ? P5SEL
#define	_PD____05(p)	((p) >> 3) == 0 ? P5DIR
#define	_PH____05(p)	((p) >> 3) == 0 ? P5DS
#define	_PP____05(p)	((p) >> 3) == 0 ? P5REN

#define	P5_0		(32+0)
#define	P5_1		(32+1)
#define	P5_2		(32+2)
#define	P5_3		(32+3)
#define	P5_4		(32+4)
#define	P5_5		(32+5)
#define	P5_6		(32+6)
#define	P5_7		(32+7)

#else

#define	_PDS_s_05(p)
#define	_PDS_c_05(p)
#define	_PFS_s_05(p)
#define	_PFS_c_05(p)
#define	_PVS_s_05(p)
#define	_PVS_c_05(p)
#define	_PHS_s_05(p)
#define	_PHS_c_05(p)
#define	_PPS_s_05(p)
#define	_PPS_c_05(p)
#define	_PV____05(p)	0 ? 0
#define	_PF____05(p)	0 ? 0
#define	_PD____05(p)	0 ? 0
#define	_PH____05(p)	0 ? 0
#define	_PP____05(p)	0 ? 0

#endif

// ============================================================================

#if defined(P6DIR) || defined(P6DIR_)

#define	_PDS_s_06(p)	case 5: _BIS (P6DIR, 1 << ((p) & 7)); break;
#define	_PDS_c_06(p)	case 5: _BIC (P6DIR, 1 << ((p) & 7)); break;
#define	_PFS_s_06(p)	case 5: _BIS (P6SEL, 1 << ((p) & 7)); break;
#define	_PFS_c_06(p)	case 5: _BIC (P6SEL, 1 << ((p) & 7)); break;
#define	_PVS_s_06(p)	case 5: _BIS (P6OUT, 1 << ((p) & 7)); break;
#define	_PVS_c_06(p)	case 5: _BIC (P6OUT, 1 << ((p) & 7)); break;
#define	_PHS_s_06(p)	case 5: _BIS (P6DS,  1 << ((p) & 7)); break;
#define	_PHS_c_06(p)	case 5: _BIC (P6DS,  1 << ((p) & 7)); break;
#define	_PPS_s_06(p)	case 5: _BIS (P6REN, 1 << ((p) & 7)); break;
#define	_PPS_c_06(p)	case 5: _BIC (P6REN, 1 << ((p) & 7)); break;
#define	_PV____06(p)	((p) >> 3) == 5 ? P6IN
#define	_PF____06(p)	((p) >> 3) == 5 ? P6SEL
#define	_PD____06(p)	((p) >> 3) == 5 ? P6DIR
#define	_PH____06(p)	((p) >> 3) == 5 ? P6DS
#define	_PP____06(p)	((p) >> 3) == 5 ? P6REN

#define	P6_0		(40+0)
#define	P6_1		(40+1)
#define	P6_2		(40+2)
#define	P6_3		(40+3)
#define	P6_4		(40+4)
#define	P6_5		(40+5)
#define	P6_6		(40+6)
#define	P6_7		(40+7)

#else

#define	_PDS_s_06(p)
#define	_PDS_c_06(p)
#define	_PFS_s_06(p)
#define	_PFS_c_06(p)
#define	_PVS_s_06(p)
#define	_PVS_c_06(p)
#define	_PHS_s_06(p)
#define	_PHS_c_06(p)
#define	_PPS_s_06(p)
#define	_PPS_c_06(p)
#define	_PV____06(p)	0 ? 0
#define	_PF____06(p)	0 ? 0
#define	_PD____06(p)	0 ? 0
#define	_PH____06(p)	0 ? 0
#define	_PP____06(p)	0 ? 0

#endif	/* P6 present */

// ============================================================================

#if defined(P7DIR) || defined(P7DIR_)

#define	_PDS_s_07(p)	case 6: _BIS (P7DIR, 1 << ((p) & 7)); break;
#define	_PDS_c_07(p)	case 6: _BIC (P7DIR, 1 << ((p) & 7)); break;
#define	_PFS_s_07(p)	case 6: _BIS (P7SEL, 1 << ((p) & 7)); break;
#define	_PFS_c_07(p)	case 6: _BIC (P7SEL, 1 << ((p) & 7)); break;
#define	_PVS_s_07(p)	case 6: _BIS (P7OUT, 1 << ((p) & 7)); break;
#define	_PVS_c_07(p)	case 6: _BIC (P7OUT, 1 << ((p) & 7)); break;
#define	_PHS_s_07(p)	case 6: _BIS (P7DS,  1 << ((p) & 7)); break;
#define	_PHS_c_07(p)	case 6: _BIC (P7DS,  1 << ((p) & 7)); break;
#define	_PPS_s_07(p)	case 6: _BIS (P7REN, 1 << ((p) & 7)); break;
#define	_PPS_c_07(p)	case 6: _BIC (P7REN, 1 << ((p) & 7)); break;
#define	_PV____07(p)	((p) >> 3) == 6 ? P7IN
#define	_PF____07(p)	((p) >> 3) == 6 ? P7SEL
#define	_PD____07(p)	((p) >> 3) == 6 ? P7DIR
#define	_PH____07(p)	((p) >> 3) == 6 ? P7DS
#define	_PP____07(p)	((p) >> 3) == 6 ? P7REN

#define	P7_0		(48+0)
#define	P7_1		(48+1)
#define	P7_2		(48+2)
#define	P7_3		(48+3)
#define	P7_4		(48+4)
#define	P7_5		(48+5)
#define	P7_6		(48+6)
#define	P7_7		(48+7)

#else

#define	_PDS_s_07(p)
#define	_PDS_c_07(p)
#define	_PFS_s_07(p)
#define	_PFS_c_07(p)
#define	_PVS_s_07(p)
#define	_PVS_c_07(p)
#define	_PHS_s_07(p)
#define	_PHS_c_07(p)
#define	_PPS_s_07(p)
#define	_PPS_c_07(p)
#define	_PV____07(p)	0 ? 0
#define	_PF____07(p)	0 ? 0
#define	_PD____07(p)	0 ? 0
#define	_PH____07(p)	0 ? 0
#define	_PP____07(p)	0 ? 0

#endif	/* P7 present */

// ============================================================================

#if defined(P8DIR) || defined(P8DIR_)

#define	_PDS_s_08(p)	case 7: _BIS (P8DIR, 1 << ((p) & 7)); break;
#define	_PDS_c_08(p)	case 7: _BIC (P8DIR, 1 << ((p) & 7)); break;
#define	_PFS_s_08(p)	case 7: _BIS (P8SEL, 1 << ((p) & 7)); break;
#define	_PFS_c_08(p)	case 7: _BIC (P8SEL, 1 << ((p) & 7)); break;
#define	_PVS_s_08(p)	case 7: _BIS (P8OUT, 1 << ((p) & 7)); break;
#define	_PVS_c_08(p)	case 7: _BIC (P8OUT, 1 << ((p) & 7)); break;
#define	_PHS_s_08(p)	case 7: _BIS (P8DS,  1 << ((p) & 7)); break;
#define	_PHS_c_08(p)	case 7: _BIC (P8DS,  1 << ((p) & 7)); break;
#define	_PPS_s_08(p)	case 7: _BIS (P8REN, 1 << ((p) & 7)); break;
#define	_PPS_c_08(p)	case 7: _BIC (P8REN, 1 << ((p) & 7)); break;
#define	_PV____08(p)	((p) >> 3) == 7 ? P8IN
#define	_PF____08(p)	((p) >> 3) == 7 ? P8SEL
#define	_PD____08(p)	((p) >> 3) == 7 ? P8DIR
#define	_PH____08(p)	((p) >> 3) == 7 ? P8DS
#define	_PP____08(p)	((p) >> 3) == 7 ? P8REN

#define	P8_0		(56+0)
#define	P8_1		(56+1)
#define	P8_2		(56+2)
#define	P8_3		(56+3)
#define	P8_4		(56+4)
#define	P8_5		(56+5)
#define	P8_6		(56+6)
#define	P8_7		(56+7)

#else

#define	_PDS_s_08(p)
#define	_PDS_c_08(p)
#define	_PFS_s_08(p)
#define	_PFS_c_08(p)
#define	_PVS_s_08(p)
#define	_PVS_c_08(p)
#define	_PHS_s_08(p)
#define	_PHS_c_08(p)
#define	_PPS_s_08(p)
#define	_PPS_c_08(p)
#define	_PV____08(p)	0 ? 0
#define	_PF____08(p)	0 ? 0
#define	_PD____08(p)	0 ? 0
#define	_PH____08(p)	0 ? 0
#define	_PP____08(p)	0 ? 0

#endif	/* P8 present */

// ============================================================================

#if defined(P9DIR) || defined(P9DIR_)

#define	_PDS_s_09(p)	case 8: _BIS (P9DIR, 1 << ((p) & 7)); break;
#define	_PDS_c_09(p)	case 8: _BIC (P9DIR, 1 << ((p) & 7)); break;
#define	_PFS_s_09(p)	case 8: _BIS (P9SEL, 1 << ((p) & 7)); break;
#define	_PFS_c_09(p)	case 8: _BIC (P9SEL, 1 << ((p) & 7)); break;
#define	_PVS_s_09(p)	case 8: _BIS (P9OUT, 1 << ((p) & 7)); break;
#define	_PVS_c_09(p)	case 8: _BIC (P9OUT, 1 << ((p) & 7)); break;
#define	_PHS_s_09(p)	case 8: _BIS (P9DS,  1 << ((p) & 7)); break;
#define	_PHS_c_09(p)	case 8: _BIC (P9DS,  1 << ((p) & 7)); break;
#define	_PPS_s_09(p)	case 8: _BIS (P9REN, 1 << ((p) & 7)); break;
#define	_PPS_c_09(p)	case 8: _BIC (P9REN, 1 << ((p) & 7)); break;
#define	_PV____09(p)	((p) >> 3) == 8 ? P9IN
#define	_PF____09(p)	((p) >> 3) == 8 ? P9SEL
#define	_PD____09(p)	((p) >> 3) == 8 ? P9DIR
#define	_PH____09(p)	((p) >> 3) == 8 ? P9DS
#define	_PP____09(p)	((p) >> 3) == 8 ? P9REN

#define	P9_0		(64+0)
#define	P9_1		(64+1)
#define	P9_2		(64+2)
#define	P9_3		(64+3)
#define	P9_4		(64+4)
#define	P9_5		(64+5)
#define	P9_6		(64+6)
#define	P9_7		(64+7)

#else

#define	_PDS_s_09(p)
#define	_PDS_c_09(p)
#define	_PFS_s_09(p)
#define	_PFS_c_09(p)
#define	_PVS_s_09(p)
#define	_PVS_c_09(p)
#define	_PHS_s_09(p)
#define	_PHS_c_09(p)
#define	_PPS_s_09(p)
#define	_PPS_c_09(p)
#define	_PV____09(p)	0 ? 0
#define	_PF____09(p)	0 ? 0
#define	_PD____09(p)	0 ? 0
#define	_PH____09(p)	0 ? 0
#define	_PP____09(p)	0 ? 0

#endif	/* P9 present */

// ============================================================================

#if defined(P10DIR) || defined(P10DIR_)

#define	_PDS_s_10(p)	case 9: _BIS (P10DIR, 1 << ((p) & 7)); break;
#define	_PDS_c_10(p)	case 9: _BIC (P10DIR, 1 << ((p) & 7)); break;
#define	_PFS_s_10(p)	case 9: _BIS (P10SEL, 1 << ((p) & 7)); break;
#define	_PFS_c_10(p)	case 9: _BIC (P10SEL, 1 << ((p) & 7)); break;
#define	_PVS_s_10(p)	case 9: _BIS (P10OUT, 1 << ((p) & 7)); break;
#define	_PVS_c_10(p)	case 9: _BIC (P10OUT, 1 << ((p) & 7)); break;
#define	_PHS_s_10(p)	case 9: _BIS (P10DS,  1 << ((p) & 7)); break;
#define	_PHS_c_10(p)	case 9: _BIC (P10DS,  1 << ((p) & 7)); break;
#define	_PPS_s_10(p)	case 9: _BIS (P10REN, 1 << ((p) & 7)); break;
#define	_PPS_c_10(p)	case 9: _BIC (P10REN, 1 << ((p) & 7)); break;
#define	_PV____10(p)	((p) >> 3) == 9 ? P10IN
#define	_PF____10(p)	((p) >> 3) == 9 ? P10SEL
#define	_PD____10(p)	((p) >> 3) == 9 ? P10DIR
#define	_PH____10(p)	((p) >> 3) == 9 ? P10DS
#define	_PP____10(p)	((p) >> 3) == 9 ? P10REN

#define	P10_0		(72+0)
#define	P10_1		(72+1)
#define	P10_2		(72+2)
#define	P10_3		(72+3)
#define	P10_4		(72+4)
#define	P10_5		(72+5)
#define	P10_6		(72+6)
#define	P10_7		(72+7)

#else

#define	_PDS_s_10(p)
#define	_PDS_c_10(p)
#define	_PFS_s_10(p)
#define	_PFS_c_10(p)
#define	_PVS_s_10(p)
#define	_PVS_c_10(p)
#define	_PHS_s_10(p)
#define	_PHS_c_10(p)
#define	_PPS_s_10(p)
#define	_PPS_c_10(p)
#define	_PV____10(p)	0 ? 0
#define	_PF____10(p)	0 ? 0
#define	_PD____10(p)	0 ? 0
#define	_PH____10(p)	0 ? 0
#define	_PP____10(p)	0 ? 0

#endif

// ============================================================================

#if defined(P11DIR) || defined(P11DIR_)

#define	_PDS_s_11(p)	case 10: _BIS (P11DIR, 1 << ((p) & 7)); break;
#define	_PDS_c_11(p)	case 10: _BIC (P11DIR, 1 << ((p) & 7)); break;
#define	_PFS_s_11(p)	case 10: _BIS (P11SEL, 1 << ((p) & 7)); break;
#define	_PFS_c_11(p)	case 10: _BIC (P11SEL, 1 << ((p) & 7)); break;
#define	_PVS_s_11(p)	case 10: _BIS (P11OUT, 1 << ((p) & 7)); break;
#define	_PVS_c_11(p)	case 10: _BIC (P11OUT, 1 << ((p) & 7)); break;
#define	_PHS_s_11(p)	case 10: _BIS (P11DS,  1 << ((p) & 7)); break;
#define	_PHS_c_11(p)	case 10: _BIC (P11DS,  1 << ((p) & 7)); break;
#define	_PPS_s_11(p)	case 10: _BIS (P11REN, 1 << ((p) & 7)); break;
#define	_PPS_c_11(p)	case 10: _BIC (P11REN, 1 << ((p) & 7)); break;
#define	_PV____11(p)	((p) >> 3) == 10 ? P11IN
#define	_PF____11(p)	((p) >> 3) == 10 ? P11SEL
#define	_PD____11(p)	((p) >> 3) == 10 ? P11DIR
#define	_PH____11(p)	((p) >> 3) == 10 ? P11DS
#define	_PP____11(p)	((p) >> 3) == 10 ? P11REN

#define	P11_0		(80+0)
#define	P11_1		(80+1)
#define	P11_2		(80+2)
#define	P11_3		(80+3)
#define	P11_4		(80+4)
#define	P11_5		(80+5)
#define	P11_6		(80+6)
#define	P11_7		(80+7)

#else

#define	_PDS_s_11(p)
#define	_PDS_c_11(p)
#define	_PFS_s_11(p)
#define	_PFS_c_11(p)
#define	_PVS_s_11(p)
#define	_PVS_c_11(p)
#define	_PHS_s_11(p)
#define	_PHS_c_11(p)
#define	_PPS_s_11(p)
#define	_PPS_c_11(p)
#define	_PV____11(p)	0 ? 0
#define	_PF____11(p)	0 ? 0
#define	_PD____11(p)	0 ? 0
#define	_PH____11(p)	0 ? 0
#define	_PP____11(p)	0 ? 0

#endif

// ============================================================================

// Port J has no SEL
#define	_PFS_s_J(p)
#define	_PFS_c_J(p)
#define	_PF____J(p)	0 ? 0

#if defined(PJDIR) || defined(PJDIR_)

#define	_PDS_s_J(p)	case 11: _BIS (PJDIR, 1 << ((p) & 7)); break;
#define	_PDS_c_J(p)	case 11: _BIC (PJDIR, 1 << ((p) & 7)); break;
#define	_PVS_s_J(p)	case 11: _BIS (PJOUT, 1 << ((p) & 7)); break;
#define	_PVS_c_J(p)	case 11: _BIC (PJOUT, 1 << ((p) & 7)); break;
#define	_PHS_s_J(p)	case 11: _BIS (PJDS,  1 << ((p) & 7)); break;
#define	_PHS_c_J(p)	case 11: _BIC (PJDS,  1 << ((p) & 7)); break;
#define	_PPS_s_J(p)	case 11: _BIS (PJREN, 1 << ((p) & 7)); break;
#define	_PPS_c_J(p)	case 11: _BIC (PJREN, 1 << ((p) & 7)); break;
#define	_PV____J(p)	((p) >> 3) == 11 ? PJIN
#define	_PD____J(p)	((p) >> 3) == 11 ? PJDIR
#define	_PH____J(p)	((p) >> 3) == 11 ? PJDS
#define	_PP____J(p)	((p) >> 3) == 11 ? PJREN

#define	PJ_0		(88+0)
#define	PJ_1		(88+1)
#define	PJ_2		(88+2)
#define	PJ_3		(88+3)
#define	PJ_4		(88+4)
#define	PJ_5		(88+5)
#define	PJ_6		(88+6)
#define	PJ_7		(88+7)

#else

#define	_PDS_s_J(p)
#define	_PDS_c_J(p)
#define	_PVS_s_J(p)
#define	_PVS_c_J(p)
#define	_PHS_s_J(p)
#define	_PHS_c_J(p)
#define	_PPS_s_J(p)
#define	_PPS_c_J(p)
#define	_PV____J(p)	0 ? 0
#define	_PD____J(p)	0 ? 0
#define	_PH____J(p)	0 ? 0
#define	_PP____J(p)	0 ? 0

#endif

// ============================================================================

//
// The compiler will optimize this garbage out as long as the port number is a
// constant
//
#define	_PDS(p,v)	do { \
			    if (v) { \
				switch ((p) >> 3) { \
					_PDS_s_01 (    p) \
					_PDS_s_02 (    p) \
					_PDS_s_03 (    p) \
					_PDS_s_04 (    p) \
					_PDS_s_05 (    p) \
					_PDS_s_06 (    p) \
					_PDS_s_07 (    p) \
					_PDS_s_08 (    p) \
					_PDS_s_09 (    p) \
					_PDS_s_10 (    p) \
					_PDS_s_11 (    p) \
					_PDS_s_J  (    p) \
					default: CNOP; \
				} \
			    } else { \
				switch ((p) >> 3) { \
					_PDS_c_01 (    p) \
					_PDS_c_02 (    p) \
					_PDS_c_03 (    p) \
					_PDS_c_04 (    p) \
					_PDS_c_05 (    p) \
					_PDS_c_06 (    p) \
					_PDS_c_07 (    p) \
					_PDS_c_08 (    p) \
					_PDS_c_09 (    p) \
					_PDS_c_10 (    p) \
					_PDS_c_11 (    p) \
					_PDS_c_J  (    p) \
					default: CNOP; \
				} \
			    } \
			} while (0)

#define	_PFS(p,v)	do { \
			    if (v) { \
				switch ((p) >> 3) { \
					_PFS_s_01 (    p) \
					_PFS_s_02 (    p) \
					_PFS_s_03 (    p) \
					_PFS_s_04 (    p) \
					_PFS_s_05 (    p) \
					_PFS_s_06 (    p) \
					_PFS_s_07 (    p) \
					_PFS_s_08 (    p) \
					_PFS_s_09 (    p) \
					_PFS_s_10 (    p) \
					_PFS_s_11 (    p) \
					_PFS_s_J  (    p) \
					default: CNOP; \
				} \
			    } else { \
				switch ((p) >> 3) { \
					_PFS_c_01 (    p) \
					_PFS_c_02 (    p) \
					_PFS_c_03 (    p) \
					_PFS_c_04 (    p) \
					_PFS_c_05 (    p) \
					_PFS_c_06 (    p) \
					_PFS_c_07 (    p) \
					_PFS_c_08 (    p) \
					_PFS_c_09 (    p) \
					_PFS_c_10 (    p) \
					_PFS_c_11 (    p) \
					_PFS_c_J  (    p) \
					default: CNOP; \
				} \
			    } \
			} while (0)

#define	_PVS(p,v)	do { \
			    if (v) { \
				switch ((p) >> 3) { \
					_PVS_s_01 (    p) \
					_PVS_s_02 (    p) \
					_PVS_s_03 (    p) \
					_PVS_s_04 (    p) \
					_PVS_s_05 (    p) \
					_PVS_s_06 (    p) \
					_PVS_s_07 (    p) \
					_PVS_s_08 (    p) \
					_PVS_s_09 (    p) \
					_PVS_s_10 (    p) \
					_PVS_s_11 (    p) \
					_PVS_s_J  (    p) \
					default: CNOP; \
				} \
			    } else { \
				switch ((p) >> 3) { \
					_PVS_c_01 (    p) \
					_PVS_c_02 (    p) \
					_PVS_c_03 (    p) \
					_PVS_c_04 (    p) \
					_PVS_c_05 (    p) \
					_PVS_c_06 (    p) \
					_PVS_c_07 (    p) \
					_PVS_c_08 (    p) \
					_PVS_c_09 (    p) \
					_PVS_c_10 (    p) \
					_PVS_c_11 (    p) \
					_PVS_c_J  (    p) \
					default: CNOP; \
				} \
			    } \
			} while (0)

#define	_PV(p)		( (  ( \
				(_PV____01 (   p) : \
				(_PV____02 (   p) : \
				(_PV____03 (   p) : \
				(_PV____04 (   p) : \
				(_PV____05 (   p) : \
				(_PV____06 (   p) : \
				(_PV____07 (   p) : \
				(_PV____08 (   p) : \
				(_PV____09 (   p) : \
				(_PV____10 (   p) : \
				(_PV____11 (   p) : \
				(_PV____J  (   p) : \
					0 )))))))))))) ) >> ((p) & 7)) & 1)
#define	_PF(p)		( (  ( \
				(_PF____01 (   p) : \
				(_PF____02 (   p) : \
				(_PF____03 (   p) : \
				(_PF____04 (   p) : \
				(_PF____05 (   p) : \
				(_PF____06 (   p) : \
				(_PF____07 (   p) : \
				(_PF____08 (   p) : \
				(_PF____09 (   p) : \
				(_PF____10 (   p) : \
				(_PF____11 (   p) : \
				(_PF____J  (   p) : \
					0 )))))))))))) ) >> ((p) & 7)) & 1)
#define	_PD(p)		( (  ( \
				(_PD____01 (   p) : \
				(_PD____02 (   p) : \
				(_PD____03 (   p) : \
				(_PD____04 (   p) : \
				(_PD____05 (   p) : \
				(_PD____06 (   p) : \
				(_PD____07 (   p) : \
				(_PD____08 (   p) : \
				(_PD____09 (   p) : \
				(_PD____10 (   p) : \
				(_PD____11 (   p) : \
				(_PD____J  (   p) : \
					0 )))))))))))) ) >> ((p) & 7)) & 1)

#ifdef	P1DS

// ============================================================================

#define	_PHS(p,v)	do { \
			    if (v) { \
				switch ((p) >> 3) { \
					_PHS_s_01 (    p) \
					_PHS_s_02 (    p) \
					_PHS_s_03 (    p) \
					_PHS_s_04 (    p) \
					_PHS_s_05 (    p) \
					_PHS_s_06 (    p) \
					_PHS_s_07 (    p) \
					_PHS_s_08 (    p) \
					_PHS_s_09 (    p) \
					_PHS_s_10 (    p) \
					_PHS_s_11 (    p) \
					_PHS_s_J  (    p) \
					default: CNOP; \
				} \
			    } else { \
				switch ((p) >> 3) { \
					_PHS_c_01 (    p) \
					_PHS_c_02 (    p) \
					_PHS_c_03 (    p) \
					_PHS_c_04 (    p) \
					_PHS_c_05 (    p) \
					_PHS_c_06 (    p) \
					_PHS_c_07 (    p) \
					_PHS_c_08 (    p) \
					_PHS_c_09 (    p) \
					_PHS_c_10 (    p) \
					_PHS_c_11 (    p) \
					_PHS_c_J  (    p) \
					default: CNOP; \
				} \
			    } \
			} while (0)

#define	_PPS(p,v)	do { \
			    if (v) { \
				switch ((p) >> 3) { \
					_PPS_s_01 (    p) \
					_PPS_s_02 (    p) \
					_PPS_s_03 (    p) \
					_PPS_s_04 (    p) \
					_PPS_s_05 (    p) \
					_PPS_s_06 (    p) \
					_PPS_s_07 (    p) \
					_PPS_s_08 (    p) \
					_PPS_s_09 (    p) \
					_PPS_s_10 (    p) \
					_PPS_s_11 (    p) \
					_PPS_s_J  (    p) \
					default: CNOP; \
				} \
			    } else { \
				switch ((p) >> 3) { \
					_PPS_c_01 (    p) \
					_PPS_c_02 (    p) \
					_PPS_c_03 (    p) \
					_PPS_c_04 (    p) \
					_PPS_c_05 (    p) \
					_PPS_c_06 (    p) \
					_PPS_c_07 (    p) \
					_PPS_c_08 (    p) \
					_PPS_c_09 (    p) \
					_PPS_c_10 (    p) \
					_PPS_c_11 (    p) \
					_PPS_c_J  (    p) \
					default: CNOP; \
				} \
			    } \
			} while (0)

#define	_PH(p)		( (  ( \
				(_PH____01 (   p) : \
				(_PH____02 (   p) : \
				(_PH____03 (   p) : \
				(_PH____04 (   p) : \
				(_PH____05 (   p) : \
				(_PH____06 (   p) : \
				(_PH____07 (   p) : \
				(_PH____08 (   p) : \
				(_PH____09 (   p) : \
				(_PH____10 (   p) : \
				(_PH____11 (   p) : \
				(_PH____J  (   p) : \
					0 )))))))))))) ) >> ((p) & 7)) & 1)

#define	_PP(p)		( (  ( \
				(_PP____01 (   p) : \
				(_PP____02 (   p) : \
				(_PP____03 (   p) : \
				(_PP____04 (   p) : \
				(_PP____05 (   p) : \
				(_PP____06 (   p) : \
				(_PP____07 (   p) : \
				(_PP____08 (   p) : \
				(_PP____09 (   p) : \
				(_PP____10 (   p) : \
				(_PP____11 (   p) : \
				(_PP____J  (   p) : \
					0 )))))))))))) ) >> ((p) & 7)) & 1)

#endif	/* P1DS */

#endif
