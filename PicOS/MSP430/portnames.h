#ifndef __pg_portnames_h
#define	__pg_portnames_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Transformations of port operations, such that port pins can be
// parameterized by constants. This sucks.

#define	P1_0		10
#define	P1_1		11
#define	P1_2		12
#define	P1_3		13
#define	P1_4		14
#define	P1_5		15
#define	P1_6		16
#define	P1_7		17

#define	P2_0		20
#define	P2_1		21
#define	P2_2		22
#define	P2_3		23
#define	P2_4		24
#define	P2_5		25
#define	P2_6		26
#define	P2_7		27

#define	P3_0		30
#define	P3_1		31
#define	P3_2		32
#define	P3_3		33
#define	P3_4		34
#define	P3_5		35
#define	P3_6		36
#define	P3_7		37

#define	P4_0		40
#define	P4_1		41
#define	P4_2		42
#define	P4_3		43
#define	P4_4		44
#define	P4_5		45
#define	P4_6		46
#define	P4_7		47

#define	P5_0		50
#define	P5_1		51
#define	P5_2		52
#define	P5_3		53
#define	P5_4		54
#define	P5_5		55
#define	P5_6		56
#define	P5_7		57

#define	P6_0		60
#define	P6_1		61
#define	P6_2		62
#define	P6_3		63
#define	P6_4		64
#define	P6_5		65
#define	P6_6		66
#define	P6_7		67

//
// The compiler will optimize this garbage out as long as you are using absolute
// constants, which you will
//
#define	_PDS(p,v)	do { \
			  if ((p) != 0) { \
			    if (v) { \
				switch ((p) / 10) { \
				  case 1: _BIS (P1DIR, 1 << (p % 10)); break; \
				  case 2: _BIS (P2DIR, 1 << (p % 10)); break; \
				  case 3: _BIS (P3DIR, 1 << (p % 10)); break; \
				  case 4: _BIS (P4DIR, 1 << (p % 10)); break; \
				  case 5: _BIS (P5DIR, 1 << (p % 10)); break; \
				  case 6: _BIS (P6DIR, 1 << (p % 10)); break; \
				  default : \
					syserror (EREQPAR, "_PDS"); \
				} \
			    } else { \
				switch ((p) / 10) { \
				  case 1: _BIC (P1DIR, 1 << (p % 10)); break; \
				  case 2: _BIC (P2DIR, 1 << (p % 10)); break; \
				  case 3: _BIC (P3DIR, 1 << (p % 10)); break; \
				  case 4: _BIC (P4DIR, 1 << (p % 10)); break; \
				  case 5: _BIC (P5DIR, 1 << (p % 10)); break; \
				  case 6: _BIC (P6DIR, 1 << (p % 10)); break; \
				  default : \
					syserror (EREQPAR, "_PDS"); \
				} \
			    } \
			  } \
			} while (0)

#define	_PVS(p,v)	do { \
			  if ((p) != 0) { \
			    if (v) { \
				switch ((p) / 10) { \
				  case 1: _BIS (P1OUT, 1 << (p % 10)); break; \
				  case 2: _BIS (P2OUT, 1 << (p % 10)); break; \
				  case 3: _BIS (P3OUT, 1 << (p % 10)); break; \
				  case 4: _BIS (P4OUT, 1 << (p % 10)); break; \
				  case 5: _BIS (P5OUT, 1 << (p % 10)); break; \
				  case 6: _BIS (P6OUT, 1 << (p % 10)); break; \
				  default : \
					syserror (EREQPAR, "_PDS"); \
				} \
			    } else { \
				switch ((p) / 10) { \
				  case 1: _BIC (P1OUT, 1 << (p % 10)); break; \
				  case 2: _BIC (P2OUT, 1 << (p % 10)); break; \
				  case 3: _BIC (P3OUT, 1 << (p % 10)); break; \
				  case 4: _BIC (P4OUT, 1 << (p % 10)); break; \
				  case 5: _BIC (P5OUT, 1 << (p % 10)); break; \
				  case 6: _BIC (P6OUT, 1 << (p % 10)); break; \
				  default : \
					syserror (EREQPAR, "_PDS"); \
				} \
			    } \
			  } \
			} while (0)

#define	_PV(p)		(    ( \
				((p)/10 == 1 ? P1IN : \
				((p)/10 == 2 ? P2IN : \
				((p)/10 == 3 ? P3IN : \
				((p)/10 == 4 ? P4IN : \
				((p)/10 == 5 ? P5IN : \
				((p)/10 == 6 ? P6IN : \
				0 )))))) & (1 << (p % 10))))
			
#endif