#ifndef __dbgtrc_h
#define	__dbgtrc_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef	dbg_0
#if			(dbg_level & 1)
#define	dbg_0(c)	__pi_dbg (0, (word)(c))
#else
#define	dbg_0(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_1
#if			(dbg_level & 2)
#define	dbg_1(c)	__pi_dbg (1, (word)(c))
#else
#define	dbg_1(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_2
#if			(dbg_level & 4)
#define	dbg_2(c)	__pi_dbg (2, (word)(c))
#else
#define	dbg_2(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_3
#if			(dbg_level & 8)
#define	dbg_3(c)	__pi_dbg (3, (word)(c))
#else
#define	dbg_3(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_4
#if			(dbg_level & 16)
#define	dbg_4(c)	__pi_dbg (4, (word)(c))
#else
#define	dbg_4(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_5
#if			(dbg_level & 32)
#define	dbg_5(c)	__pi_dbg (5, (word)(c))
#else
#define	dbg_5(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_6
#if			(dbg_level & 64)
#define	dbg_6(c)	__pi_dbg (6, (word)(c))
#else
#define	dbg_6(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_7
#if			(dbg_level & 128)
#define	dbg_7(c)	__pi_dbg (7, (word)(c))
#else
#define	dbg_7(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_8
#if			(dbg_level & 256)
#define	dbg_8(c)	__pi_dbg (8, (word)(c))
#else
#define	dbg_8(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_9
#if			(dbg_level & 512)
#define	dbg_9(c)	__pi_dbg (9, (word)(c))
#else
#define	dbg_9(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_a
#if			(dbg_level & 1024)
#define	dbg_a(c)	__pi_dbg (10, (word)(c))
#else
#define	dbg_a(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_b
#if			(dbg_level & 2048)
#define	dbg_b(c)	__pi_dbg (11, (word)(c))
#else
#define	dbg_b(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_c
#if			(dbg_level & 4096)
#define	dbg_c(c)	__pi_dbg (12, (word)(c))
#else
#define	dbg_c(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_d
#if			(dbg_level & 8192)
#define	dbg_d(c)	__pi_dbg (13, (word)(c))
#else
#define	dbg_d(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_e
#if			(dbg_level & 16384)
#define	dbg_e(c)	__pi_dbg (14, (word)(c))
#else
#define	dbg_e(c)	do { } while (0)
#endif
#endif

#ifndef	dbg_f
#if			(dbg_level & 32768)
#define	dbg_f(c)	__pi_dbg (15, (word)(c))
#else
#define	dbg_f(f)	do { } while (0)
#endif
#endif

#if 	dbg_level != 0

#ifndef	dbg_binary
#define	dbg_binary	0
#endif

extern	void __pi_dbg (int, word);

#endif

#endif
