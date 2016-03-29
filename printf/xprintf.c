#include <locale.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>

#include "yvals.h"
#include "xprintf.h"
#include "xmath.h"

void xprintf_dtob(struct ps *ps);

int xprintf_parse_spec(struct ps *ps, const char *fmt)
{
	const char *p = fmt;
	char c;
	while(1) {
		if (*p == '#') {
			ps->flags |= F_HASH;
			p++;
			continue;
		}
		if (*p == '0') {
			if (!(ps->flags & F_DASH))
				ps->flags |= F_ZERO;
			p++;
			continue;
		}
		if (*p == '-') {
			ps->flags |= F_DASH;
			ps->flags &= ~F_ZERO;
			p++;
			continue;
		}
		if (*p == '+') {
			ps->flags |= F_PLUS;
			ps->flags &= ~F_BLNK;
			p++;
			continue;
		}
		if (*p == ' ') {
			if (!(ps->flags & F_PLUS))
				ps->flags |= F_BLNK;
			p++;
			continue;
		}
		break;
	}

	/* check min field with (optional) */
	ps->width = 0;
	while(isdigit(*p)) {
		ps->width = ps->width * 10 + (*p - '0');
		p++;
	}

	/* check for precision (optional) */
	while(*p == '.') {
		ps->precision = 0;
		ps->flags |= F_DOT;
		p++; /* consume '.' */
		while(isdigit(*p)) {
			ps->precision = ps->precision * 10 + (*p - '0');
			p++;
		}
		/* ignore zero flag if precision is given */
		if (ps->precision != 0)
			ps->flags & ~F_ZERO;
	}

	/* check lengt (optional) */
	if (*p == 'h') {
		if (*(p+1) == 'h') {
			ps->lq = 'H';
			p++;	
		} else 	{
			ps->lq = 'h';
		}
		p++;
		goto spec;
	}
	if (*p == 'l') {
		if (*(p+1) == 'l') {
			ps->lq = 'L';
			p++;	
		} else 	{
			ps->lq = 'l';
		}
		p++;
		goto spec;
	}
	if (*p == 'z' || *p == 't') {
		ps->lq = *p;
		p++;
		goto spec;
	}

spec:
	/* conversion specifier (mandatory) */
	c = *p;
	switch(c) {
		case 'd':
		case 'i':
			ps->flags |= F_SIGN;
		case 'c':
		case 'o':
		case 'u':
			ps->flags |= F_INT;
			break;
		case 'X':
			ps->flags |= F_UPPER;
		case 'x':
			ps->flags |= F_INT;
			break;
		case 'E':
			ps->flags |= F_UPPER;
		case 'e':
			ps->precision = ps->precision ? ps->precision : 6;
			ps->flags |= F_FPNBR;
			break;
		case 'F':
			ps->flags |= F_UPPER;
		case 'f':
			ps->precision = ps->precision ? ps->precision : 6;
			ps->flags |= F_FPNBR;
			break;
		case 'G':
			ps->flags |= F_UPPER;
		case 'g':
			ps->precision = ps->precision ? ps->precision : 6;
			ps->flags |= F_FPNBR;
			break;
		case 's':
			ps->flags |= F_STR;
			break;
		case 'p':
			ps->flags |= F_PTR;
			break;
		default:
			/* shell error howto ? */
			(void*)0;	
	}
	ps->spec = *p++;
	return (p - fmt);
}

void xprintf_itob(struct ps *ps)
{
	char *t, *digits, *ebp;
	int sign = 0;
	long unsigned int li = ps->v.li;
	char ac[32];

	ebp = &ac[sizeof(ac) -1];
	t = ebp; 

	switch(ps->spec) {
	case 'c':
	case 'd':
	case 'i':
		if (0 > (long int)li) {
			li = -(long int)li;
			sign = 1;
		} else {
			sign = 0;
		}
	/* fallthrough */
	case 'u':
		do {
			*--t = li % 10 + '0';
			li /= 10;
		} while (li);
		ps->n1 = (ebp - t);
		if (ps->spec != 'u') {
			if (sign) 
				ps->s[0] = '-', ps->n0++; 
			else if (ps->flags & F_PLUS)
				ps->s[0] = '+', ps->n0++;
			else if (ps->flags & F_BLNK)
				ps->s[0] = ' ', ps->n0++;
		}
		break;
	case 'o':
		do {
			*--t = (li & 0x7) + '0';
			li >>= 3;
		} while(li);
		ps->n1 = (ebp - t);
		if ((ps->flags & F_HASH) && *t != '0') {
			ps->s[0] = '0', ps->n0++;
		}
	case 'p':
	case 'x':
		digits = (ps->flags & F_UPPER) ? "0123456789ABCDEF" 
			: "0123456789abcdef";
		do {
			*--t = digits[li & 0xf];
			li >>= 4;
		} while(li);
		ps->n1 = (ebp - t);
		if (ps->flags & F_HASH) {
			ps->s[0] = '0';
			ps->s[1] = (ps->flags & F_UPPER) ? 'X' : 'x';
			ps->n0 = 2;
		}
		break;
	default:
		/* handle error ?*/
		(void*)0;
	}


        /* move ps to start of number and copy it from buf */ 
	ps->s += ps->n0;
	memcpy(ps->s, ebp - ps->n1, ps->n1);
	//ps->s = ebp - ps->n1;         
	/* length of zero padding if any */    
	//ps->n0z = ps->width - ps->n1 - ps->n0; 
}

void xprintf_convert_output(struct ps *ps, va_list *ap)
{
	ps->n0 = ps->n1 = ps->n2 = ps->n0z = ps->n1z = ps->n2z = 0;
	if (ps->flags & F_INT) {
		ps->v.li = (unsigned long)va_arg(*ap, long unsigned int);
		xprintf_itob(ps);
		/* ps->s now contains char representation of the 
		 * integer type nbr setup struct to match format
		 * specification precision width etc */
		
		if (ps->n1 >= ps->precision) {
			ps->precision = ps->n1;
		} else {
			ps->n0z = ps->precision - ps->n1 - ps->n0;
		}

		if (ps->precision > ps->width) {
			ps->width = ps->precision;
		}

		/* len(n1) <=  precision <= width */
		if (ps->width > ps->precision && ps->flags & F_ZERO) {
			ps->n0z = ps->width - ps->precision - ps->n0;
			ps->precision = ps->width;
		}
 


	printf("\n n0:%2d n1:%2d n2:%2d n0z:%2d n1z:%2d n2z:%2d\n"
		, ps->n0, ps->n1, ps->n2, ps->n0z, ps->n1z, ps->n2z);

	printf("width:%2d precision:%2d\n"
		, ps->width, ps->precision);
		
		
	} else if (ps->flags & F_FPNBR) {
		ps->v.d  = va_arg(*ap, double);
		if (0.0 < ps->v.d)
			ps->s[ps->n0++] = '-';
		else if (ps->flags & F_PLUS) 
			ps->s[ps->n0++] = '+';
		else if (ps->flags & F_BLNK) 
			ps->s[ps->n0++] = ' ';
		ps->s = &ps->s[ps->n0];
		xprintf_dtob(ps);
	}
}

static int getexp(short *pex, double d)
{
	unsigned short *ps = (unsigned short *)&d;
	short xchar = (ps[SHD0] & SHDMASK) >> SHDOFF;

	if (xchar == SHDMAX) {
		*pex = 0;
		return (ps[SHD0] & SHDFRAC || ps[SHD1]
			|| ps[SHD2] || ps[SHD3] ? NAN : INF);
	} else if (0 < xchar) {
		*pex = xchar - SHDBIAS;
		return FINITE;
	} else {
		*pex = 0;
		return 0;
	}
}

#define NDIG 8
#define LOG2	30103L / 100000L

static const double pows[] = {
	1e1L, 1e2L, 1e4L, 1e8L, 1e16L
	,1e32L, 1e64L, 1e128L, 1e256L
};

void xprintf_gend(struct ps *ps, char *buf, short nsig, short xexp)
{
	char *p = buf;

	const char point = localeconv()->decimal_point[0];

	if (nsig <= 0)
		nsig = 1, p = "0";
	if (ps->spec == 'f' || ps->spec == 'g' &&
			-4 <= xexp && xexp < ps->precision) {
		// ordinary floating point precision 
		// and for g when xexp fits in precision and is
		// >= -4
		++xexp;
		if (ps->spec != 'f') {
			if (!(ps->flags & F_HASH) && nsig < ps->precision)
				ps->precision = nsig;
			if ((ps->precision -= xexp) < 0)
				ps->precision = 0;
		}

		if (xexp <= 0) {
			ps->s[ps->n1++] = '0';
			if (0 < ps->precision || ps->flags & F_HASH)
				ps->s[ps->n1++] = point;
			/* if this is true only output zeros 
			 * precision is not large enoughe to 
			 * include any significant digits */
			if (ps->precision < -xexp) 
				xexp = -ps->precision;
			ps->n1z = -xexp;
			ps->precision += xexp;
			/* precision >= 0 */
			if (ps->precision < nsig) {
				nsig = ps->precision;
			}
			/* nsig <= precision */
			ps->n2  = nsig;
			memcpy(&ps->s[ps->n1], p, ps->n2);
			ps->n2z = ps->precision - nsig;
		} else if (nsig < xexp) {
			/* generate zeros to the right of value 
			 * before decimal point
			 **/
			memcpy(&ps->s[ps->n1], p, nsig);
			ps->n1 += nsig;
			ps->n1z = xexp - nsig;
			if (0 < ps->precision || ps->flags & F_HASH) {
				ps->s[ps->n1] = point;
				++ps->n2;
			}
			/* nbr of zeros after decimal point */
			ps->n2z = ps->precision;
		} else {
			/* enough digits before point */
			memcpy(&ps->s[ps->n1], p, xexp);
			ps->n1 += xexp;
			nsig -= xexp;
			if (0 < ps->precision || ps->flags & F_HASH) {
				ps->s[ps->n1] = point;
				++ps->n2;
			}
			if (ps->precision < nsig) 
				nsig = ps->precision;
			memcpy(&ps->s[ps->n1], p + xexp, nsig);
			ps->n1 += nsig;
			ps->n1z = ps->precision;
		}
	} else {
		if (ps->spec == 'g') {
			if (nsig < ps->precision) 
				ps->precision = nsig;
			if (--ps->precision < 0)
				ps->precision = 0;
		}
		ps->s[ps->n1++] = *p++;
		if (0 < ps->precision || ps->flags & F_HASH)
			ps->s[ps->n1++] = point;
		if (0 < ps->precision) {
			/* output upto nsig digits and
			 * adjust precision accordingly */
			if (ps->precision < --nsig)
				nsig = ps->precision;
			memcpy(&ps->s[ps->n1], p, nsig);
			ps->n1 += nsig;
			ps->n1z = ps->precision - nsig;
		}
		p = &ps->s[ps->n1];
		*p++ = ps->flags & F_UPPER ? 'E' : 'e';
		if (0 <= xexp)
			*p++ = '+';
		else {
			*p++ = '-';
			xexp = -xexp;
		}

		if (100 <= xexp) {
			if (1000 <= xexp)
				*p++ = xexp / 1000 + '0', xexp %= 1000;
			*p++ = xexp / 100 + '0', xexp %= 100;
		}
		*p++ = xexp / 10 + '0', xexp %= 10;
		*p++ = xexp + '0';
		ps->n2 = p - &ps->s[ps->n1];
	}

	if ((ps->flags & (F_DASH | F_ZERO)) == F_ZERO) {
		int n = ps->n0 + ps->n1 + ps->n1z + ps->n2 + ps->n2z;
		if (n < ps->width) 
			ps->n0z = ps->width -n;
	}
}

void xprintf_dtob(struct ps *ps)
{
	char ac[FPBUFSIZE];
	char *p = ac;

	short errx, xexp, nsign;

	double ld = ps->v.d;

	if (0 < (errx = getexp(&xexp, ps->v.d))) {
		memcpy(p, errx == NAN ? "NaN" : "Inf", ps->n1 = 3);
		return;
	} else if (0 == errx) {
		nsign = 0, xexp = 0;
	} else {
		int i, n;
		if (ld < 0.0)
			ld = -ld;

		/* scale to expose a reasonable nbr of digits 
		 * to the left of the decimal point */
		xexp = xexp * LOG2 - NDIG/2;
		if (0 > xexp) {
			/* scale up */
			n = (-xexp + (NDIG/2 - 1)) & ~(NDIG/2 - 1);
			for (i = 0; i < n; n >>= 1, ++i) {
				if (n & 1) 
					ld *= pows[i];
			}
		} else if ( 0 < xexp) {
			/* scale down */
			double factor = 1.0;
			xexp &= ~(NDIG/2-1);
			for (n = xexp, i = 0; 0 < n; n >>= 1, ++i) {
				if (n & 1)
					factor *= pows[i];
			}
			ld /= factor;
		}

		/* amount of output we are expected to generate */
		int gen = ps->precision + (ps->spec == 'f' 
			? xexp + 2 + NDIG : 2 + NDIG / 2);

		/* correct for long double 
		if (LDBL_DIG + NDIG/2 < gen)
			gen = LDBL_DIG + NDIG/2;
		*/

		/* output decimal version of ld 8 char at time 
		 * right to left */
		for (*p++ = '0'; 0 < gen && 0.0 < ld; p += NDIG) {
			/* p = 9, 17, 25, ... */
			int j;
			long li = (long)ld;

			if (0  < (gen -= NDIG)) 
				ld = (ld - (double)li) * 1e8L;

			for (p += NDIG, j = NDIG; 0 < li && 0 <= --j;) {
				/* p = 17, 25, ... */
				ldiv_t qr = ldiv(li, 10);
				*--p = qr.rem + '0', li = qr.quot;
			}

			/* left pad with zeros */
			while (0 <= --j) {
				*--p = '0';
			}

		}
		gen = p - &ac[1];
		for (p = &ac[1], xexp += NDIG - 1; *p == '0'; ++p) 
			--gen, --xexp;

		nsign = ps->precision;
		if (ps->spec == 'f')
			nsign += xexp + 1;
		else if (ps->spec == 'e')
			nsign += 1;
		
		if (gen < nsign)
			nsign = gen;

		/* at this point the following is true for nsig
		 * -xexp < nsig <= gen
		 * negative nsit resutls in a zero left padded reult
		 */
		if (0 < nsign) {
			const char drop = nsign < gen && '5' < p[nsign]
				? '9' : '0';
			int n;
			for (n = nsign; p[--n] == drop;)
				--nsign;
		
			/* increase first digit form the right
			 * which is not a nine in a string of nines*/	
			if (drop == '9')
				++p[n];
		
			/* input exhausted */	
			if (n < 0)
				--p, ++nsign, ++xexp;
		}
	}
	xprintf_gend(ps, p, nsign, xexp);
}

#define PADSIZE		FPBUFSIZE
#define PUT(b, n)	if ((n) > 0) 					\
				if ((w = ofn(ofn_arg, (b), (n))) > 0)   \
					nwritten += w; 			\
				else					\
					return EOF

#define PAD(b, n)	if ((n) % PADSIZE) {				\
				PUT((b), ((n) % PADSIZE));		\
				(n) -=((n) % PADSIZE);			\
			}						\
			while ((n)) {					\
				PUT((b), PADSIZE);			\
				(n) -= PADSIZE;				\
			}


_ssize_t xprintf(_ssize_t (*ofn)(void *, const char *, size_t)
		, void *ofn_arg, const char *fmt, va_list *ap)
{
	const char *p;
	char c;
	char *buf;
	int w, nwritten = 0; 
	struct ps ps;
	char ac[FPBUFSIZE];
	char space[PADSIZE+1] = "                                ";
	char zero[PADSIZE+1]  = "00000000000000000000000000000000";

	if (!fmt)
		return 0;

	p = fmt;
	while((c = *p) != 0) {
		if (c != '%') {
			PUT(p, 1);
			p++;
			continue;
		}
		/* consume '%' */
		p++;
		if (*p == '%') {
			/* output escaped '%' */
			PUT(p, 1);
			p++;
			continue;
		}
		ps.flags = 0;
		ps.width = 0;
		ps.precision = 0;

		/* parse format specifier */
		p += xprintf_parse_spec(&ps, p);

		/* use info in ps to correctly format data */
		ps.s = ac;
		xprintf_convert_output(&ps, ap);

		/* len(n1z) <=  precision <= width */
		int sp;
		if (ps.width > ps.precision && !(ps.flags & F_DASH)) {
			/* left pad with space */
			sp = ps.width - ps.precision - ps.n0;
			PAD(space, sp);
		}

		/* any perfixes ie +, -, 0, 0x etc. */
		PUT(ac, ps.n0);     
		PAD(zero, ps.n0z);  /* zero pad befor any digits */
		PUT(ps.s, ps.n1);   /* put digits including decimal point */ 
		PAD(zero, ps.n1z);  /* ?? */
		PUT(ps.s + ps.n1, ps.n2);
		PAD(zero, ps.n2z);

		if (ps.width > ps.precision && (ps.flags & F_DASH)) {
			sp = ps.width - ps.precision - ps.n0;	
			PAD(space, sp);
		}
	}
	return nwritten;
}
