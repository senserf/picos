/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"

word __pi_vfparse (char *res, word n, const char *fm, va_list ap) {

	char c;
	word d;

#ifdef	INTERNAL_FUNCTIONS_ALLOWED

	void outc (word c) {
		if (res && (d < n)) res [d] = (char)(c); d++;
	};

#else

#define outc(c) do { if (res && (d < n)) res [d] = (char)(c); d++; } while (0)

#endif	/* INTERNAL_FUNCTIONS_ALLOWED */

#define enci(b)	i = (b); \
		while (1) { \
			c = (char) (val / i); \
			if (c || i == 1) \
				break; \
			i /= 10; \
		} \
		while (1) { \
			outc (c + '0'); \
			val = val - (c * i); \
			i /= 10; \
			if (i == 0) \
				break; \
			c = (char) (val / i); \
		}

	d = 0;

	while (1) {

		c = *fm++;

		if (c == '\\') {
			/* Escape the next character unless it is 0 */
			if ((c = *fm++) == '\0') {
				outc ('\\');
				goto Eol;
			}
			outc (c);
			continue;
		}

		if (c == '%') {
			/* Something special */
			c = *fm++;
			switch (c) {
			    case 'x' : {
				word val; int i;
				val = va_arg (ap, word);
				for (i = 12; ; i -= 4) {
					outc (__pi_hex_enc_table
						[(val>>i)&0xf]);
					if (i == 0)
						break;
				}
				break;
			    }
			    case 'd' :
			    case 'u' : {
				word val, i;
				val = va_arg (ap, word);
				if (c == 'd' && (val & 0x8000) != 0) {
					/* Minus */
					outc ('-');
					val = (~val) + 1;
				}
				enci (10000);
				break;
			    }
#if	CODE_LONG_INTS
			    case 'l' :
				c = *fm;
				if (c == 'd' || c == 'u') {
					lword val, i;
					fm++;
					val = va_arg (ap, lword);
					if (c == 'd' &&
					    (val & 0x80000000L) != 0) {
						/* Minus */
						outc ('-');
						val = (~val) + 1;
					}
					enci (1000000000L);
				} else if (c == 'x') {
					lword val;
					int i;
					fm++;
					val = va_arg (ap, lword);
					for (i = 28; ; i -= 4) {
						outc (__pi_hex_enc_table
							[(val>>i)&0xf]);
						if (i == 0)
							break;
					}
				} else {
					outc ('%');
					outc ('l');
				}
				break;
#endif
			    case 'c' : {
				word val;
				val = va_arg (ap, word);
				outc (val);
				break;
			    }

		  	    case 's' : {
				char *st;
				st = va_arg (ap, char*);
				while (*st != '\0') {
					outc (*st);
					st++;
				}
				break;
			    }
		  	    default:
				outc ('%');
				outc (c);
				if (c == '\0')
					goto Ret;
			}
		} else {
			// Regular character
Eol:
			outc (c);
			if (c == '\0')
Ret:
				return d;
		}
	}
}

char *vform (char *res, const char *fm, va_list aq) {

	word fml, d;

	if (res != NULL) {
		// We trust the caller
		__pi_vfparse (res, MAX_UINT, fm, aq);
		return res;
	}

	// Size unknown; guess a decent size
	fml = strlen (fm) + 17;
	// Sentinel included (it is counted by outc)
Again:
	if ((res = (char*) umalloc (fml)) == NULL)
		/* There is not much we can do */
		return NULL;

	if ((d = __pi_vfparse (res, fml, fm, aq)) > fml) {
		// No luck, reallocate
		ufree (res);
		fml = d;
		goto Again;
	}
	return res;
}
