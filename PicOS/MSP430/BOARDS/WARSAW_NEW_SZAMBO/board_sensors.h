//
// One analog sensor for the MaxBotix MB7001 WR1 ultrasound distance sensor
// (analog interface)
//

#include "analog_sensor.h"
#include "sensors.h"

#define	PIN_DEFAULT_P6SEL	0x01	// ADC version of ECHO

// ============================================================================

#define	ECHO_SHT	4	// Sample hold time indicator
#define	ECHO_ISI	1	// Inter sample interval indicator
#define	ECHO_NSA	512	// Number of samples
#define	ECHO_URE	SREF_AVCC_AVSS	// Voltage reference: Veref
#define	ECHO_ERE	0	// No exported reference
#define	ECHO_PIN	0

#define	SENSOR_LIST { \
		ANALOG_SENSOR ( ECHO_ISI,  \
				ECHO_NSA,  \
				ECHO_PIN,  \
				ECHO_URE,  \
				ECHO_SHT,  \
				ECHO_ERE), \
	}

#define	sensor_adc_prelude(p) CNOP
#define	sensor_adc_postlude(p) CNOP

// Defining these brings up second UART operations in Apps/TESTS/WARSAW to test
// the RS232 interface of MB7001 WR1; it doesn't work, looks like inverted
// signal polarity, and there seems to be no way to invert the polarity on the
// micro side
#define	gps_bring_up	CNOP
#define	gps_bring_down	CNOP

// #define	SENSOR_INITIALIZERS
// #define	SENSOR_DIGITAL
#define	SENSOR_ANALOG		// To make sure analog sensors are processed
