#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include "defs.h"
#include "context.h"

typedef unsigned char Symbol;

typedef struct
	{
	int size;
	int *position;
	int deepestPosition;
	}
CTemplate;

//-----------------------------------------------------------------------------

void ShiftBuffer(Symbol *buf, int bufSize, Symbol newSymbol);
unsigned long long GetPModelIdx(Symbol *buf, CTemplate *cTemplate);
unsigned long long GetPModelIdxC(Symbol *buf, CTemplate *cTemplate);
unsigned GetSideInfoPModelIdx(Symbol *buf, int bufSize, CModel *cModel, CTemplate *cTemplate);
void IncreaseTemplateStorage(CTemplate *cTemplate, int additionalSize);

#endif


