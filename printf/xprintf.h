#ifndef XPRINTF_H
#define XPRINTF_H

#include <sys/types.h>

/* flags */
#define F_HASH	0x0001  /* '#' */
#define F_ZERO  0x0002  /* '0' */
#define F_PLUS  0x0004  /* '+' */
#define F_BLNK	0x0008  /* ' ' */
#define F_DASH  0x0010  /* '-' */ 


#define F_SIGN    0x0020  /* singed (d,i) */
#define F_UPPER   0x0040  /* upper format char */
#define F_INT     0x0080  /* integer type */
#define F_FPNBR   0x0100  /* floating pont type */
#define F_STR     0x0200  /* floating pont type */
#define F_PTR     0x0400  /* floating pont type */
#define F_DOT     0x0800  /* we have seen precision */

#define FPBUFSIZE 32

struct ps {
	int   width;
	int   precision;
	int   flags;
	char  *s;
	int   lq;           /* length quallifier */
	int   spec;         /* format specifier */
	int   n0;
	int   n0z;
	int   n1;
	int   n1z;
	int   n2;
	int   n2z;
	union {
		unsigned long int li;
		double d;
	} v;
};

_ssize_t xprintf(_ssize_t (*ofn)(void *, const char *, size_t)
		, void *ofn_arg, const char *fmt, va_list *ap);
#endif
