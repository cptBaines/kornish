#ifndef ALLOC_H
#define ALLOC_H
Area * ainit    (Area *ap);
void   afreeall (Area *ap);
void * alloc    (size_t size, Area *ap);
void * aresize  (void *ptr, size_t size, Area *ap);
void   afree    (void *ptr, Area *ap);
#endif
