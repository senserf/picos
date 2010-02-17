#ifndef	__pg_sensors_sys_h
#define	__pg_sensors_sys_h

#include "sysio.h"

// ============================================================================
// These structures are interchangeable and take the same amount of space; they
// are alternative layouts of the sensor array
// ============================================================================

typedef	void (*fun_sen_acc_t) (void);

typedef struct {

	fun_sen_acc_t W [3];

} i_sensdesc_t;


typedef	struct {
//
// These structures are not eCOG friendy, unfortunately, because of bytes and
// function pointers
//
	byte	tp, dummy;	// Type
	word	adcpars [3];	// ADC parameters
	word	nsamples,	// Number of samples per measurement
		dummy1;

} a_sensdesc_t;

typedef struct {

	byte tp, num;
	word dummy;
	void (*fun_val) (word, const byte*, address);
	void (*fun_ini) ();

} d_sensdesc_t;

// on eCOG, pin number includes the reference specification

#define	sensor_adc_config(a)	adc_config_read ((byte)((a)[0]), 0, a [1])

#define	DIGITAL_SENSOR(par,ini,pro) \
		{ (fun_sen_acc_t)((0x8000 + (lword)(par)) << 16 | 0), \
		  (fun_sen_acc_t)(ini), (fun_sen_acc_t)(pro) }

#define	ANALOG_SENSOR(isi,ns,pn,sht) \
		{ (fun_sen_acc_t)(((lword)(isi)) << 24 | (pn)), \
		  (fun_sen_acc_t)(((lword)(sht)) << 16), \
		  (fun_sen_acc_t) 0 }

// This one we need to identify sensors from the ADC parameters (by pin number)
// in macros defined in BOARD-specific files, e.g., to implement conditional
// preludes and postludes
#define	ANALOG_SENSOR_PIN(par)	((byte)(*par))

#endif
