#ifndef MYUTF8_H
#define MYUTF8_H

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

int cp_2_utf8(unsigned char *b, size_t n, uint32_t cp);
int cp_2_utf16le(unsigned char *b,  size_t n, uint32_t cp);

int utf8_2_cp(uint32_t *cp, unsigned char *b, size_t n);
int utf16le_2_cp(uint32_t *cp, unsigned char *b, size_t n); 

size_t utf8_bytelen(unsigned char *buf);
size_t utf8_strlen(unsigned char* buf);

size_t utf16_bytelen(wchar_t *buf);
size_t utf16le_strlen(wchar_t *buf);

int utf8_to_utf16le(unsigned char *utf16, size_t len16
	, unsigned char *utf8); 

#endif
