#ifndef	__tcv_defs_h
#define __tcv_defs_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2020                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

typedef	struct {
/*
 * Application data pointers. These two numbers represent the offset to the
 * starting byte in the application portion of a received packet, and the
 * offset of the last application byte from the end, or, in simple words,
 * the header and trailer length, respectively.
 */
	word	head,
		tail;
} tcvadp_t;

/*
 * Plugin functions
 */
typedef struct {
	int (*tcv_ope) (int, int, va_list);
	int (*tcv_clo) (int, int);
	int (*tcv_rcv) (int, address, int, int*, tcvadp_t*);
	int (*tcv_frm) (address, tcvadp_t*);
	int (*tcv_out) (address);
	int (*tcv_xmt) (address);
	int (*tcv_tmt) (address);
	int tcv_info;
} tcvplug_t;

// ============================================================================


struct __tcv_qitem_s {
	struct __tcv_qitem_s	*next,
				*prev;
};

typedef	struct __tcv_qitem_s	__tcv_qitem_t;
typedef	struct __tcv_qitem_s	__tcv_qhead_t;

// Two pointers
#define	TCV_QHEAD_LENGTH	(SIZE_OF_AWORD * 2)

struct __tcv_titem_s {
// Timer queue item
	struct __tcv_titem_s	*next,
				*prev;
	word			value;
};

typedef	struct __tcv_titem_s	__tcv_titem_t;

typedef	struct {
	__tcv_titem_t		*next,
				*prev;
} __tcv_thead_t;

typedef	union {
	word	value;
	struct	{
		word 	queued:1,
			outgoing:1,
			urgent:1,
			session:7,
			plugin:3,
			phys:3;
	} b;
} __tcv_battr_t;

/* =================== */
/* Buffer header block */
/* =================== */
struct __tcv_hblock_s {
	/*
	 * These ones must be at the very beginning
	 */
    union {
	__tcv_qitem_t	bqueue;		/* Buffer queue links */
	tcvadp_t	pointers;	/* Application data pointers */
	/*
	 * Note: the application data pointers are only valid for a packet that
	 * has been either removed from a queue, or not yet put into a queue;
	 * thus, we recycle the links for this purpose.
	 */
    } u;

#if	TCV_HOOKS
	address *hptr;
#define	TCV_HBLOCK_HOOKS_LENGTH		SIZE_OF_AWORD
#else
#define	TCV_HBLOCK_HOOKS_LENGTH		0
#endif
	/*
	 * Packet length in bytes.
	 */
	word	length;		// 2 bytes
	/*
	 * Flags (e.g., whether the packet is queued or not) + plugin ID +
	 * phys ID
	 */
	__tcv_battr_t	attributes;	// 2 bytes

#if	TCV_TIMERS
	/*
	 * Timer queue links (must be the last item, see
	 * t_tqoffset below
	 */
	__tcv_titem_t	tqueue;		// 2 pointers + word
	// Or is it SIZE_OF_AWORD * 3?
#define	TCV_HBLOCK_TIMERS_LENGTH	(SIZE_OF_AWORD * 2 + 2) 
#else
#define	TCV_HBLOCK_TIMERS_LENGTH	0
#endif
};

#define	TCV_HBLOCK_LENGTH   (TCV_QHEAD_LENGTH+4+TCV_HBLOCK_HOOKS_LENGTH+TCV_HBLOCK_TIMERS_LENGTH)

typedef	struct __tcv_hblock_s	__tcv_hblock_t;

typedef	struct {
/*
 * Session descriptor
 */
	__tcv_qhead_t		rqueue;		/* Reception queue */
	/*
	 * This is the attribute pattern word for a new outgoing packet
	 */
	__tcv_battr_t		attpattern;
	/*
	 * Note: we no longer use the notion of the currently read/written
	 * packet because the packet itself knows where it belongs.
	 */

#if TCV_OPEN_CAN_BLOCK
	/*
	 * This one is used while the session is being open and the requesting
	 * process must go to sleep. Using it, we can identify the descriptor
	 * when the open operation is resumed. Kind of clumsy, especially that
	 * I can think of no other use for this attribute.
	 */
	aword		pid;
#endif
} __tcv_sesdesc_t;

#define	TCV_SESDESC_LENGTH	(TCV_QHEAD_LENGTH + 2 + SIZE_OF_AWORD)

#define	__tcv_hblenb		(sizeof (__tcv_hblock_t))
#define __tcv_hblen		(__tcv_hblenb/sizeof(word))
#define	__tcv_header(p)		((__tcv_hblock_t*)((p) - __tcv_hblen))

#define	tcv_isurgent(p)		(__tcv_header (p) -> attributes.b.urgent)
#define	tcv_left(p)		(__tcv_header (p) -> u.pointers.tail)
#define	tcv_offset(p)		(__tcv_header (p) -> u.pointers.head)
#define	tcv_tlength(p)		(__tcv_header (p) -> length)

#define	tcv_urgent(p)		do { \
				  __tcv_header (p) -> attributes.b.urgent = 1; \
				} while (0)

#define	tcv_wnp(p,f,l)		tcv_wnps (p, f, l, NO)
#define	tcv_wnpu(p,f,l)		tcv_wnps (p, f, l, YES)

// ============================================================================

int	tcv_plug (int, const tcvplug_t*);
int	tcv_open (word, int, int, ...);
int	tcv_close (word, int);
address	tcv_rnp (word, int);
address tcv_wnps (word, int, int, Boolean);
int	tcv_qsize (int, int);
int	tcv_erase (int, int);
int	tcv_read (address, byte*, int);
int	tcv_write (address, const byte*, int);
void	tcv_endp (address);
void	tcv_drop (address);
int	tcv_control (int, int, address);

#endif
