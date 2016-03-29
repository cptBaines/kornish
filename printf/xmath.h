#ifndef XMATH_H
#define XMATH_H

/* short array indexing depeding on little / big endian system */
#if SHD0 == 3
#  define SHD1	2
#  define SHD2	1
#  define SHD3	0
#else
#  define SHD1	1
#  define SHD2	2
#  define SHD3	3
#endif

#define SHDSIGN 0x8000
#define SHDFRAC ((1<<SHDOFF)-1)
#define SHDMASK (0x7fff & ~SHDFRAC)
#define SHDMAX  (1 << (15 - SHDOFF) -1)
#define SHDNAN  (0x8000 | SHDMAX << SHDOFF | 1 << (SHDOFF-1))

#define FINITE -1
#define NAN     1
#define INF     2

#endif
