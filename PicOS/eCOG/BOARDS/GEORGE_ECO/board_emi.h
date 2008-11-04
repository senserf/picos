/*
 * External memory for GEORGE's board
 */

// N01L083WC2A
#define	SDRAM_END		0x10000L	/* 64 K words */

// 0x0A88
// 0x3AAA
#define	sdram_init	do { \
 \
	rg.emi.bus_cfg1 = 0x0008; \
	rg.emi.bus_cfg2 = 0x3AAA; \
	rg.emi.ctrl_sts = 0x0020; \
 \
			} while (0)

