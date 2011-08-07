#ifndef __pg_sht_xx_cmd_h
#define __pg_sht_xx_cmd_h

// Commands for the SHT sensor

#define	SHTXX_TEMP	0
#define	SHTXX_HUMID	1

#define	SHTXX_CMD_TEMP	0x03	// Read temperature
#define	SHTXX_CMD_HUMID	0x05	// Read humidity
#define	SHTXX_CMD_RESET	0x1E	// Soft reset
#define	SHTXX_CMD_WSR	0x06	// Write status register
#define	SHTXX_CMD_RSR	0x07	// Read status register

#define	SHTXX_DELAY_TEMP	210	// milliseconds (measurement delay)
#define	SHTXX_DELAY_HUMID	55

#endif
