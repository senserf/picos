//+++ "board_counter.c"

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x00
#define	PIN_DEFAULT_P2DIR	0xE0
#define	PIN_DEFAULT_P3DIR	0x00
#define	PIN_DEFAULT_P4DIR	0xFF	// Reserved for parallel USB interface
#define	PIN_DEFAULT_P5DIR	0x00
#define	PIN_DEFAULT_P6DIR	0x00

#define	PIN_LIST	{	\
	PIN_DEF (P2, 7),	\
	PIN_DEF	(P3, 0),	\
	PIN_DEF	(P3, 1),	\
	PIN_DEF	(P5, 4),	\
	PIN_DEF	(P5, 5),	\
	PIN_DEF	(P5, 6),	\
	PIN_DEF	(P5, 7),	\
}

#define	PIN_MAX			7	// Number of pins
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0x0

#define	enable_heart_rate_counter	do { \
						_BIC (P1IES, 0x02); \
						_BIS (P1IE, 0x02); \
					} while (0)

#define	disable_heart_rate_counter	_BIC (P1IE, 0x02)

#define	clear_heart_rate_counter_int	_BIC (P1IFG, 0x02)

#define	heart_rate_counter_int		(P1IFG & 0x02)
