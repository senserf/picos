#include <ecog.h>
#include <ecog1.h>
#include "kernel.h"
#include "ethernet.h"

/* ============================================================================ */
/*                       PicOS                                                  */
/*                                                                              */
/* Ethernet interface driver                                                    */
/*                                                                              */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002, 2003                 */
/*                                                                              */
/* Permission is hereby granted, free of charge, to any person obtaining a copy */
/* of this software and associated documentation files (the "Software"), to     */
/* deal in the Software without restriction, including without limitation the   */
/* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  */
/* sell copies of the Software, and to permit persons to whom the Software is   */
/* furnished to do so, subject to the following conditions:                     */
/*                                                                              */
/* The above copyright notice and this permission notice shall be included in   */
/* all copies or substantial portions of the Software.                          */
/*                                                                              */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS */
/* IN THE SOFTWARE.                                                             */
/*                                                                              */
/* ============================================================================ */


#if	ETHERNET_DRIVER

ddata_t		*zzd_data = NULL;

static const word mcastaddr [3] = ETH_DEST;

static void macrst () {
/* ===== */
/* Reset */
/* ===== */
	select (0);
	outw (RCR_SOFTRST, RCR_REG);
	select (1);
	outw (CONFIG_DEFAULT, CONFIG_REG);
	select (0);
	mdelay (10);
	/* Disable transmit and receive */
	outw (RCR_CLEAR, RCR_REG);
	outw (TCR_CLEAR, TCR_REG);
	/* Automatically release transmitted packets */
	select (1);
	outw (inw (CTL_REG) | CTL_AUTORELEASE, CTL_REG);
	/* Reset the MMU */
	select (2);
	outw (MC_RESET, MMU_CMD_REG);
	/* Disable interrupts */
	outw (0, INT_REG);
}

static void enable () {

	select (0);
	outw (TCR_ENABLE, TCR_REG);
	outw (RCR_DEFAULT, RCR_REG);
	/* Interrupts stay disabled after this; they are enabled later */
}

#if ! ECOG_SIM 

static word read_phy_reg (word phyaddr, word phyreg) {

	word *pmem, i;

/* =============================================================== */
/* I am doing this nonsense to avoid putting too much on the stack */
/* =============================================================== */
#define	bits		((byte*) pmem)
#define	clk_idx		(*(pmem + 32))
#define	input_idx	(*(pmem + 33))
#define	phydata		(*(pmem + 34))
#define	mii_reg		(*(pmem + 35))
#define	mask		(*(pmem + 36))

	pmem = umalloc (37 * 2);
	clk_idx = 0;

	// 32 consecutive ones on MDO to establish sync
	for (i = 0; i < 32; i++)
		bits [clk_idx++] = MII_MDOE | MII_MDO;

	// Start code <01>
	bits [clk_idx++] = MII_MDOE;
	bits [clk_idx++] = MII_MDOE | MII_MDO;

	// Read command <10>
	bits [clk_idx++] = MII_MDOE | MII_MDO;
	bits [clk_idx++] = MII_MDOE;

	// Output the PHY address, msb first
	mask = 0x10;
	for (i = 0; i < 5; i++) {
		if (phyaddr & mask)
			bits [clk_idx++] = MII_MDOE | MII_MDO;
		else
			bits [clk_idx++] = MII_MDOE;

		// Shift to next lowest bit
		mask >>= 1;
	}

	// Output the phy register number, msb first
	mask = 0x10;
	for (i = 0; i < 5; i++) {
		if (phyreg & mask)
			bits [clk_idx++] = MII_MDOE | MII_MDO;
		else
			bits [clk_idx++] = MII_MDOE;

		// Shift to next lowest bit
		mask >>= 1;
	}

	// Tristate and turnaround (2 bit times)
	bits [clk_idx++] = 0;

	// Input starts at this bit time
	input_idx = clk_idx;

	// Will input 16 bits
	for (i = 0; i < 16; i++)
		bits [clk_idx++] = 0;

	// Final clock bit
	bits [clk_idx++] = 0;

	// Select bank 3
	select (3);

	// Get the current MII register value
	mii_reg = inw (MII_REG);

	// Turn off all MII Interface bits
	mii_reg &= ~(MII_MDOE|MII_MCLK|MII_MDI|MII_MDO);

	// Clock all 64 cycles
	for (i = 0; i < 64; i++) {
		// Clock Low - output data
		outw (mii_reg | bits [i], MII_REG);
		// We put it at roughly 100 usec (was 50)
		udelay (100);
		// Clock Hi - input data
		outw (mii_reg | bits [i] | MII_MCLK, MII_REG);
		udelay (100);
		bits [i] |= (byte) (inw (MII_REG) & MII_MDI);
	}

	// Return to idle state
	// Set clock to low, data to low, and output tristated

	outw (mii_reg, MII_REG);
	udelay (100);

	// Recover input data
	phydata = 0;
	for (i = 0; i < 16; i++) {
		phydata <<= 1;
		if (bits [input_idx++] & MII_MDI)
			phydata |= 0x0001;
	}

	ufree (pmem);
	// phydata is still safe: deallocation destroys the first
	// word only. Arrrggghhhh!

	return phydata;

#undef	bits
#undef	clk_idx
#undef	input_idx
#undef	phydata
#undef	mii_reg
#undef	mask
}

static void write_phy_reg (word phyaddr, word phyreg, word phydata) {
	address pmem;
	word	i;

#define	bits		((byte*) pmem)
#define	clk_idx		(*(pmem + 33))
#define	mii_reg		(*(pmem + 34))
#define	mask		(*(pmem + 35))

	pmem = umalloc (36 * 2);
	clk_idx = 0;

	// 32 consecutive ones on MDO to establish sync
	for (i = 0; i < 32; i++)
		bits [clk_idx++] = MII_MDOE | MII_MDO;

	// Start code <01>
	bits [clk_idx++] = MII_MDOE;
	bits [clk_idx++] = MII_MDOE | MII_MDO;

	// Write command <01>
	bits [clk_idx++] = MII_MDOE;
	bits [clk_idx++] = MII_MDOE | MII_MDO;

	// Output the PHY address, msb first
	mask = 0x10;
	for (i = 0; i < 5; i++) {
		if (phyaddr & mask)
			bits [clk_idx++] = MII_MDOE | MII_MDO;
		else
			bits [clk_idx++] = MII_MDOE;

		// Shift to next lowest bit
		mask >>= 1;
	}

	// Output the phy register number, msb first
	mask = 0x10;
	for (i = 0; i < 5; i++) {
		if (phyreg & mask)
			bits [clk_idx++] = MII_MDOE | MII_MDO;
		else
			bits [clk_idx++] = MII_MDOE;

		// Shift to next lowest bit
		mask >>= 1;
	}

	// Tristate and turnaround (2 bit times)
	bits [clk_idx++] = 0;
	bits [clk_idx++] = 0;

	// Write out 16 bits of data, msb first
	mask = 0x8000;
	for (i = 0; i < 16; i++) {
		if (phydata & mask)
			bits [clk_idx++] = MII_MDOE | MII_MDO;
		else
			bits [clk_idx++] = MII_MDOE;

		// Shift to next lowest bit
		mask >>= 1;
	}

	// Final clock bit (tristate)
	bits [clk_idx++] = 0;

	// Select bank 3
	select (3);

	// Get the current MII register value
	mii_reg = inw (MII_REG);

	// Turn off all MII Interface bits
	mii_reg &= ~(MII_MDOE|MII_MCLK|MII_MDI|MII_MDO);

	// Clock all cycles
	for (i = 0; i < 65; i++) {
		// Clock Low - output data
		outw (mii_reg | bits[i], MII_REG);
		udelay (100);

		// Clock Hi - input data
		outw (mii_reg | bits[i] | MII_MCLK, MII_REG);
		udelay (100);
		bits [i] |= (byte) (inw (MII_REG) & MII_MDI);
	}

	// Return to idle state
	// Set clock to low, data to low, and output tristated

	outw (mii_reg, MII_REG);
	udelay (100);

	ufree (pmem);

#undef	bits
#undef	clk_idx
#undef	mii_reg
#undef	mask

}

#endif

static int ioreq_ethernet (int ope, char *buf, int len) {

	int plen, np;
	word w;

	switch (ope) {

		case READ:

			/* Disable interrupts. select (2) not needed */
			soft_din;
		ReRead:
			/* Check FIFO status */
			plen = (int) inw (RXFIFO_REG);
			if ((plen & RXFIFO_EMPTY) != 0) {
				/* Nothing available */
				zzd_data->flags |= FLG_ENRCV;
				return BLOCKED;
			}

			/* Prepare to read the packet */
			outw (PTR_READ | PTR_RCV | PTR_AUTOINC, PTR_REG);

			w = inw (DATA_REG);
			w &= RS_ERRORS;
			if (w) {
				zzd_data->error = w;
				/* Ignore the packet */
		ReIgnore:
				waitmmu (w);
				outw (MC_RELEASE, MMU_CMD_REG);
				goto ReRead;
			}

			/* Received length of the packet */
			plen = (int) inw (DATA_REG);

			/* Subtract status and length */
			plen = (plen & 0x07ff) - 4 /* status + length */;

			switch (zzd_data->readmode) {

			    case ETHERNET_MODE_COOKED:
				/*
				 * This is the structure of an incoming cooked
				 * packet:
				 *
				 *     PROTO == ETH_PTYPE 0x6006
				 *     word (0) == TYPE << 8 | n
				 *     n == 0 -> all cards receive this stuff
				 *     words (1 ... n) == card ids
				 *     word (n+1) == payload length
				 *     ... payload bytes ...
				 *
				 *     MSB of type indicates direction, with 0
				 *     meaning "down". The remaining bits are
				 *     left for future extensions with 0 meaning
				 *     regular packet (received or transmitted).
				 *     Later we may add commands, status
				 *     sensing, and so on (if they are needed
				 *     at all).
				 */
				w = inw (DATA_REG);
				/* Skip destination address */
				w = inw (DATA_REG);
				w = inw (DATA_REG);
				for (np = 0; np < 3; np++) {
					w = inw (DATA_REG);
					if ((zzd_data->flags & FLG_DSTOK) == 0) {
						/* Save server address */
						swab (w);
						zzd_data->h_dest [np] = w;
					}
				}
				/* PROTO */
				w = inw (DATA_REG);
				swab (w);
				if (w != ETH_PTYPE)
					/* Ignore this packet */
					goto ReIgnore;
			KeepCooking:
				/* Control word */
				w = inw (DATA_REG);
				swab (w);
				if (w & PTYPE_UP)
					/* This is an uplink packet */
					goto ReIgnore;
				/* Indicate we have the server's MAC address */
				zzd_data->flags |= (FLG_DSTOK | FLG_RCOOKED);
				/* Extract the actual packet type */
				np = (w & PTYPE_PTP);
				if (np != 0)
					/*
					 * This is the only type we understand
					 * at this time
					 */
					goto ReIgnore;
				/* Extract the number of card IDs */
				np = (w & PTYPE_CNT);
				/* Bytes left after cooked header */
				plen -= 14 + 2 + (np << 1) + 2;
				if (plen < 0) {
					/* Format error - too few bytes */
					zzd_data->error = ERR_FORMAT;
					goto ReIgnore;
				}
				if (np) {
					/* We have to look for our ID */
					while (1) {
						w = inw (DATA_REG);
						swab (w);
						np--;
						if (w == zzd_data->cardid) {
							while (np--)
							    w = inw (DATA_REG);
							break;
						}
						if (np <= 0)
							goto ReIgnore;
					}
				}
				/* The number of bytes */
				w = inw (DATA_REG);
				swab (w);
				if (w > plen) {
					/* Format error */
					zzd_data->error = ERR_FORMAT;
					goto ReIgnore;
				}
				plen = (int) w;
				if (len < plen)
					plen = len;
				else
					/* Make sure returned length is OK */
					len = plen;
				while (1) {
					if (plen-- <= 0)
						break;
					w = inw (DATA_REG);
					*buf++ = f_byte (w);
					if (plen-- <= 0)
						break;
					*buf++ = s_byte (w);
				}
				break;

			    case ETHERNET_MODE_RAW:

				/* RAW reception */
				if (len < plen)
					plen = len;
				else
					/* Returned length */
					len = plen;

				if (plen < 14) {
					/* Less than header */
					while (1) {
						if (plen-- <= 0)
							break;
						w = inw (DATA_REG);
						*buf++ = f_byte (w);
						if (plen-- <= 0)
							break;
						*buf++ = s_byte (w);
					}
					/* And we are done */
				} else {
					for (np = 0; np < 7; np++) {
						w = inw (DATA_REG);
						*buf++ = f_byte (w);
						*buf++ = s_byte (w);
					}
					plen -= 14;
					/* Determine the remaining length */
					swab (w);
			KeepRaw:
					if (w < plen) {
						/* PROTO means length */
						plen = w;
						len = plen + 14;
					}

					while (1) {
						if (plen-- <= 0)
							break;
						w = inw (DATA_REG);
						*buf++ = f_byte (w);
						if (plen-- <= 0)
							break;
						*buf++ = s_byte (w);
					}
				}
				zzd_data->flags &= ~FLG_RCOOKED;
				break;

			    default: /* ETHERNET_MODE_BOTH */

				/* Header */
				for (np = -3; np < 3; np++) {
					w = inw (DATA_REG);
					/*
					 * Now we store everything until we
					 * learn whether the packet is raw
					 * or cooked.
					 */
					if (len-- > 0)
						*buf++ = f_byte (w);
					if (len-- > 0)
						*buf++ = s_byte (w);
					if (np >= 0 &&
					    /* Source address */
					    (zzd_data->flags & FLG_DSTOK) == 0) {
						swab (w);
						zzd_data->h_dest [np] = w;
					}
				}

				/* PROTO */
				w = inw (DATA_REG);
				swab (w);

				if (w == ETH_PTYPE) {
					/* Cooked - reset length */
					len += 12;
					buf -= 12;
					goto KeepCooking;
				} else {
					/* Has been swapped, so s goes first */
					if (len-- > 0)
						*buf++ = s_byte (w);
					if (len-- > 0)
						*buf++ = f_byte (w);
					plen -= 14;
					if (plen > len)
						plen = len;
					else
						len = plen;
					len += 14;
					if (plen > 0)
						goto KeepRaw;
					/* Fall through */
				}

			} /* switch */

			/* Release the packet */
			waitmmu (w);
			outw (MC_RELEASE, MMU_CMD_REG);

			/* Enable interrupts */
			soft_eni;

			return len;

		case WRITE:

			if (zzd_data->writemode) {
				/*
				 * This is the structure of an outgoing cooked
				 * packet:
				 *
				 *     PROTO == ETH_PTYPE 0x6006
				 *     word (0) == TYPE << 8
				 *     word (1) == card Id
				 *     word (2) == payload length
				 *     ... payload bytes ...
				 */
				if (len < 0 || len > ETH_MXLEN - 14 - 6)
					syserror (EREQPAR, "ioreq_ether (1)");
				/* Total MAC frame length */
				plen = len + 14 + 6;
			} else {
				/* Raw mode */
				if (len < 14 || len > ETH_MXLEN)
					/* At least there should be a header */
					syserror (EREQPAR, "ioreq_ether (2)");
				plen = len;
			}

			if (plen < ETH_ZLEN)
				plen = ETH_ZLEN;

			/* Calculate the number of pages for the MMU */
			np = (plen + 6) >> 8;

			/* Disable interrupts. select (2) not needed */
			soft_din;

			waitmmu (w);
			outw (MC_ALLOC | np, MMU_CMD_REG);

			/* Quick wait for the memory */
			for (w = 5; w > 0; w++) {
				np = inw (AR_REG);
				if ((np & ((word)AR_FAILED << 8)) == 0) {
					outw (IM_ALLOC_INT /* << 8 */, INT_REG);
					break;
				}
			}

			if (w == 0) {
				/* Failure */
				zzd_data->flags |= FLG_ENXMT;
				return BLOCKED;
			}

			/* We have the memory */
			outw (((np >> 8) & 0x3f), AR_REG);

			/* Rewind the packet buffer */
			outw (PTR_AUTOINC, PTR_REG);

			outw (0, DATA_REG);
			outw (plen+6, DATA_REG);

			if (zzd_data->writemode) {
				/* Cooked mode */
				for (np = 0; np < 3; np++) {
					w = (zzd_data->flags & FLG_DSTOK) ?
						/* We have server's address */
						zzd_data->h_dest [np] :
						/* We don't, so we multicast */
						mcastaddr [np];
					swab (w);
					outw (w, DATA_REG);
				}
				/* Our address */
				for (np = 0; np < 3; np++) {
					w = macaddr [np];
					swab (w);
					outw (w, DATA_REG);
				}
				w = ETH_PTYPE;
				swab (w);
				outw (w, DATA_REG);
				w = PTYPE_UP;
				swab (w);
				outw (w, DATA_REG);
				w = zzd_data->cardid;
				swab (w);
				outw (w, DATA_REG);
				w = len;
				swab (w);
				outw (w, DATA_REG);
				/* And now for the real stuff */
				for (np = 2; np <= len; np += 2) {
					f_byte_set (w, *buf++);
					s_byte_set (w, *buf++);
					outw (w, DATA_REG);
				}
				if (len & 1) {
					f_byte_set (w, *buf);
					outw (w, DATA_REG);
					np += 14 + 6;
				} else {
					np += 14 + 4;
				}
			} else {
				/* Raw mode */
				for (np = 0; np < 3; np++) {
					/* Destination address goes first */
					f_byte_set (w, *buf++);
					s_byte_set (w, *buf++);
					outw (w, DATA_REG);
				}

				/*
				 * Our MAC address. It replaces whatever has
				 * been specified by the user.
				 */
				for (np = 0; np < 3; np++) {
					w = macaddr [np];
					swab (w);
					outw (w, DATA_REG);
				}
				buf += 6;

				for (np = 14; np <= len; np += 2) {
					f_byte_set (w, *buf++);
					s_byte_set (w, *buf++);
					outw (w, DATA_REG);
				}

				if (len & 1) {
					f_byte_set (w, *buf);
					outw (w, DATA_REG);
				} else {
					np -= 2;
				}
			}

			/* Filler */
			while (np < plen) {
				outw (0, DATA_REG);
				np += 2;
			}

			/* Status */
			outw (0, DATA_REG);

			/* Done */
			outw (MC_ENQUEUE, MMU_CMD_REG);

			/* Enable interrupts */
			soft_eni;

			/* Return the actual written frame length */
			return plen;

		case NONE:
			/*
			 * NONE - this is harmless even from the application:
			 * it just unmasks the interrupts.
			 */
			soft_eni;
			return len;

		case CONTROL:

			switch (len) {

				case ETHERNET_CNTRL_PROMISC:
					/* Set/clear promiscuous mode */
					soft_din;
					select (0);
					w = inw (RCR_REG);
					w &= ~RCR_PRMS;
					if (*buf)
						w |= RCR_PRMS;
					outw (w, RCR_REG);
					select (2);
					soft_eni;
					return 1;

				case ETHERNET_CNTRL_MULTI:
					/* Set/clear multicast reception */
					soft_din;
					select (0);
					w = inw (RCR_REG);
					w &= ~RCR_ALMUL;
					if (*buf)
						w |= RCR_ALMUL;
					outw (w, RCR_REG);
					select (2);
					soft_eni;
					return 1;

				case ETHERNET_CNTRL_SETID:
					/* Set card Id for cooked mode */
					zzd_data->cardid = *((word*)buf);
					return 1;

				case ETHERNET_CNTRL_RMODE:
					if ((zzd_data->readmode = *buf) !=
					    ETHERNET_MODE_RAW) {
						/*
						 * Make sure that ALMUL is set,
						 * because this mode makes no
						 * sense otherwise.
						 */
						soft_din;
						select (0);
						w = inw (RCR_REG);
						w |= RCR_ALMUL;
						outw (w, RCR_REG);
						select (2);
						soft_eni;
					}
					return 1;

				case ETHERNET_CNTRL_WMODE:
					zzd_data->writemode = *buf;
					return 1;

				case ETHERNET_CNTRL_GMODE:
					return
					   ((zzd_data->flags & FLG_RCOOKED) != 0);

				case ETHERNET_CNTRL_ERROR:
					/* Return error status */
					*((word*)buf) = zzd_data->error;
					zzd_data->error = 0;
					return 1;

				case ETHERNET_CNTRL_SENSE:
					/* Check is a packet is pending */
					plen = (int) inw (RXFIFO_REG);
					if ((plen & RXFIFO_EMPTY) != 0)
						/* The FIFO is empty */
						return 0;
					if (buf == NULL || *buf == 0)
						/* Just check */
						return 1;
					/* Erase */
					soft_din;
					do {
						waitmmu (w);
						outw (MC_RELEASE, MMU_CMD_REG);
						plen = (int) inw (RXFIFO_REG);
					} while ((plen & RXFIFO_EMPTY) == 0);
					soft_eni;
					return 1;

				/* Fall through */
			}

		default:
			syserror (ENOOPER, "ioreq_ether");
	}
	/* Will never get here */
	return 0;
}

void devinit_ethernet (int dummy) {

/* ============================ */
/* Initialize the Ethernet chip */
/* ============================ */

	word 	i;

#if ! ECOG_SIM
	address	pmem;
#endif

	if (zzd_data != NULL)
		/* Initialized already */
		return;

	/* ======================================================== */
	/* For the Ethernet chip, GPIO7 must be output and set high */
	/* ======================================================== */
	rg.io.gp4_7_out =
		IO_GP4_7_OUT_SET6_MASK |
		IO_GP4_7_OUT_EN6_MASK |
		IO_GP4_7_OUT_CLR7_MASK |
		IO_GP4_7_OUT_DIS7_MASK;

	/* ================================================================= */
	/* This is for chip reset. The documentation says (incorrectly) that */
	/* RESET  is  GPIO7  and  INT is GPIO6,  whereas it is the other way */
	/* around.                                                           */
	/* ================================================================= */
	fd.io.gp4_7_out.set6 = 1 ;
	fd.io.gp4_7_out.clr6 = 1 ;
	/* It will be up to the driver to enable interrupts if needed */
	tcv_hard_din;

	/* Get a chunk of private memory */
	zzd_data = (ddata_t*) umalloc (sizeof (ddata_t));
	zzd_data -> flags = 0;
	/* Cardid == 0 means RAW mode */
	zzd_data -> cardid = 0;

	mdelay (1);

	/* =============================================================== */
	/* This is something I REALLY do not understand. The chip does not */
	/* seem to respond correctly until I touch the serial port for the */
	/* first time. Only the God Almighty knows why.                    */
	/*                                                                 */
	/* FIXME: check it again, it may have been due to the power supply */
	/* problems.                                                       */
	/* =============================================================== */
	diag ("91C111 Ethernet interface");

	/* Get the MAC address */
	select (1);
	for (i = 0; i < 3; i++) {
		macaddr [i] = inw (ADDR0_REG + i);
		swab (macaddr [i]);
	}
	diag ("91C111 MAC address:   %x%x%x",
					macaddr [0],
					macaddr [1],
					macaddr [2]);
	select (3);
	i = inw (REV_REG);
	diag ("91C111 chip revision: %x", i);

	macrst ();
	enable ();

#define	phy_id1		(*(pmem + 0))
#define	phy_id2		(*(pmem + 1))
#define	phyaddr		(*(pmem + 2))
#define	found		(*(pmem + 3))
#define	my_phy_caps	(*(pmem + 4))
#define	my_ad_caps	(*(pmem + 5))

#if ! ECOG_SIM

	pmem = umalloc (6 * 2);

	/* Find the PHY address */

	// Scan all 32 PHY addresses if necessary
	for (phyaddr = 0; phyaddr < 32; phyaddr++) {
		// Read the PHY identifiers
		phy_id1  = read_phy_reg (phyaddr, PHY_ID1_REG);
		phy_id2  = read_phy_reg (phyaddr, PHY_ID2_REG);
		// Make sure it is a valid identifier
		if ((phy_id2 > 0x0000) && (phy_id2 < 0xffff) &&
		    (phy_id1 > 0x0000) && (phy_id1 < 0xffff)) {
			if ((phy_id1 != 0x8000) && (phy_id2 != 0x8000)) {
				// Save the PHY's address
				// lp->phyaddr = phyaddr;
				found = 1;
				break;
			}
		}
	}

	if (!found) {
		diag ("91C111 no PHY found");
		syserror (EHARDWARE, "devinit_ethernet (1)");
	}
#if 0
	// Set the PHY type
	if ((phy_id1 == 0x0016) && ((phy_id2 & 0xFFF0) == 0xF840)) {
		// lp->phytype = PHY_LAN83C183;
		diag ("91C111 PHY = LAN83C183");
	} else if ((phy_id1 == 0x0282) && ((phy_id2 & 0xFFF0) == 0x1C50)) {
		// lp->phytype = PHY_LAN83C180;
		diag ("91C111 PHY = LAN83C180");
	} else {
		diag ("91C111 PHY = UNKNOWN");
	}
#endif
	// Reset the PHY, setting all other bits to zero
	write_phy_reg (phyaddr, PHY_CNTL_REG, PHY_CNTL_RST);

	// Wait for the reset to complete, or time out
	found = 6; // Wait up to 3 seconds

	while (found--) {
		if (!(read_phy_reg (phyaddr, PHY_CNTL_REG) & PHY_CNTL_RST)) {
			// reset complete
			break;
		}
		mdelay (500); // wait 500 millisec
	}

	if (found < 1) {
		diag ("91C111 PHY reset timeout");
		syserror (EHARDWARE, "devinit_ethernet (2)");
	}

	/* Configure the Receive/Phy Control register */

	select (0);
	outw (RPC_DEFAULT, RPC_REG);

	// Copy our capabilities from PHY_STAT_REG to PHY_AD_REG
	my_phy_caps = read_phy_reg (phyaddr, PHY_STAT_REG);
	my_ad_caps  = PHY_AD_CSMA; // I am CSMA capable

	if (my_phy_caps & PHY_STAT_CAP_T4)
		my_ad_caps |= PHY_AD_T4;

	if (my_phy_caps & PHY_STAT_CAP_TXF)
		my_ad_caps |= PHY_AD_TX_FDX;

	if (my_phy_caps & PHY_STAT_CAP_TXH)
		my_ad_caps |= PHY_AD_TX_HDX;

	if (my_phy_caps & PHY_STAT_CAP_TF)
		my_ad_caps |= PHY_AD_10_FDX;

	if (my_phy_caps & PHY_STAT_CAP_TH)
		my_ad_caps |= PHY_AD_10_HDX;

	// Update our Auto-Neg Advertisement Register
	write_phy_reg (phyaddr, PHY_AD_REG, my_ad_caps);
#if 0
	diag ("PHY caps:     %x", my_phy_caps);
	diag ("PHY adv caps: %x", my_ad_caps);
#endif
	// I tried auto-negotiation and it didn't quite work, so let us
	// do it manually - makes better sense anyway

	found = read_phy_reg (phyaddr, PHY_CFG1_REG);
	found |= PHY_CFG1_LNKDIS;

	write_phy_reg (phyaddr, PHY_CFG1_REG, found);
	write_phy_reg (phyaddr, PHY_CNTL_REG, PHY_DEFAULT);

	select (0);
	outw (RPC_DEFAULT, RPC_REG);

	ufree (pmem);

#endif // ! ECOG_SIM

	adddevfunc (ioreq_ethernet, ETHERNET);

	/* This must be in effect while the device is running */
	select (2);

#undef	phy_id1
#undef	phy_id2
#undef	phyaddr
#undef	found
#undef	my_phy_caps
#undef	my_ad_caps

}

/* ETHERNET_DRIVER */

#endif
