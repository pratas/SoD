#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "defs.h"
#include "mem.h"
#include "context.h"

//-----------------------------------------------------------------------------

static HCCounters zeroCounters = {0x00, 0x00, 0x00, 0x00};
static HCCounters auxCounters;

//-----------------------------------------------------------------------------

static void InitHashTable(CModel *cModel)
	{ 
	cModel->hTable.entries = (Entry **) Calloc(cModel->hTable.size, sizeof(Entry *));
	cModel->hTable.counters = (HCCounters **) Calloc(cModel->hTable.size, sizeof(HCCounters *));
	cModel->hTable.entrySize = (unsigned *) Calloc(cModel->hTable.size, sizeof(unsigned));
	cModel->hTable.nUsedEntries = 0;
	cModel->hTable.nUsedKeys = 0;
	}

//-----------------------------------------------------------------------------

static void InitArray(CModel *cModel)
	{
	cModel->array.counters = (ACCounter *) Calloc(cModel->nPModels * cModel->nSymbols, sizeof(ACCounter));
	}

//-----------------------------------------------------------------------------

static void InitThreshold(CModel *cModel)
        {
        cModel->threshold.idxOcc = (unsigned *) Calloc(cModel->threshold.sizeThreshold, sizeof(unsigned));
        cModel->threshold.symbols = (unsigned char *) Calloc(cModel->threshold.sizeThreshold, sizeof(unsigned char));
        }

//-----------------------------------------------------------------------------

static void InsertKey(HashTable *hTable, unsigned hIndex, unsigned key)
	{
	hTable->entries[hIndex] = (Entry *) Realloc(hTable->entries[hIndex], (hTable->entrySize[hIndex] + 1) * sizeof(Entry), sizeof(Entry));
	
	if(!hTable->entrySize[hIndex])
		hTable->nUsedEntries++;

	hTable->nUsedKeys++;
	hTable->entries[hIndex][hTable->entrySize[hIndex]].key = key;
	hTable->entrySize[hIndex]++;
	}

//-----------------------------------------------------------------------------

static void InsertCounters(HashTable *hTable, unsigned hIndex, unsigned nHCCounters, unsigned k, unsigned smallCounters)
	{
	hTable->counters[hIndex] = (HCCounters *) Realloc(hTable->counters[hIndex], (nHCCounters + 1) * sizeof(HCCounters), sizeof(HCCounters));

	if(k < nHCCounters)
		memmove(hTable->counters[hIndex][k + 1], hTable->counters[hIndex][k], (nHCCounters - k) * sizeof(HCCounters));

	hTable->counters[hIndex][k][0] = smallCounters & 0x03;
	hTable->counters[hIndex][k][1] = (smallCounters & (0x03 << 2)) >> 2;
	hTable->counters[hIndex][k][2] = (smallCounters & (0x03 << 4)) >> 4;
	hTable->counters[hIndex][k][3] = (smallCounters & (0x03 << 6)) >> 6;
	}

//-----------------------------------------------------------------------------

static HCCounter *GetHCCounters(HashTable *hTable, unsigned key)
	{
	unsigned k = 0, n;
	unsigned hIndex = key % hTable->size;

	for(n = 0 ; n < hTable->entrySize[hIndex] ; n++)
		{
		if(hTable->entries[hIndex][n].key == key)
			switch(hTable->entries[hIndex][n].counters)
				{
				case 0:
					return hTable->counters[hIndex][k];
				default:
					auxCounters[0] = hTable->entries[hIndex][n].counters & 0x03;
					auxCounters[1] = (hTable->entries[hIndex][n].counters & (0x03 << 2)) >> 2;
					auxCounters[2] = (hTable->entries[hIndex][n].counters & (0x03 << 4)) >> 4;
					auxCounters[3] = (hTable->entries[hIndex][n].counters & (0x03 << 6)) >> 6;
					return auxCounters;
				}
	
		if(hTable->entries[hIndex][n].counters == 0)
			k++;
		}

	return NULL;
	}

//-----------------------------------------------------------------------------

PModel *CreatePModel(unsigned nSymbols)
	{
	PModel *pModel;
	pModel = (PModel *) Malloc(sizeof(PModel));
	pModel->freqs = (unsigned long long *) Malloc(nSymbols * sizeof(unsigned long long));
	
	return pModel;
	}

//-----------------------------------------------------------------------------

void UpdateCModelCounter(CModel *cModel, unsigned pModelIdx, unsigned symbol)
	{
	unsigned n;
	ACCounter *aCounters;

	if(cModel->mode == HASH_TABLE_MODE)
		{
		unsigned char smallCounter;
		unsigned i, k = 0;
		unsigned nHCCounters;
		unsigned hIndex = pModelIdx % cModel->hTable.size;

		for(n = 0 ; n < cModel->hTable.entrySize[hIndex] ; n++)
			{
			if(cModel->hTable.entries[hIndex][n].key == pModelIdx)
				{
				if(cModel->hTable.entries[hIndex][n].counters == 0)
					{
					if(++cModel->hTable.counters[hIndex][k][symbol] == 255)
						for(i = 0 ; i < cModel->nSymbols ; i++)
							cModel->hTable.counters[hIndex][k][i] >>= 1;

					return;
					}
				
				smallCounter = (cModel->hTable.entries[hIndex][n].counters >> (symbol << 1)) & 0x03;
		
				if(smallCounter == 3)
					{
					nHCCounters = k;
					for(i = n + 1 ; i < cModel->hTable.entrySize[hIndex] ; i++)
						if(cModel->hTable.entries[hIndex][i].counters == 0)
							nHCCounters++;

					InsertCounters(&cModel->hTable, hIndex, nHCCounters, k, cModel->hTable.entries[hIndex][n].counters);
					cModel->hTable.entries[hIndex][n].counters = 0;
					cModel->hTable.counters[hIndex][k][symbol]++;
					return;
					}
				else
					{
					smallCounter++;
					cModel->hTable.entries[hIndex][n].counters &= ~(0x03 << (symbol << 1));
					cModel->hTable.entries[hIndex][n].counters |= (smallCounter << (symbol << 1));
					return;
					}

				}

			if(!cModel->hTable.entries[hIndex][n].counters)
				k++;
			}

		InsertKey(&cModel->hTable, hIndex, pModelIdx);
		cModel->hTable.entries[hIndex][cModel->hTable.entrySize[hIndex] - 1].counters = (0x01 << (symbol << 1));
		}
	else
		{
		aCounters = &cModel->array.counters[pModelIdx * cModel->nSymbols];
		aCounters[symbol]++;
		if(aCounters[symbol] == cModel->maxCount && cModel->maxCount != 0)
			for(n = 0 ; n < cModel->nSymbols ; n++)
				aCounters[n] >>= 1;
		}
	}

//-----------------------------------------------------------------------------

void RemoveCModelCounter(CModel *cModel, unsigned pModelIdx, unsigned char symbol)	
	{
	ACCounter *aCounters;

        aCounters = &cModel->array.counters[pModelIdx * cModel->nSymbols];

        if(aCounters[symbol] > 0)
                aCounters[symbol]--;
	else
		{
		printf("Internal error! (RemoveCModelCounter)\n");
		exit(1);
		}
	}

//-----------------------------------------------------------------------------

CModel *CreateCModel(unsigned maxCtxSize, unsigned nSymbols, unsigned nCtxSymbols, unsigned deltaNumEval, unsigned deltaDenEval, unsigned deltaNumGen, 
  unsigned deltaDenGen, unsigned maxCount, unsigned hSize, unsigned threshold)
	{
	CModel *cModel;
	
	cModel = (CModel *)Calloc(1, sizeof(CModel));

	if(maxCtxSize > 16)
		{
		fprintf(stderr, "Error: context size cannot be greater than 16\n");
		exit(1);
		}

	cModel->nPModels = (ULL)pow(nCtxSymbols, maxCtxSize);
	cModel->maxCtxSize = maxCtxSize;
	cModel->ctxSize = maxCtxSize;
	cModel->nSymbols = nSymbols;
	cModel->nCtxSymbols = nCtxSymbols;
	cModel->deltaNumEval = deltaNumEval;
	cModel->deltaDenEval = deltaDenEval;
	cModel->deltaNumGen = deltaNumGen;
	cModel->deltaDenGen = deltaDenGen;

	cModel->hTable.size = hSize;

	if((ULL)(cModel->nPModels) * nSymbols * sizeof(ACCounter) >> 20 > MAX_ARRAY_MEMORY)
		{
		cModel->mode = HASH_TABLE_MODE;
		cModel->maxCount = maxCount >> 8;
		InitHashTable(cModel);
		}
	else
		{
		cModel->mode = ARRAY_MODE;
		cModel->maxCount = maxCount;
		InitArray(cModel);
		}

	cModel->threshold.sizeThreshold = threshold;
	cModel->threshold.indexThreshold = 0;

	if(threshold != 0)
		InitThreshold(cModel);

	return cModel;
	}

//-----------------------------------------------------------------------------

void ComputePModel(CModel *cModel, PModel *pModel, unsigned long long pModelIdx)
	{
	int symbol;
	ACCounter *aCounters;
	HCCounter *hCounters;

	pModel->sum = 0;
	if(cModel->mode == HASH_TABLE_MODE)
		{
		if(!(hCounters = GetHCCounters(&cModel->hTable, pModelIdx)))
			hCounters = zeroCounters;

		for(symbol = 0 ; symbol < cModel->nSymbols ; symbol++)
			{
			pModel->freqs[symbol] = cModel->deltaNumEval + cModel->deltaDenEval * hCounters[symbol];
			pModel->sum += pModel->freqs[symbol];
			}
		}
	else
		{
		aCounters = &cModel->array.counters[pModelIdx * cModel->nSymbols];
		for(symbol = 0 ; symbol < cModel->nSymbols ; symbol++)
			{
			pModel->freqs[symbol] = cModel->deltaNumEval + cModel->deltaDenEval * aCounters[symbol];
			pModel->sum += pModel->freqs[symbol];
			}
		}
	}

//-----------------------------------------------------------------------------

double PModelSymbolNats(PModel *pModel, unsigned symbol)
	{
	return log((double)pModel->sum / pModel->freqs[symbol]);
	}

//-----------------------------------------------------------------------------

double FractionOfPModelsUsed(CModel *cModel)
	{
	unsigned pModel, symbol, sum, counter = 0;
	ACCounter *aCounters;
	HCCounter *hCounters;

	sum = 0;
	for(pModel = 0 ; pModel < cModel->nPModels ; pModel++)
		if(cModel->mode == HASH_TABLE_MODE)
			{
			hCounters = GetHCCounters(&(cModel->hTable), pModel);
			if(hCounters)
				counter++;
			}
		else
			{
			aCounters = &(cModel->array.counters[pModel * cModel->nSymbols]);
			for(symbol = 0 ; symbol < cModel->nSymbols ; symbol++)
				sum += aCounters[symbol];

			if(sum != 0)
				counter++;
			}

	return (double)counter / cModel->nPModels;
	}

//-----------------------------------------------------------------------------

double FractionOfPModelsUsedOnce(CModel *cModel)
	{
	unsigned pModelIdx;
	unsigned symbol, sum, counter = 0;
	ACCounter *aCounters;
	HCCounter *hCounters;

	sum = 0;
	for(pModelIdx = 0 ; pModelIdx < cModel->nPModels ; pModelIdx++)
		if(cModel->mode == HASH_TABLE_MODE)
			{
			hCounters = GetHCCounters(&(cModel->hTable), pModelIdx);

			if(!hCounters)
				continue;

			for(symbol = 0 ; symbol < cModel->nSymbols ; symbol++)
				sum += hCounters[symbol];

			if(sum == 1)
				counter++;
			}
		else
			{
			aCounters = &(cModel->array.counters[pModelIdx * cModel->nSymbols]);
			for(symbol = 0 ; symbol < cModel->nSymbols ; symbol++)
				sum += aCounters[symbol];

			if(sum == 1)
				counter++;
			}

	return (double)counter / cModel->nPModels;
	}

//-----------------------------------------------------------------------------

void HashingStats(CModel *cModel)

	{
	unsigned entry, n, k, emptyEntries = 0, nSmallCounters = 0, maxEntrySize = 0;
	ULL possibleKeys;
	double average = (double)cModel->hTable.nUsedKeys / cModel->hTable.size;
	double deviation = 0;

	for(entry = 0 ; entry < cModel->hTable.size ; entry++)
		{
		deviation += fabs(average - cModel->hTable.entrySize[entry]);

		if(!cModel->hTable.entrySize[entry])
			{
			emptyEntries++;
			continue;
			}

		if(cModel->hTable.entrySize[entry] > maxEntrySize)
			maxEntrySize = cModel->hTable.entrySize[entry];

		k = 0;
		for(n = 0 ; n < cModel->hTable.entrySize[entry] ; n++)
			{
			if(cModel->hTable.entries[entry][n].counters != 0)
				nSmallCounters++;

			if(cModel->hTable.entries[entry][n].counters == 0)
				k++;
			}
		}

	possibleKeys = powl((double)cModel->nSymbols, (double)cModel->maxCtxSize);

	printf("Hash size ......... %u\n", cModel->hTable.size);
	printf("Used entries ...... %u\n", cModel->hTable.nUsedEntries);
	printf("Ideal entry size .. %.1f\n", average);
	printf("Deviation ......... %.1f\n", deviation / cModel->hTable.size);
	printf("Used keys ......... %u [%.2f %% of %llu]\n", cModel->hTable.nUsedKeys, 100.0 * (double)cModel->hTable.nUsedKeys / possibleKeys, possibleKeys);
	printf("Small counters .... %u\n", nSmallCounters);
	printf("Large counters .... %u\n", cModel->hTable.nUsedKeys-nSmallCounters);
	printf("Max entry size .... %u\n", maxEntrySize);
	}

//-----------------------------------------------------------------------------

