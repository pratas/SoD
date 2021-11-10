#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

typedef unsigned int        ACCounter;  //Size of context counters for arrays
typedef unsigned char       HCCounter;  //Size of context counters for hash tables
typedef unsigned long long  ULL;
typedef HCCounter           HCCounters[4];

typedef struct
	{
	unsigned        cModel;         //current cModel for generation
	unsigned        nBlock;         //current number of blocks for generation
	}
Signal;

typedef struct
	{
	unsigned        key;            //The key stored in this entry
	unsigned char   counters;       //"Small" counters: 2 bits for each one
	}
Entry;

typedef struct
	{
	unsigned        size;           //Size of the hash table
	unsigned        *entrySize;     //Number of keys in this entry
	Entry           **entries;      //The heads of the hash table lists
	HCCounters      **counters;     //The context counters
	unsigned        nUsedEntries;
	unsigned        nUsedKeys;
	}
HashTable;

typedef struct
	{
	ACCounter       *counters;
	}
Array;

typedef struct
	{
	unsigned        sizeThreshold;  //Threshold size
	unsigned        indexThreshold; //Current position in the threshold
	unsigned        *idxOcc;        //Index of the string by number of occurrences
	unsigned char   *symbols;       //symbol for each threshold occureence
	}
Threshold;

typedef struct
	{
	unsigned long long    *freqs;
	unsigned long long    sum;
	}
PModel;

typedef struct
	{
	unsigned        maxCtxSize;     //Maximum depth of context template
	unsigned        ctxSize;        //Current depth of context template 
	unsigned        nSymbols;       //Number of coding symbols
	unsigned        nCtxSymbols;    //Number of symbols used for context computation
	ULL             nPModels;       //Maximum number of probability models
	unsigned        deltaNumEval;   //Numerator of delta
	unsigned        deltaDenEval;   //Denominator of delta
	unsigned        deltaNumGen;    //Numerator of delta for generation
	unsigned        deltaDenGen;    //Denominator of delta for generation
	unsigned        maxCount;       //Counters /= 2 if one counter >= maxCount
	unsigned        mode;
	Threshold       threshold;      //Threshold of context template
	HashTable       hTable;
	Array           array;
	}
CModel;

PModel *CreatePModel(unsigned nSymbols);
void UpdateCModelCounter(CModel *cModel, unsigned pModelIdx, unsigned symbol);
void RemoveCModelCounter(CModel *cModel, unsigned pModelIdx, unsigned char symbol);
void CreateBlockModel(unsigned nCModels);
CModel *CreateCModel(unsigned maxCtxSize, unsigned nSymbols, unsigned nCtxSymbols, unsigned deltaNumEval, unsigned deltaDenEval, unsigned deltaNumGen, 
  unsigned deltaDenGen, unsigned maxCount, unsigned hSize, unsigned threshold);
double FractionOfPModelsUsed(CModel *cModel);
double FractionOfPModelsUsedOnce(CModel *cModel);
void ComputePModel(CModel *cModel, PModel *pModel, unsigned long long pModelIdx);
double PModelSymbolNats(PModel *pModel, unsigned symbol);
void HashingStats(CModel *cModel);

#endif


