#ifndef __pg_portnames_h
#define	__pg_portnames_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Transformations of port operations, such that port pins can be
// parameterized by constants

#define	GPIO_0		0
#define	GPIO_1		1
#define	GPIO_2		2
#define	GPIO_3		3
#define	GPIO_4		4
#define	GPIO_5		5
#define	GPIO_6		6
#define	GPIO_7		7
#define	GPIO_8		8
#define	GPIO_9		9
#define	GPIO_10		10
#define	GPIO_11		11
#define	GPIO_12		12
#define	GPIO_13		13
#define	GPIO_14		14
#define	GPIO_15		15
#define	GPIO_16		16
#define	GPIO_17		17
#define	GPIO_18		18
#define	GPIO_19		19
#define	GPIO_20		20
#define	GPIO_21		21
#define	GPIO_22		22
#define	GPIO_23		23
#define	GPIO_24		24
#define	GPIO_25		25
#define	GPIO_26		26
#define	GPIO_27		27
#define	GPIO_28		28
#define	GPIO_29		29

#define _PDS(p,v)	_PDS_base_ [((p) >> 2)] |= \
				(1 << ((((p) & 3) << 2) + 3 - v))

#define _PVS(p,v)	_PDS_base_ [((p) >> 2)] |= \
				(1 << ((((p) & 3) << 2) + 1 - v))

#define _PFS(p,v)	CNOP

#define	_PV(p)		((_PDV_base_ [((p) >> 3)] >> (((p) & 7) << 1)) & 1)

#define	_PD(p)		((_PDS_base_ [((p) >> 2)] >> ((((p) & 3) << 2) + 2)) & \
				1)
#define	_PF(p)		0

#endif
