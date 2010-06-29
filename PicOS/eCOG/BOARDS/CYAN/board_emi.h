/*
 * External memory for the CYAN board
 */
#define	SDRAM_END		0x800000L	/* External memory size */

// Chip select for SDRAM

#define	sdram_init	do { \
 \
	rg.emi.sdram_cfg = 0x940d; \
	rg.emi.sdram_refr_per = 0x330c; \
	rg.emi.sdram_refr_cnt = 0x0010; \
	rg.emi.ctrl_sts = 0x0804; \
	fd.emi.sdram_cust_adr.adr = 0x400; \
	rg.emi.sdram_cust_cmd = 0x7c85; \
	fd.emi.sdram_cust_adr.adr = 0x030; \
	rg.emi.sdram_cust_cmd = 0x7c01; \
	rg.emi.sdram_cust_cmd = 0x0c63; \
 \
			} while (0)
