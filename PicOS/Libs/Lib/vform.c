/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"

char *vform (char *res, const char *fm, va_list aq) {

	word fml, s, d;
	char c;
	va_list ap;

#define	outc(c)	do { \
			if (d >= fml) \
				goto ReAlloc; \
			res [d++] = (char)(c); \
		} while (0)

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
#define encx(s)	for (i = 0; i < (s); i += 4) { \
			outc (zz_hex_enc_table [((val >> (((s)-4)-i)) & 0xf)]);\
		}


	if (res != NULL)
		/* Fake huge maximum length */
		fml = MAX_UINT;
	else
		/* Guess an initial length of the formatted string */
		fml = strlen (fm) + 16;

	while (1) {
		if (fml != MAX_UINT) {
			if ((res = (char*) umalloc (fml+1)) == NULL)
				/* There is not much we can do */
				return NULL;
			/* This is how far we can go */
			fml = actsize (res) - 1;
		}
		s = d = 0;
		ap = aq;

		while (1) {
			c = fm [s++];
			if (c == '\\') {
				/* Escape the next character unless it is 0 */
				if ((c = fm [s++]) == '\0') {
					res [d] = '\0';
					return res;
				}
				outc (c);
				continue;
			}
			if (c == '%') {
				/* Something special */
				c = fm [s++];
				if (c == '\0') {
					res [d] = '\0';
					return res;
				}
				switch (c) {
				    case 'x' : {
					word val; int i;
					val = va_arg (ap, word);
					encx (16);
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
					c = fm [s];
					if (c == 'd' || c == 'u') {
						lword val, i;
						s++;
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
						s++;
						val = va_arg (ap, lword);
						encx (32);
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
					char * st;
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
				}
			} else {
				outc (c);
				if (c == '\0')
					return res;
			}
		}
	ReAlloc:
		if (fml == MAX_UINT)
			/* Impossible */
			return res;
		ufree (res);
		fml += 16;
	}
}
