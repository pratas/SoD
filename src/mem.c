#include "mem.h"
#include <stdlib.h>

//-----------------------------------------------------------------------------

static unsigned long long totalMemory = 0;

//-----------------------------------------------------------------------------

void *Malloc(size_t size)
	{
	void *pointer = malloc(size);

	if(pointer == NULL)
		{
		fprintf(stderr, "Error allocating %zu bytes.\n", size);
		exit(1);
		}

	totalMemory += size;

	return pointer;
	}

//----------------------------------------------------------------------------

void *Calloc(size_t nmemb, size_t size)
	{
	void *pointer = calloc(nmemb, size);

	if(pointer == NULL)
		{
		fprintf(stderr, "Error allocating %zu bytes.\n", size);
		exit(1);
		}

	totalMemory += nmemb * size;

	return pointer;
	}

//----------------------------------------------------------------------------

void *Realloc(void* ptr, size_t size, size_t additionalSize)
	{
	void *pointer = realloc(ptr, size);
	
	if(pointer == NULL) 
		{
		fprintf(stderr, "Error allocating %zu bytes.\n", size);
		exit(1);
		}

	totalMemory += additionalSize;

	return pointer;
	}

//----------------------------------------------------------------------------

FILE *Fopen(const char *path, const char *mode)
	{
	FILE *file = fopen(path, mode);

	if(file == NULL)
		{
		fprintf(stderr, "Error opening file %s with mode %s.\n", path, mode);
		exit(1);
		}

	return file;
	}

//----------------------------------------------------------------------------

unsigned long long TotalMemory()
	{
	return totalMemory;
	}

//----------------------------------------------------------------------------

