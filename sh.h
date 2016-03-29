#include <windows.h>
#include <stdio.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* enable mulitple include 
 * if EXTERN defined befor include of this file
 * then variables only declared  not defined 
 * if NOT defined then variables will be defined */
#ifdef EXTERN
#
#else
# define EXTERN extern
# define EXTERN_DEFINED
#endif

typedef struct Area {
	struct Block *freelist;
} Area;

EXTERN Area aperm;
//#define APERM &aperm
//#define ATEMP &e->area
#define ATEMP &aperm 

#ifndef offsetof
# define offsetof(type, id) ((size_t)&((type*)NULL)->id)
#endif

#include "shf.h"
#include "proto.h"
