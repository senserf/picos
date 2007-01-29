/*
 * Pin assignment (MT29XXX <-> MSP430-xxx)
 *
 *  MT29XXX		        MSP430
 *  =================================
 *  WP				P2.0	->
 *  WE				P2.1	->
 *  ALE				P2.2	->
 *  CLE				P2.3	->
 *  CE				P2.4	->
 *  RE 				P2.5	->
 *  RY/BY			P2.6	<-
 *  D0-D7			P4.0-7
 */

#include "board_pins.h"
#include "storage_mt29xxx.h"

#define	EE_NBLOCKS	2048	// Determines actual storage size

#define	ee_ini_regs	do { \
				_BIS (P2OUT, 0x32); \
				_BIC (P2OUT, 0x0d); \
				_BIS (P2DIR, 0x3f); \
				_BIC (P2DIR, 0x40); \
				P4DIR = 0xff; \
			} while (0)

#define	ee_wp_high	_BIS (P2OUT, 0x01)
#define	ee_wp_low	_BIC (P2OUT, 0x01)
#define	ee_wp_val	(P2OUT & 0x01)
#define	ee_we_high	_BIS (P2OUT, 0x02)
#define	ee_we_low	_BIC (P2OUT, 0x02)
#define	ee_ale_high	_BIS (P2OUT, 0x04)
#define	ee_ale_low	_BIC (P2OUT, 0x04)
#define	ee_cle_high	_BIS (P2OUT, 0x08)
#define	ee_cle_low	_BIC (P2OUT, 0x08)
#define	ee_ce_high	_BIS (P2OUT, 0x10)
#define	ee_ce_low	_BIC (P2OUT, 0x10)
#define	ee_re_high	_BIS (P2OUT, 0x20)
#define	ee_re_low	_BIC (P2OUT, 0x20)
#define	ee_ready	(P2IN & 0x40)

#define	ee_set_input	(P4DIR = 0x00)
#define	ee_set_output	(P4DIR = 0xff)

#define	ee_start	ee_ce_low
#define	ee_stop		ee_ce_high

#define	ee_strobe_out	do { \
				ee_we_low; ee_we_low; \
				ee_we_high; ee_we_high; \
			} while (0)

#define	ee_strobe_in	do { \
				ee_re_low; ee_re_low; \
				ee_re_high; ee_re_high; \
			} while (0)

#define	ee_cmd_mode	do { ee_ale_low; ee_cle_high; } while (0)
#define	ee_adr_mode	do { ee_cle_low; ee_ale_high; } while (0)
#define	ee_dat_mode	do { ee_cle_low; ee_ale_low;  } while (0)

#define	ee_obuf		P4OUT
#define	ee_ibuf		P4IN

