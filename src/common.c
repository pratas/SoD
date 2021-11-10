#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "mem.h"
#include "common.h"

//-----------------------------------------------------------------------------

void ShiftBuffer(Symbol *buf, int bufSize, Symbol newSymbol)
	{
	memmove(buf, buf + 1, bufSize * sizeof(Symbol));
	buf[bufSize - 1] = newSymbol;
	}

//-----------------------------------------------------------------------------

unsigned long long GetPModelIdx(Symbol *symbol, CTemplate *cTemplate)
	{
	unsigned long long n, idx = 0;

	for(n = 0 ; n < cTemplate->size ; n++)
		idx += *(symbol - cTemplate->position[n]) << (n << 1);

	return idx;
	}

//-----------------------------------------------------------------------------

unsigned long long GetPModelIdxC(Symbol *symbol, CTemplate *cTemplate)
	{
	unsigned long long n, idx = 0;

	symbol++;
	for(n = 0 ; n < cTemplate->size ; n++)
		idx += (3 - *(symbol - cTemplate->position[n])) <<
		  ((cTemplate->size - n - 1) << 1);

	return idx;
	}

//-----------------------------------------------------------------------------

unsigned GetSideInfoPModelIdx(Symbol *buf, int bufSize, CModel *cModel, CTemplate *cTemplate)
	{
	unsigned n, idx = 0, w = 1;

	for(n = 0 ; n < cTemplate->size ; n++)
		{
		idx += buf[bufSize - cTemplate->position[n] - 1] * w;
		w *= cModel->nSymbols;
		}

	return idx;
	}

//-----------------------------------------------------------------------------

void IncreaseTemplateStorage(CTemplate *cTemplate, int additionalSize)
	{
	if(cTemplate->size == 0)
		cTemplate->position = (int *) Malloc(additionalSize * sizeof(int));
	else
		cTemplate->position = (int *) Realloc(cTemplate->position, (additionalSize + cTemplate->size) * sizeof(int), additionalSize * sizeof(int));
	}

//-----------------------------------------------------------------------------
