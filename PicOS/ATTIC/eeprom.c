/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "eeprom.h"

#if EE_USE_UART

static byte get_byte () {
	// Send a dummy byte of zeros; we are the master so we have to
	// keep the clock ticking
	ee_put (0);
	while (!ee_tx_ready);
	while (!ee_rx_ready);
	return ee_get;
}

static void put_byte (byte b) {
	byte s;
	ee_put (b);
	while (!ee_tx_ready);
	while (!ee_rx_ready);
	s = ee_get;
}

#else	/* EE_USE_UART */

static byte get_byte () {

	register int i;
	register byte b;

	for (b = 0, i = 0; i < 8; i++) {
		b <<= 1;
		if (ee_inp)
			b |= 1;
		ee_clkh;
		ee_clkl;
	}

	return b;
}

static void put_byte (byte b) {

	register int i;

	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			ee_outh;
		else
			ee_outl;
		ee_clkh;
		ee_clkl;
		b <<= 1;
	}
}

#endif

void zz_ee_init () {

	ee_ini_regs;
	ee_ini_spi;

	ee_start;
	put_byte (EE_WRSR);
	put_byte (STAT_INI);
	ee_stop;
	udelay (10);

	ee_postinit;
}

void ee_read (word a, byte *s, word len) {

	byte c;

	if (len == 0)
		return;

	ee_start;
	put_byte (EE_RDSR);
	do {
		c = get_byte ();
	} while (c & STAT_WIP);
	ee_stop;

	a &= (EE_SIZE - 1);

	ee_start;
	put_byte (EE_READ);
	put_byte ((byte)((a) >> 8));
	put_byte ((byte)((a)     ));

	while (len--)
		*s++ = get_byte ();
	ee_stop;
}

void ee_erase () {

	word a;
	byte c;
	byte cnt;

	for (a = 0; a < EE_SIZE; a += EE_PAGE_SIZE) {
		ee_start;
		put_byte (EE_RDSR);
		do {
			c = get_byte ();
		} while (c & STAT_WIP);
		ee_stop;

		a &= (EE_SIZE - 1);

		ee_start;
		put_byte (EE_READ);
		put_byte ((byte)((a) >> 8));
		put_byte ((byte)((a)     ));

		for (cnt = 0; cnt < EE_PAGE_SIZE; cnt++)
			if (get_byte () != 0xff)
				break;
		ee_stop;

		if (cnt >= EE_PAGE_SIZE)
			// This block is OK
			continue;

		// Erase it

		ee_start;
		put_byte (EE_RDSR);
		do {
			c = get_byte ();
		} while (c & STAT_WIP);
		ee_stop;

		// Set WEL
		ee_start;
		put_byte (EE_WREN);
		ee_stop;

		ee_start;
		put_byte (EE_WRITE);
		put_byte ((byte)((a) >> 8));
		put_byte ((byte)((a)     ));

		for (cnt = 0; cnt < EE_PAGE_SIZE; cnt++)
			put_byte (0xff);
		ee_stop;

	}
}

void ee_write (word a, const byte *s, word len) {

	byte c, ne;

	if (a >= EE_APP_SIZE || (a + len) > EE_APP_SIZE)
		syserror (EEEPROM, "ee_write");

	while (len) {
		// How far to the end of page
		ne = EE_PAGE_SIZE - (a & (EE_PAGE_SIZE - 1));
		if (ne > len)
			ne = len;

		ee_start;
		put_byte (EE_RDSR);
		do {
			// Wait for the end of previous write
			c = get_byte ();
		} while (c & STAT_WIP);
		ee_stop;

		// Set WEL
		ee_start;
		put_byte (EE_WREN);
		ee_stop;

		ee_start;
		put_byte (EE_WRITE);
		put_byte ((byte)((a) >> 8));
		put_byte ((byte)((a)     ));

		len -= ne;
		a += ne;

		while (ne--)
			put_byte (*s++);
		ee_stop;

	}
}

#if DIAG_MESSAGES > 2

static word ee_dbg_curr = EE_DBG_START;

static void ee_diag (const char *s, word len) {

	byte c, ne;

	while (len) {
		// How far to the end of page
		ne = EE_PAGE_SIZE - (ee_dbg_curr & (EE_PAGE_SIZE - 1));
		if (ne > len)
			ne = len;

		ee_start;
		put_byte (EE_RDSR);
		do {
			// Wait for the end of previous write
			c = get_byte ();
		} while (c & STAT_WIP);
		ee_stop;

		// Set WEL
		ee_start;
		put_byte (EE_WREN);
		ee_stop;

		ee_start;
		put_byte (EE_WRITE);
		put_byte ((byte)((ee_dbg_curr) >> 8));
		put_byte ((byte)((ee_dbg_curr)     ));

		len -= ne;
		ee_dbg_curr += ne;

		while (ne--)
			put_byte ((byte)(*s++));
		ee_stop;
		if (ee_dbg_curr >= EE_SIZE)
			ee_dbg_curr = EE_DBG_START;

	}
}

#if	UART_DRIVER

static void diag_ds (word *ptr) {

	byte c;

	while (1) {
		ee_read (*ptr, &c, 1);
		if (c == '\0')
			break;
		(*ptr)++;
		if (*ptr >= EE_SIZE)
			*ptr = EE_DBG_START;
		if (c > 127 || c < 32)
			continue;
		diag_wait (a);
		diag_wchar (c, a);
	}
	diag_wait (a);
	diag_wchar ('\r', a);
	diag_wait (a);
	diag_wchar ('\n', a);
}

static void diag_st (const char *str) {

	while (*str != '\0') {
		diag_wait (a);
		diag_wchar (*str, a);
		str++;
	}
	diag_wait (a);
	diag_wchar ('\r', a);
	diag_wait (a);
	diag_wchar ('\n', a);
}

#endif

void diag_dump () {

#if 	UART_DRIVER

	word ptr, sptr;
	byte c, is;

	diag_disable_int (a, is);

	for (ptr = EE_DBG_START; ptr < EE_SIZE; ptr++) {
		ee_read (ptr, &c, 1);
		if (c == 0xff) {
			// Nothing
NotFound:
			diag_st ("NO INFO IN DIAG BUFFER");
			goto End;
		}
		if (c == 0375) {
			if (++ptr >= EE_SIZE)
				ptr = EE_DBG_START;
			ee_read (ptr, &c, 1);
			if (c != '\0')
				goto NotFound;
			else
				break;
		}
	}

	if (ptr >= EE_SIZE)
		goto NotFound;

	diag_st ("BEGIN OF DUMP");

	sptr = ptr;
	do {
		if (++ptr >= EE_SIZE)
			ptr = EE_DBG_START;
		diag_ds (&ptr);
	} while (ptr != sptr);
		
	diag_st ("DUMP COMPLETE");
End:
	diag_wait (a);
	diag_enable_int (a, is);
#endif
}

void diag (const char *mess, ...) {

	va_list	ap;
	word i, is, val, v;
	const char *s;
	char buf [6];

	va_start (ap, mess);

	while  (*mess != '\0') {
		for (i = 0, s = mess; *s != '\0' && *s != '%'; s++, i++);
		if (i) {
			ee_diag (mess, i);
			mess = s;
		}
		if (*mess == '%') {
			mess++;
			switch (*mess) {
			  case 'x' :
				val = va_arg (ap, word);
				for (i = 0; i < 16; i += 4) {
					v = (val >> (12 - i)) & 0xf;
					if (v > 9)
						v = (word)'a' + v - 10;
					else
						v = (word)'0' + v;
					buf [i >> 2] = (char) v;
				}
				ee_diag (buf, 4);
				break;
			  case 'd' :
				is = 0;
				val = va_arg (ap, word);
				if (val & 0x8000) {
					buf [is++] = '-';
					val = (~val) + 1;
				}
			    DI_SIG:
				i = 10000;
				while (1) {
					v = val / i;
					if (v || i == 1) break;
					i /= 10;
				}
				while (1) {
					buf [is++] = (char) (v + '0');
					val = val - (v * i);
					i /= 10;
					if (i == 0) break;
					v = val / i;
				}
				ee_diag (buf, is);
				break;
			  case 'u' :
				val = va_arg (ap, word);
				is = 0;
				goto DI_SIG;
			  case 's' :
				s = va_arg (ap, char*);
				for (i = 0; *s != '\0'; s++, i++);
				s -= i;
				ee_diag (s, i);
				break;
			  default:
				ee_diag (mess-1, 2);
			}
			mess++;
		}
	}

	// Complete
	ee_diag ("\0\375\0", 3);
	// Backspace the pointer
	if (ee_dbg_curr - EE_DBG_START < 2)
		ee_dbg_curr = EE_SIZE - EE_DBG_START + ee_dbg_curr - 2;
	else
		ee_dbg_curr -= 2;
}

#endif
