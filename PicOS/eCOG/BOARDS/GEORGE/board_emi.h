/*
 * External memory for GEORGE's board
 */

#if 0
// SRAM: BS62LV4000 (512 KB = 256 KW)

#define	SDRAM_END		0x40000L	/* External memory size */
#endif

#define	SDRAM_END		0x10000L	/* 64 K words only */

// 0x0A88
// 0x3AAA
#define	sdram_init	do { \
 \
	rg.emi.bus_cfg1 = 0x0008; \
	rg.emi.bus_cfg2 = 0x3AAA; \
	rg.emi.ctrl_sts = 0x0020; \
 \
			} while (0)

