#ifndef MEM_H_INCLUDED
#define MEM_H_INCLUDED

#include <stdio.h>

void *Malloc(size_t size);
void *Calloc(size_t nmemb, size_t size);
void *Realloc(void *ptr, size_t size, size_t additionalSize);
FILE *Fopen(const char *path, const char *mode);
unsigned long long TotalMemory();

#endif
