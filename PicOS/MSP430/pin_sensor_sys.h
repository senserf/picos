#ifndef	__pin_sensor_sys_h
#define	__pin_sensor_sys_h

#ifdef	INPUT_PIN_P1_IRQ
#define	__pinsen_clear_p1_irq	_BIC (P1IFG, INPUT_PIN_P1_IRQ)
#define	__pinsen_enable_p1_irq	_BIS (P1IE, INPUT_PIN_P1_IRQ)
#define	__pinsen_disable_p1_irq	_BIC (P1IE, INPUT_PIN_P1_IRQ)
#else
#define	__pinsen_clear_p1_irq	CNOP
#define	__pinsen_enable_p1_irq	CNOP
#define	__pinsen_disable_p1_irq	CNOP
#endif

#ifdef	INPUT_PIN_P2_IRQ
#define	__pinsen_clear_p2_irq	_BIC (P2IFG, INPUT_PIN_P2_IRQ)
#define	__pinsen_enable_p2_irq	_BIS (P2IE, INPUT_PIN_P2_IRQ)
#define	__pinsen_disable_p2_irq	_BIC (P2IE, INPUT_PIN_P2_IRQ)
#else
#define	__pinsen_clear_p2_irq	CNOP
#define	__pinsen_enable_p2_irq	CNOP
#define	__pinsen_disable_p2_irq	CNOP
#endif

#define	__pinsen_clear_irq	do { \
					__pinsen_clear_p1_irq; \
					__pinsen_clear_p2_irq; \
				} while (0)

#define	__pinsen_enable_irq	do { \
					__pinsen_enable_p1_irq; \
					__pinsen_enable_p2_irq; \
				} while (0)

#define	__pinsen_disable_irq	do { \
					__pinsen_disable_p1_irq; \
					__pinsen_disable_p2_irq; \
				} while (0)

#ifdef	INPUT_PIN_P1_EDGES
#define	__pinsen_setedge_p1_irq P1IES = (P1IES & ~INPUT_PIN_P1_IRQ) | \
					INPUT_PIN_P1_EDGES
#else
#define	__pinsen_setedge_p1_irq	CNOP
#endif

#ifdef	INPUT_PIN_P2_EDGES
#define	__pinsen_setedge_p2_irq P2IES = (P2IES & ~INPUT_PIN_P2_IRQ) | \
					INPUT_PIN_P2_EDGES
#else
#define	__pinsen_setedge_p2_irq	CNOP
#endif

#define	__pinsen_setedge_irq	do { \
					__pinsen_setedge_p1_irq; \
					__pinsen_setedge_p2_irq; \
				} while (0)
#endif
