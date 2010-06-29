#include "board_rf.h"

// Sensor connections:
//
// Port D2-D3 -> DIO2-3 (J2 4,6,8) D2 = SHT DATA, D3 = SHT CLOCK
// ADC input  -> Vin2   (J2 7)

#define	EMI_USED		0

/* ====== */
/* PORT A */
/* ====== */
	/*
	 * Port A as GPIO 0-7, needed for the LEDs on A3-4. Why is A5
	 * pulled up through a 3.3K resistor? A6 triggers battery test.
	 * A1,2,7 disconnected (unfortunately).
	 */
#define	PORT_A_ENABLE		PORT_EN_A_MASK
#define	PORT_A_SELECT		fd.port.sel1.a = 11

	/*
	 * B3-B5 as GPIO 8-10, B0-B2 -> SPI (SCLK, MOSI, MISO), B6,B7 ->
	 * USART (DATA_IN, DATA_OUT); B3-B5 needed by LCD (see also Port J)
	 * CC1000 doesn't want to work without it, although it doesn't
	 * need Port B. Strange.
	 * Note that B3-B5 are usable, unfortunately, the whole port B is
	 * disconnected.
	 */
#define	PORT_B_ENABLE		PORT_EN_B_MASK
#define	PORT_B_SELECT		fd.port.sel1.b = 3

#if 	SWITCHES
	/*
	 * Port C (4 signals) as GPIO13-16. Conflicts with Port L, except
	 * for C3 (GPIO16).
	 * Disabled. Perhaps we can use sel 3 to reclaim C3 -> GPIO16
	 * without conflicts?
	 */
#define	PORT_C_ENABLE		PORT_EN_C_MASK
#define PORT_C_SELECT		fd.port.sel1.c = 7
	// Use switches, make GPIO 4,5,13,14 input
#define	SWITCHES_ENABLE		do { \
	  rg.io.gp4_7_out = IO_GP4_7_OUT_DIS4_MASK | IO_GP4_7_OUT_DIS5_MASK; \
	  rg.io.gp12_15_out = IO_GP12_15_OUT_DIS13_MASK | \
		IO_GP12_15_OUT_DIS14_MASK; \
				} while (0)
#endif

/*
 * D_1 unusable (the router setting of 3 conflicts with port L needed by RF)
 * GPIO21 -> D_2 -> DIO_2
 * GPIO22 -> D_3 -> DIO_3
 * D2-3 could be used as UART B
 */
#define	PORT_D_ENABLE		PORT_EN_D_MASK
#define	PORT_D_SELECT		fd.port.sel1.d = 1;

#ifdef	EMI_USED
	/*
	 * Ports E-I are used by EMI
	 */
#define	PORT_E_ENABLE		PORT_EN_E_MASK
#define	PORT_E_SELECT		fd.port.sel1.e = 0
#endif

#ifdef	EMI_USED
	/* Port F as A8-15 (address lines that is). */
#define	PORT_F_ENABLE		PORT_EN_F_MASK
#define	PORT_F_SELECT		fd.port.sel1.f = 0
#endif

#ifdef	EMI_USED
	/* Port G as D0-7 (data lines that is). */
#define	PORT_G_ENABLE		PORT_EN_G_MASK
#define	PORT_G_SELECT		fd.port.sel2.g = 0
#endif

#ifdef	EMI_USED
	/* Port H as D8-D15 (more data lines). */
#define	PORT_H_ENABLE		PORT_EN_H_MASK
#define	PORT_H_SELECT		fd.port.sel2.h = 0
#endif

#ifdef	EMI_USED
	/* Port I as EMI control lines. */
#define	PORT_I_ENABLE		PORT_EN_I_MASK
#define	PORT_I_SELECT		fd.port.sel2.i = 0
#endif

	/*
	 * Port J as a DUART A (J0-J1), side effect: J2-J5 -> GPIO 17-20, with
	 * GPIO 17-20 being used by the LCD.
	 * J6-J7 -> PWM 1-2
	 * J2-5 could be used but are disconnected.
	 */
#define	PORT_J_ENABLE		PORT_EN_J_MASK
#define	PORT_J_SELECT		fd.port.sel2.j = 3

/*
 * L3-7 on GPIO_11-15 (not used by anything else) L0-2 on GPIO8-10 conflict
 * with Port B.
 */
#define	PORT_L_ENABLE		PORT_EN_L_MASK
#define	PORT_L_SELECT		fd.port.sel2.l = 2

// ============================================================================

#include "sht_xx.h"
#include "analog_sensor.h"
#include "sensors.h"

//
// I am not sure if this is going to work for PAR. The "standard" setting
// (QSO_PAR_PIN = 0x1) may use a better calibrated reference, but the minimum
// voltage on the sensor to show up as nonzero is 0.2V (way too much for PAR).
// The only other option that makes some sense for George's board is 9, i.e.,
// using Vin4 as a reference, which is roughly 0.66V (may be right, as 0.4V
// is the maximum) except that the quality of that reference is going to be
// poor.
//
// See page 23-4 of the manual
//
#define	QSO_PAR_PIN	0x9	// 1 = Vin2 - VRef, 9 = Vin2 - Vin4
#define	QSO_PAR_SHT	1	// High-ref clock
#define	QSO_PAR_ISI	1	// Inter sample interval indicator
#define	QSO_PAR_NSA	6	// Number of samples, corresponds to 512

#define	SENSOR_LIST	{ \
		ANALOG_SENSOR ( QSO_PAR_ISI,  \
				QSO_PAR_NSA,  \
				QSO_PAR_PIN,  \
				QSO_PAR_SHT), \
		DIGITAL_SENSOR (0, shtxx_init, shtxx_temp), \
		DIGITAL_SENSOR (0, NULL, shtxx_humid) \
	}

#define	SENSOR_DIGITAL
#define	SENSOR_ANALOG
#define	SENSOR_INITIALIZERS

#define	SENSOR_PAR	0
#define	SENSOR_TEMP	1
#define	SENSOR_HUMID	2

// Pin definitions for the SHT sensor

#define	shtxx_ini_regs	rg.io.gp20_23_out = IO_GP20_23_OUT_DIS21_MASK | \
					    IO_GP20_23_OUT_EN22_MASK | \
				    	    IO_GP20_23_OUT_CLR21_MASK | \
				    	    IO_GP20_23_OUT_CLR22_MASK

// GP21 is permanently low, DATA is pulled down when set to output
#define	shtxx_dtup	rg.io.gp20_23_out = IO_GP20_23_OUT_DIS21_MASK
#define	shtxx_dtdown	rg.io.gp20_23_out = IO_GP20_23_OUT_EN21_MASK
#define	shtxx_dtin	shtxx_dtup
#define	shtxx_dtout	CNOP
#define	shtxx_data	(rg.io.gp16_23_sts & 0x400)

#define	shtxx_ckup	rg.io.gp20_23_out = IO_GP20_23_OUT_SET22_MASK
#define	shtxx_ckdown	rg.io.gp20_23_out = IO_GP20_23_OUT_CLR22_MASK
