/* ====== */
/* PORT A */
/* ====== */

#define	EMI_USED		1
	/*
	 * Port A as GPIO 0-7, needed for the LEDs on A0-A3, side effect:
	 * A4-A7 -> GPIO 4-7 (used by the Ethernet chip).
	 */
#define	PORT_A_ENABLE		PORT_EN_A_MASK
#define	PORT_A_SELECT		fd.port.sel1.a = 11

	/*
	 * B3-B5 as GPIO 8-10, B0-B2 -> SPI (SCLK, MOSI, MISO), B6,B7 ->
	 * USART (DATA_IN, DATA_OUT); B3-B5 needed by LCD (see also Port J)
	 * CC1000 doesn't want to work without it, although it doesn't
	 * need Port B. Strange.
	 */
#define	PORT_B_ENABLE		PORT_EN_B_MASK
#define	PORT_B_SELECT		fd.port.sel1.b = 3

#if 	SWITCHES
	/*
	 * Port C (4 signals) as GPIO13-16. Conflicts with Port L, except
	 * for C3 (GPIO16).
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

#if	UART_DRIVER == 2
	/*
	 * Port D as DUART B (D0-D1), side effect: D2-D3 -> GPIO 21-22. D2 is
	 * used by XEMICS.
	 */
#define	PORT_D_ENABLE		PORT_EN_D_MASK
#define	PORT_D_SELECT		fd.port.sel1.d = 1
#endif

	/*
	 * Ports E-I are used by EMI
	 */
#define	PORT_E_ENABLE		PORT_EN_E_MASK
#define	PORT_E_SELECT		fd.port.sel1.e = 0
#define	PORT_F_ENABLE		PORT_EN_F_MASK
#define	PORT_F_SELECT		fd.port.sel1.f = 0
#define	PORT_G_ENABLE		PORT_EN_G_MASK
#define	PORT_G_SELECT		fd.port.sel2.g = 0
#define	PORT_H_ENABLE		PORT_EN_H_MASK
#define	PORT_H_SELECT		fd.port.sel2.h = 0
#define	PORT_I_ENABLE		PORT_EN_I_MASK
#define	PORT_I_SELECT		fd.port.sel2.i = 0

	/*
	 * Port J as a DUART A (J0-J1), side effect: J2-J5 -> GPIO 17-20, with
	 * GPIO 17-20 being used by the LCD.
	 * J6-J7 -> PWM 1-2
	 */
#define	PORT_J_ENABLE		PORT_EN_J_MASK
#define	PORT_J_SELECT		fd.port.sel2.j = 3

/*
 * L3-7 on GPIO_11-15 for CC1000 (not used by anything else) L0-2 on GPIO8-10
 * conflicts with Port B.
 */
#define	PORT_L_ENABLE		PORT_EN_L_MASK
#define	PORT_L_SELECT		fd.port.sel2.l = 2