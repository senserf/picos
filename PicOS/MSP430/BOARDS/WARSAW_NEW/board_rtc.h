#define	rtc_clk_delay	udelay (2)

// Both CLK and DATA are open drain, so we never pull them up from here; this
// assumes that their OUT bits are both zero
#define	rtc_clkh	do { rtc_clk_delay; _BIC (P2DIR, 0x02); } while (0)
#define	rtc_clkl	do { rtc_clk_delay; _BIS (P2DIR, 0x02); } while (0)
#define	rtc_set_input	_BIC (P2DIR, 0x80)
#define	rtc_outh	rtc_set_input
#define	rtc_outl	_BIS (P2DIR, 0x80)
#define	rtc_inp		(P2IN & 0x80)
