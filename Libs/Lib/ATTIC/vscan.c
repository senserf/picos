#include "sysio.h"
#include "form.h"
/* ============================================================================ */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002--2005                 */
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

int vscan (const char *buf, const char *fmt, va_list ap) {

	int nc;

#define	scani(at)	{ unsigned at *vap; bool mf; \
			Retry_d_ ## at: \
			while (!isdigit (*buf) && *buf != '-' && *buf != '+') \
				if (*buf++ == '\0') \
					return nc; \
			mf = NO; \
			if (*buf == '-' || *buf == '+') { \
				if (*buf++ == '-') \
					mf = YES; \
				if (!isdigit (*buf)) \
					goto Retry_d_ ## at; \
			} \
			nc++; \
			vap = va_arg (ap, unsigned at *); \
			*vap = 0; \
			while (isdigit (*buf)) { \
				*vap = (*vap) * 10 - \
				     (unsigned at)(unsigned int)(*buf - '0'); \
				buf++; \
			} \
			if (!mf) \
				*vap = (unsigned at)(-((at)(*vap))); \
			}
#define scanu(at)	{ unsigned at *vap; \
			while (!isdigit (*buf)) \
				if (*buf++ == '\0') \
					return nc; \
			nc++; \
			vap = va_arg (ap, unsigned at *); \
			*vap = 0; \
			while (isdigit (*buf)) { \
				*vap = (*vap) * 10 + \
				     (unsigned at)(unsigned int)(*buf - '0'); \
				buf++; \
			} \
			}
#define	scanx(at)	{ unsigned at *vap; int dc; char c; \
			while (!isxdigit (*buf)) \
				if (*buf++ == '\0') \
					return nc; \
			nc++; \
			vap = va_arg (ap, unsigned at *); \
			*vap = 0; \
			dc = 0; \
			while (isxdigit (*buf) && dc < 2 * sizeof (at)) { \
				c = *buf++; dc++; \
				if (isdigit (c)) \
					c -= '0'; \
				else if (c <= 'f' && c >= 'a') \
					c -= (char) ('a' - 10); \
				else \
					c -= (char) ('A' - 10); \
				*vap = ((*vap) << 4) | (at) c; \
			} \
			}




	nc = 0;
	while (*fmt != '\0') {
		if (*fmt++ != '%')
			continue;
		switch (*fmt++) {
		    case '\0': return nc;
		    case 'd': scani (int); break;
		    case 'u': scanu (int); break;
		    case 'x': scanx (int); break;
#if	CODE_LONG_INTS
		    case 'l':
			switch (*fmt++) {
			    case '\0':	return nc;
		    	    case 'd': scani (long); break;
		    	    case 'u': scanu (long); break;
		    	    case 'x': scanx (long); break;
			}
			break;
#endif
		    case 'c': {
			char c, *sap;
			/* One character exactly where we are */
			if ((c = *buf++) == '\0')
				return nc;
			nc++;
			sap = va_arg (ap, char*);
			*sap = c;
			break;
		    }
		    case 's': {
			char *sap;
			while (isspace (*buf)) buf++;
			if (*buf == '\0')
				return nc;
			nc++;
			sap = va_arg (ap, char*);

			if (*buf != ',') {
				while (!isspace (*buf) && *buf != ',' &&
					*buf != '\0')
						*sap++ = *buf++;
			}
			while (isspace (*buf)) buf++;
			if (*buf == ',') buf++;
			*sap = '\0';
			break;
		    }
		}
	}
	return nc;
}
