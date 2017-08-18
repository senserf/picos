/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "form.h"

int vscan (const char *buf, const char *fmt, va_list ap) {

	int nc;

#define	scani(at)	{ at *vap; Boolean mf; \
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
			vap = va_arg (ap, at *); \
			*vap = 0; \
			while (isdigit (*buf)) { \
				*vap = (*vap) * 10 - \
				     (at)(word)(*buf - '0'); \
				buf++; \
			} \
			if (!mf) \
				*vap = (at)(-((at)(*vap))); \
			}
#define scanu(at)	{ at *vap; \
			while (!isdigit (*buf)) \
				if (*buf++ == '\0') \
					return nc; \
			nc++; \
			vap = va_arg (ap, at *); \
			*vap = 0; \
			while (isdigit (*buf)) { \
				*vap = (*vap) * 10 + \
				     (at)(word)(*buf - '0'); \
				buf++; \
			} \
			}
#define	scanx(at)	{ at *vap; int dc; char c; \
			while (!isxdigit (*buf)) \
				if (*buf++ == '\0') \
					return nc; \
			nc++; \
			vap = va_arg (ap, at *); \
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

	if (buf == NULL || fmt == NULL)
		// A safety hatch
		return 0;

	nc = 0;
	while (*fmt != '\0') {
		if (*fmt++ != '%')
			continue;
		switch (*fmt++) {
		    case '\0': return nc;
		    case 'd': scani (word); break;
		    case 'u': scanu (word); break;
		    case 'x': scanx (word); break;
#if	CODE_LONG_INTS
		    case 'l':
			switch (*fmt++) {
			    case '\0':	return nc;
		    	    case 'd': scani (lword); break;
		    	    case 'u': scanu (lword); break;
		    	    case 'x': scanx (lword); break;
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
