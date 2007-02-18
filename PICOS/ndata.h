#ifndef	__ndata_h__
#define	__ndata_h__

/*
 * Structures used to describe packages of input data pertaining to
 * specific modules. We want to pass their pointers as arguments to
 * respective constructors. Those constructors, as a matter of principle,
 * will deallocate the structures and those of their attributes that they
 * will not use directly (like, possibly, some const arrays).
 * The reason we use such packages is that oterwise the list of arguments
 * of the node constructor would be very long and messy.
 */

typedef	struct {

// RF module

	double	XP, RP;		// Xmit power, receiver sensitivity
	double	LBTThs;		// LBT threshold
	word	BCMin, BCMax,	// Minimum and maximum backoff
		LBTDel,		// LBT delay
		Pre;		// Preamble length
} data_rf_t;

#define	EP_N_BOUNDS		6

typedef struct {

// EEPROM + IFLASH (FIM)

	lword	EEPRS,		// EEPROM size in bytes
		EEPPS;		// EEPROM page size

	FLAGS	EFLGS;

	// Write/erase/sync timing bounds, 6 numbers:
	// byte write time (min, max), byte erase time (min, max), sync time
	// (min, max)
	double	bounds [EP_N_BOUNDS];

	word	IFLSS,		// IFLASH size
		IFLPS;		// IFLASH page size

	Boolean	absent;		// Flag == explicitly absent
} data_ep_t;

typedef struct {

// UART

	FLAGS	UMode;		// Mode bits
	word 	UIBSize,	// Buffer size
		UOBSize,
		URate;		// Rate

	char	*UIDev,
		*UODev;		// Input/output devices
	Boolean	absent;		// Flag == explicitly absent
} data_ua_t;

typedef struct {

// PINS
	FLAGS	PMode;

	byte	NP,		// Total number of pins
		NA,		// Number of analog pins
		MPIN,		// Pulse monitor pin
		NPIN,		// Notifier pin
		D0PIN,		// DAC pin 0
		D1PIN;		// DAC pin 1

	const byte	*ST,	// Status
			*IV;	// Default input values
	const short	*VO;	// Default input voltage

	const char	*PIDev,	// Input device
			*PODev;	// Output device
	Boolean	absent;		// Flag == explicitly absent

} data_pn_t;

typedef	struct {

// LEDs module

	const char	*LODev;	// NULL == no output file, i.e., socket
	word		NLeds;	// The number of LEDS

	Boolean	absent;		// Flag == explicitly absent

} data_le_t;

typedef struct {
/*
 * This is a set of parameters describing a node. By keeping everything
 * in one place, we can simplify constructor headers and avoid some mess
 * during network construction stage.
 */
	word	Mem;		// Memory
	double X, Y;		// Coordinates
	data_rf_t *rf;		// RF module data parameters
	data_ep_t *ep;		// EEPROM parameters
	data_ua_t *ua;		// UART parameters
	data_pn_t *pn; 		// PINS module parameters
	data_le_t *le;		// LEDs module

} data_no_t;

#endif
