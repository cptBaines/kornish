/*
#include <stdio.h>

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
*/
#include "utf8.h"
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#define INVALID_CODE_POINT 0x80000000

/* max path in win API */
#define WIN_MAX_WCHAR_PATH  32768

int utf16le_2_cp(uint32_t *cp, unsigned char *b, size_t n)
{
	if (b == NULL || cp == NULL || n < 2) 
		return 0;
	unsigned char c = *b++;
	if (*b <= 0xD7) {
		*cp = (*b << 8 | c);
		return 2;
	}
	if (*b <= 0xDF) {
		if (n < 4) 
			return 0;
		*cp = 0x10000 + ((*b++ - 0xD8) << 18 | c << 10);
		c = *b++;
		*cp |= ((*b - 0xDC) << 8 | c);
		return 4;
	}  
	*cp = (*b << 8 | c);
	return 2;
}

int cp_2_utf16le(unsigned char *b,  size_t n, uint32_t cp) 
{	
	uint16_t hi;
	uint16_t lo;

	/* free */
	/* D7FF = 1101 0111 1111 1111 */
	/* reserved */
	/* D800 = 1101 1000 0000 0000 */
	/* DFFF = 1101 1111 1111 1111 */
	/* free */
	/* E0 = 1110 0000 0000 0000 */
	if (b == NULL || n < 2) return 0;

	/* UTF-16LE-encoding */
	if (cp <= 0xD7FF) {
		*b++ = (unsigned char)(cp & 0xFF);
		*b++ = (unsigned char)((cp & 0xFF00) >> 8);
		return 2;
	} else if ( cp <= 0xDFFF) { /* in reserved range */
		return 0;
	} else if ( cp <= 0xFFFF) { 
		*b++ = (unsigned char)(cp & 0xFF);
		*b++ = (unsigned char)((cp & 0xFF00) >> 8);
		return 2;
	} else if (cp <= 0x10FFFF) {
		if (n < 4) return 0;
		cp -= 0x10000;
		hi = (uint16_t) (0xD800 + ((cp & 0x000FFC00) >> 10));
		lo = (uint16_t) (0xDC00 + (cp & 0x000003FF));
		*b++ = (unsigned char)(hi & 0xFF);
		*b++ = (unsigned char)((hi & 0xFF00) >> 8);
		*b++ = (unsigned char)(lo & 0xFF);
		*b++ = (unsigned char)((lo & 0xFF00) >> 8);
		return 4;
	}

	/* cp out of range */
	return 0;
}

int utf8_2_cp(uint32_t *cp, unsigned char *b, size_t n)
{
	uint32_t t = 0;
	unsigned int i = 0;	

	if (b == NULL || cp == NULL || n < 1) 
		return 0;
	

	if ((*b & 0x80) != 0x80) { /* 7-bit ascii */
		*cp =  (uint32_t) (*b & 0x7F);
		return 1;
	}

	if ((*b & 0xFE) == 0xFC) { /* 31 - bit */
		t = ((*b & 0x01) << 30);	
		i = 5;
	}
	else if ((*b & 0xFC) == 0xF8) { /* 26 - bit */
		t = ((*b & 0x03) << 24);	
		i = 4;
	}
	else if ((*b & 0xF8) == 0xF0) { /* 21 - bit */
		t = ((*b & 0x07) << 18);	
		i = 3;
	}
	else if ((*b & 0xF0) == 0xE0) { /* 16 - bit */
		t = ((*b & 0xF) << 12);	
		i = 2;
	}
	else if ((*b & 0xE0) == 0xC0) { /* 11 - bit */
		t = ((*b & 0x1F) << 6);	
		i = 1;
	} else {
		return 0; /* error conituation char */
	}

	if (i > (n - 1)) {
		/* error buffer to smal for first byte */
		return 0; 
	}

	*b++;
	int j = i;
	while (j > 0) {
		t |= (*b++ & 0x03F) << (--j * 6);
	}

	*cp = t;
	return i+1;
}

int cp_2_utf8(unsigned char *b, size_t n, uint32_t cp) 
{
	unsigned int i = 0;

	if (b == NULL || n < 1)
		return 0;

	if (cp <= 0x007F) { /* 7-bit ascii */
		*b++ = (unsigned char)((0x00) | (cp & 0x0000007F));
		i = 0;
	} else if (cp <= 0x07FF) { /* 11-bit */
		*b++ = (unsigned char)((0xC0) | ((cp & 0x000007C0) >> 6));
		i = 1;
	} else if (cp <= 0xFFFF) { /* 16-bit */
		*b++ = (unsigned char) ((0xE0) | ((cp & 0x0000F000) >> 12));
		i = 2;
	} else if (cp <= 0x1FFFFF) { /* 21-bit */
		*b++ = (unsigned char) ((0xF0) | ((cp & 0x001C0000) >> 18));
		i = 3;
	} else if (cp <= 0x3FFFFFF) { /* 26-bit */
		*b++ = (unsigned char) ((0xF8) | ((cp & 0x03000000) >> 24));
		i = 4;
	} else if (cp <= 0x7FFFFFFF) { /* 31-bit */
		*b++ = (unsigned char) ((0xFC) | ((cp & 0x40000000) >> 30));
		i = 5;
	} else {
		return 0; /* error */
	}

	if (i > (n - 1))
		return 0; /* buffer to small */

	int j = i;
	while(j > 0) {
		int s = (--j * 6);
		*b++ = (unsigned char) ((0x80) | ((cp & (0x0000003F << s)) >> s ));
	}

	return i + 1;
}

/* return nbr of bytes before null termination byte */
size_t utf8_bytelen(unsigned char *buf)
{
	size_t n = 0;
	while(*buf != 0) {
		n++;
		buf++;
	}
	return n;
}

/* return nbr of bytes before null termination byte */
size_t utf16_bytelen(wchar_t *buf)
{
	size_t n = 0;
	while(*buf != 0) {
		n++;
		buf++;
	}
	return (n * sizeof(wchar_t));
}

/* return nbr of codepoints (characters) in string */
size_t utf8_strlen(unsigned char* buf)
{
	size_t n = 0;
	while(*buf != 0) {
		if ((*buf & 0xC0) != 0x80) {
			n++;
		}
		buf++;
	}
	return n;
}

size_t utf16le_strlen(wchar_t *buf)
{
	size_t n = 0;
	unsigned char c = 0;
	while(*buf != 0) {
		c = (unsigned char)((*buf) & 0x00FF);
		if (c <= 0xD7) {
			buf++;
			n++;
			continue;
		}
		if (c <= 0xDF) {
			if (*(buf+1) == 0)
				break;
			buf += 2;
			n++;
			continue;
		}  

		buf++;
		n++;
	}
	return n;
}


int utf8_to_utf16le(unsigned char *utf16, size_t len16
	, unsigned char *utf8) 
{
	uint32_t cp = 0;
	size_t nwrite = 0;
	size_t len8 = utf8_bytelen(utf8);
	if (len8 * 2 > len16) {
		/* utf16 buffer to small */
		return -1;
	}

	while(*utf8 != 0) {
		size_t n = 0;
		n = utf8_2_cp(&cp, utf8, len8);
		if (n == 0) {
			/* error */
			return -1;
		}
		utf8 += n;
		len8 -= n;

		n = cp_2_utf16le(utf16, len16, cp);
		if (n == 0) {
			/* error */
			return -1;
		}
		utf16 += n;
		len16 -= n;
		nwrite += n;
	}
	return nwrite;
}

#if 0
wchar_t *utf8_2_utf16(const char *utf8)
{
	wchar_t buf[WIN_MAX_WCHAR_PATH];	

	if (utf8 == NULL)
		return NULL;

	unsigned short codepoint = 0;
	unsigned const char *cp = (unsigned char *)utf8;
	unsigned char *wp = (unsigned char *)(buf);

	size_t wcount = 0;
	int i = 0;

	while(*cp != 0 && wcount < WIN_MAX_WCHAR_PATH) {
		if ((*cp & 0x80) != 0x80) { /* single char */
			if (codepoint) {
				wp[0] = (codepoint & 0x00ff);
				wp[1] = ((codepoint & 0xff00) >> 8);
				codepoint = 0;
				wp += 2;
				wcount++;
			}
			/* copy current single char */
			*wp = *cp;
			wp += 2;
			i = 0;
			wcount++;
		} else if ((*cp & 0xf0) == 0xe0) { /* start 3 char */
			if (codepoint) {
				wp[0] = (codepoint & 0x00ff);
				wp[1] = ((codepoint & 0xff00) >> 8);
				wp += 2;
				wcount++;
			}
			codepoint = ((*cp & 0x0f) << 12);
			i = 2;
		} else if ((*cp & 0xe0) == 0xc0) { /* start 2 char */
			if (codepoint) {
				wp[0] = (codepoint & 0x00ff);
				wp[1] = ((codepoint & 0xff00) >> 8);
				wp += 2;
				wcount++;
			}
			codepoint = ((*cp & 0x1f) << 6);
			i = 1;
		} else if ((*cp & 0xc0) == 0x80) { /* continue */
			if (i == 0) { /* error continuation without start */
				printf("Continuation without start\n");
				return NULL;
			}	
			codepoint += ((*cp & 0x3f) << (6 * --i)); 
		}
		cp++;
	}
	/* handle any trailing code point */
	if (codepoint && wcount < WIN_MAX_WCHAR_PATH) {
		wp[0] = (codepoint & 0x00ff);
		wp[1] = ((codepoint & 0xff00) >> 8);
		codepoint = 0;
		wp+=2;
		wcount++;
	}

	/* zero terminate if we have room otherwise return NULL
	 * buffer to small
	 */
	if (wcount < WIN_MAX_WCHAR_PATH - 1) {
		wp[0] = 0;
		wp[1] = 0;
		wcount++;
	} else {
		return NULL;
	}

	/* Alloc mem and return utf16 str */
	wchar_t *utf16 = (wchar_t*) malloc(sizeof(wchar_t) * wcount);
	if (utf16 == NULL) {
		return NULL;
	}

	memcpy(utf16, buf, sizeof(wchar_t) * wcount);
	return utf16;
}
#endif
