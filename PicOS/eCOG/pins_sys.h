#ifndef	__pins_sys_h
#define	__pins_sys_h

#include "board_pins.h"

/*
 * ADC configuration for polled sample collection
 */

// Note: on the eCOG, p includes reference specification (it is not just
// a pin number); the role of t is to select high/1 or low/0 reference clock;
// r is ignored
#define	adc_config_read(p,r,t)	do { \
				  fd.ssm.ex_ctrl.adc_rst_set = 1; \
				  if (t) \
					fd.ssm.ex_ctrl.adc_high_ref_clk = 1; \
				  else \
					fd.ssm.ex_ctrl.adc_low_pll_clk = 1; \
				  rg.adc.cfg = (p); \
				  fd.ssm.ex_ctrl.adc_rst_clr = 1; \
				  fd.adc.ctrl.int_dis = 1; \
				} while (0)
#ifdef PIN_ADC_RSSI

// ADC used to collect RSSI

#define	adc_config_rssi	do { \
				fd.ssm.ex_ctrl.adc_rst_set = 1; \
				fd.ssm.ex_ctrl.adc_high_ref_clk = 1; \
				rg.adc.cfg = PIN_ADC_RSSI; \
				fd.ssm.ex_ctrl.adc_rst_clr = 1; \
				fd.adc.ctrl.int_dis = 1; \
			} while (0)

// Used by the RF module
#define	adc_rcvmode	(rg.adc.cfg == PIN_ADC_RSSI)

#else	/* NO ADC RSSI */

#define	adc_config_rssi	CNOP
#define	adc_rcvmode	0

#endif	/* ADC RSSI */

// Result not available
#define	adc_busy	((fd.adc.sts.rdy == 0) || \
			    (zz_systat.adcval = (word)(fd.adc.sts.data), 0))

// Wait for result
#define	adc_wait	do { } while (adc_busy)

// ADC is on
#define	adc_inuse	(fd.ssm.cfg.adc_en != 0)

// Explicit end of sample indication (no such thing on eCOG)
#define	adc_stop	CNOP

// Off but possibly less than disable (same thing on eCOG)
#define	adc_off		do { \
				fd.ssm.cfg.adc_en = 0; \
				fd.adc.ctrl.int_clr = 1; \
			} while (0)

// Same as off
#define	adc_disable	adc_off

#define	adc_value	(zz_systat.adcval+((zz_systat.adcval & 0x0800) ? \
				0xF800 : 0x800))

#define	adc_start	do { fd.ssm.cfg.adc_en = 1; } while (0)

#define adc_advance	do { \
				if (fd.adc.sts.rdy != 0)  \
			    		zz_systat.adcval = \
						(word)(fd.adc.sts.data); \
			} while (0)

#endif
