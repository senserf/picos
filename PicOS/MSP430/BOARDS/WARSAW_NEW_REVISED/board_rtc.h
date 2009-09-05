#define	rtc_clk_delay	udelay (2)

// Both CLK and DATA are open drain, so we never pull them up from here; this
// assumes that their OUT bits are both zero
#define	rtc_clkh	do { rtc_clk_delay; _BIC (P2DIR, 0x02); } while (0)
#define	rtc_clkl	do { rtc_clk_delay; _BIS (P2DIR, 0x02); } while (0)
#define	rtc_set_input	_BIC (P2DIR, 0x80)
#define	rtc_outh	rtc_set_input
#define	rtc_outl	_BIS (P2DIR, 0x80)
#define	rtc_inp		(P2IN & 0x80)
// In case special actions are required before we start talking

// Note: for battery backup, use a pin to drive the pull-up resistors
#define	rtc_open	do { _BIS (P2OUT, 0x04); udelay (100); } while (0)
#define	rtc_close	_BIC (P2OUT, 0x04)

// Note: if the clock is not initialized, then the chip drains about 150uA,
// but the roblem goes away if the pull-ups are driven by a pin.
