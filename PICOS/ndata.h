#ifndef	__ndata_h__
#define	__ndata_h__

/*
 * Structures used to describe packages of input data pertaining to
 * specific modules. We want to pass their pointers as arguments to
 * respective constructors. Those constructors, as a matter of principle,
 * will deallocate the structures and those of their attributes that they
 * will not use directly (like, possibly, some const arrays).
 * The reason we use such packages is that otherwise the list of arguments
 * of the node constructor would be very long and messy.
 */

typedef	struct {

// RF module

	double	Boost,		// Receiver boost
		*LBTThs;	// LBT thresholds; the number equals LBTTries
				// ... stored as strpool items
	word	Rate,		// Rate select
		Power,		// Power select
		Channel,	// Channel number
		BCMin, BCMax,	// Minimum and maximum backoff
		LBTDel,		// LBT delay
		LBTTries,	// Maximum number of attempts
		Pre;		// Preamble length

	Boolean absent;		// Explicitly absent

} data_rf_t;

#define	EP_N_BOUNDS		8
#define	RF_N_THRESHOLDS		16

struct data_epini_struct {

// EEPROM/IFLASH initializer

	struct data_epini_struct *Next;	// They can be linked

	byte	*chunk;		// Data or filename
	lword	Size;		// Chunk size (if zero == filename)
	lword	Address;	// Starting address

};

typedef	struct data_epini_struct data_epini_t;

typedef struct {

// EEPROM + IFLASH (FIM); perhaps they should be separate

	lword	EEPRS,		// EEPROM size in bytes
		EEPPS;		// EEPROM page size

	FLAGS	EFLGS;

	// Read/write/erase/sync timing bounds, 8 numbers:
	//
	// 	byte read time (min, max)
	//	byte write time (min, max)
	//	byte erase time (min, max)
	//	sync time (min, max)
	//
	double	bounds [EP_N_BOUNDS];

	// File names of "image"
	char	*EPIF,
		*IFIF;

	word	IFLSS,		// IFLASH size
		IFLPS;		// IFLASH page size

	byte	EECL,		// Empty byte content (typically 00 or FF)
		IFCL;	

	// Initializer lists
	data_epini_t *EPINI, *IFINI;

	Boolean	absent;		// Explicitly absent

} data_ep_t;

typedef struct {

// UART

	FLAGS	UMode;		// Mode bits

	word 	UIBSize,	// Buffer size
		UOBSize,
		URate;		// Rate

	char	*UIDev,
		*UODev;		// Input/output devices
	byte	iface;		// Praxis-level interface type
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
		D1PIN,		// DAC pin 1
		BPol;		// Polarity of buttons

	const byte	*ST,	// Status
			*IV,	// Default input values
			*BN;	// Button function number
	const short	*VO;	// Default input voltage

	const char	*PIDev,	// Input device
			*PODev;	// Output device

	// Debouncers: 2 for CNT, 2 for NOT, 3 for Buttons
	Long		DEB[7];	// Debouncers

	Boolean	absent;		// Flag == explicitly absent

} data_pn_t;

typedef struct {

// EMUL

	const char	*EODev;	// Output device
	Boolean 	held, absent;

} data_em_t;

class SensActDesc {

// A sensor/actuator descriptor, more like a structure

	public:

	TIME	ReadyTime;

	double	MinTime, MaxTime;

	lword	Max, Value,
		RValue;		// Reset value
	byte	Length,		// This is the value length in bytes
		Type,		// Type: SEN_TYPE_SENSOR or SEN_TYPE_ACTUATOR
		Id;		// Number in the array

	SensActDesc () {
		Length = 0;	// Default == absent
		Type = 0;
		MinTime = MaxTime = 0.0;
		ReadyTime = TIME_inf;
	};	

	// Determine the number size for a value
	static int bsize (lword);

	// Set the value
	Boolean set (lword v) {
		if (v > Max)
			v = Max;
		if (Value != v) {
			Value = v;
			return YES;
		}
		return NO;
	}
		
	Boolean expand (address v);

	// Retrieve the value
	void get (address);
	
	// Action delay
	TIME action_time () {
		return etuToItu (MinTime == MaxTime ? MinTime :
			dRndUniform (MinTime, MaxTime) );
	};
};

class data_sa_t {

// SNSRS (sensors and actuators)

	public:

	FLAGS	SMode;

	byte	NS, NA;		// Total number of sensors/actuators

	sint	SOff, AOff;	// Offsets (hidden)

	SensActDesc	*Sensors,
			*Actuators;

	const char	*SIDev,	// Input device
			*SODev;	// Output device

	Boolean absent;		// Explicitly absent

	~data_sa_t () {
		delete [] Sensors;
		delete [] Actuators;
	};

};

typedef	struct {

// LEDs module

	const char	*LODev;	// NULL == no output file, i.e., socket
	word		NLeds;	// The number of LEDS

	Boolean	absent;		// Flag == explicitly absent

} data_le_t;

#define	PWRT_N_MODULES	4

typedef struct {
	word	NStates;
	double	*Levels;
} pwr_mod_t;

typedef struct {

// Power tracker

	FLAGS		PMode;
	const char	*PIDev, *PODev;	// Input/output file selector
	Boolean		absent;		// Flag == explicitly absent
	pwr_mod_t	*Modules [PWRT_N_MODULES];

} data_pt_t;

typedef struct {
/*
 * This is a set of parameters describing a node. By keeping everything
 * in one place, we can simplify constructor headers and avoid mess
 * during network construction stage.
 */
	word	PLimit;		// Process table size
	word	Mem;		// Memory
	word	On;		// Initially on
	word	Lcdg;		// This will do for now as a flag
	lword	HID;
	double X, Y;		// Coordinates
	Boolean	Movable,
		HID_present;
	data_rf_t *rf;		// RF module data parameters
	data_ep_t *ep;		// EEPROM parameters
	data_ua_t *ua;		// UART parameters
	data_pn_t *pn; 		// PINS module parameters
	data_sa_t *sa;
	data_le_t *le;		// LEDs module
	data_em_t *em;		// Emulator output
	data_pt_t *pt;		// Power tracker

} data_no_t;

#endif
