#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <malloc.h>
#include "mem.h"
#include "common.h"

//-----------------------------------------------------------------------------

void ParseListOfPositions(char *str, CTemplate *cTemplate)

	{
	int n, low, high;

	cTemplate->size = 0;
	while(1)
		{
		switch(sscanf(str, "%d:%d", &low, &high))
			{
			case 1:
				if(low == 0)
					return;

				if(low < 0)
					{
					fprintf(stderr,"Error: can't handle non-causal contexts\n");
					exit(1);
					}

				IncreaseTemplateStorage(cTemplate, 1);
				cTemplate->position[cTemplate->size++] = low;
				break;

			case 2:
				if(low <= 0)
					{
					fprintf(stderr,"Error: can't handle non-causal contexts\n");
					exit(1);
					}

				if(high - low + 1 > 0)
					{
					IncreaseTemplateStorage(cTemplate, high - low + 1);
					for(n = 0 ; n < high - low + 1 ; n++)
						cTemplate->position[cTemplate->size++] = low + n;
					}

				break;

			default: return;
			}

		while(*str != '\0' && *str++ != ',')
			;
		}
	}

//-----------------------------------------------------------------------------

int GetBlock(FILE *fp, Symbol *block, int bSize, int deepestPosition, char *alphabet)

	{
	char *ptr;
	int c;
	unsigned nBases = 0;

	memcpy(block, block + bSize, deepestPosition * sizeof(Symbol));
	block += deepestPosition;

	while(nBases < bSize)
		{
		if((c = fgetc(fp)) == EOF)
			break;

		if(c == '>' || c == '#')
			{
			while((c = fgetc(fp)) != EOF && c != '\n')
				;

			continue;
			}

		if((ptr = strchr(alphabet, toupper(c))))
			block[nBases++] = ptr - alphabet;
		}

	return nBases;
	}

//-----------------------------------------------------------------------------

unsigned long long CountBases(FILE *file, char *alphabet)

	{
	int c;
	unsigned long long nBases = 0;

	while((c = fgetc(file)) != EOF)
		{
		if(c == '>' || c == '#')
			{
			while((c = fgetc(file)) != EOF && c != '\n')
				;

			continue;
			}

		if(strchr(alphabet, toupper(c)))
			nBases++;
		}

	return nBases;
	}

//-----------------------------------------------------------------------------

void OutputSymbol(FILE *file, int symbol)

	{
	switch(symbol)
		{
		case 0: fputc('A', file); break;
		case 1: fputc('C', file); break;
		case 2: fputc('G', file); break;
		case 3: fputc('T', file); break;
		}
	}

//-----------------------------------------------------------------------------

int GenSymbol(PModel *pModel)

	{
        unsigned symbol;
	unsigned long long sum = 0;
        long double rnd = drand48() * pModel->sum;

        for(symbol = 0 ; symbol < N_SYMBOLS ; symbol++)
                {
                if(rnd <= sum + pModel->freqs[symbol])
                        return symbol;
                else
                        sum += pModel->freqs[symbol];
                }

	printf("Internal error (generating symbol)\n");
	exit(1);
	}

//-----------------------------------------------------------------------------

void GenCModel(unsigned **blockTable, unsigned nCModels, unsigned blockTableSize, Signal *signal) 

	{
	unsigned cModel, block, sum = 0;
	double rnd = drand48() * blockTable[nCModels][blockTableSize];

	for(block = 0 ; block < blockTableSize ; block++)
		for(cModel = 0 ; cModel < nCModels ; cModel++)
			{
			if(rnd <= sum + blockTable[cModel][block])
				{
				signal->cModel = cModel;
				signal->nBlock = block + 1;
				return;
				}
			else
				sum += blockTable[cModel][block];
			}

	printf("Internal error (generating cModel)\n");
	exit(1);
	}

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])

	{
	FILE *fp, *fpGenOut = NULL, *fpBlockInfo = NULL, *fpConfig = NULL;
	char *updateCModel, updateInvRepeats, verbose, help, fName[DEFINE_MAX_FILE_NAME], 
	  *alphabet = "ACGT", **xargv, setFpGenOut, setSeed, fNameGen[MAX_FILENAME_SIZE],
	  **filesGen;
	double bestCModelNats, *cModelNats, *cModelTotalNats, nNats, blockNats;
	unsigned nBases, nCModels, cModel, model, deepestPosition, bSize, nBlocks, bestCModel = 0,
	  *cModelUsage, deltaNumEval, deltaDenEval, deltaNumGen, deltaDenGen, blockIndex,
	  maxCount = DEFAULT_PMODEL_MAX_COUNT, hSize = HASHING_TABLE_SIZE, bufSize, n, 
	  modelIndex, threshold = DEFAULT_THRESHOLD, **blockTable, *signalModelBlock, 
	  bestCModelCount = 0, bestCModelTmp = 0, blockTableSize, xargc,
	  nSeq, fLenght, numbSequences = DEFAULT_NUMBER_SEQUENCES;
	unsigned long seed = 0;
	unsigned long long totalBases, pModelIdx, idxBase = 0, nGenSymbols, iSymbol = 0;
	

	Symbol *block, symbol, symbolC, *buf;
	CModel **cModels;
	CTemplate *cTemplate;
	PModel *pModel;
	Signal *signal;
	
	clock_t ticCompress, tacCompress, startCompress;
	clock_t ticGenerate, startGenerate;
	double cpuTimeUsedCompress;
	double cpuTimeUsedGenerate;

	startCompress = clock();

	updateInvRepeats = 'n';
	verbose = 'n';
	help = 'n';
	setSeed = 'n';
	setFpGenOut = 'n';
	bSize = DEFAULT_BLOCK_SIZE_EVALUATION;
	blockTableSize = DEFAULT_BLOCK_TABLE_SIZE_GENERATION;
	nGenSymbols = 0;
	nCModels = 0;

	xargv = (char **) Malloc(argc * sizeof(char *));
	for (n = 0 ; n < argc - 1; n++) 
		{
		xargv[n] = (char *) Malloc((strlen(argv[n]) + 1) * sizeof(char));
		strcpy(xargv[n],argv[n]);
		}

	xargc = argc - 1;

	for(n = 1 ; n < argc ; n++)
		if(strcmp("-conf", argv[n]) == 0)
			{
			if((fpConfig = Fopen(argv[n+1], "r")) != NULL)
				{
				char line[256];
				while (fgets(line, sizeof(line), fpConfig) != NULL) 
					{
					if(line[0] == '#') continue;
					if(line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';

					char *result = NULL;
					result = strtok(line," ");
					while(result != NULL) 
						{
						xargv = (char **) Realloc(xargv, (xargc+1) * sizeof(char *), sizeof(char *));
						xargv[xargc] = (char *) Malloc((strlen(result) + 1) * sizeof(char));
						strcpy(xargv[xargc++],result);
						result = strtok(NULL," ");
						}		
					}
				}
			break;
			}

	for(n = 0 ; n < DEFAULT_ARGS_PROTECTION ; n++)
		{
        	xargv = (char **) Realloc(xargv, (xargc+1) * sizeof(char *), sizeof(char *));
             	xargv[xargc] = (char *) Malloc((strlen("Empty") + 1) * sizeof(char));
	        strcpy(xargv[xargc++],"Empty");
		}

	for(n = 1 ; n < argc ; n++)
		if(strcmp("-h", argv[n]) == 0)
			{
			help = 'y';
			break;
			}

	if(argc < 2 || help != 'n')
		{
		fprintf(stderr, "Usage: SoD     [ -h (show this) ]\n");
		fprintf(stderr, "               [ -v (verbose info) ]\n");
		fprintf(stderr, "               [ -t <template> a=<n>/<d> d=<n>/<d> tr=<threshold> ]\n");
		fprintf(stderr, "               [ -t <template> a=<n>/<d> d=<n>/<d> tr=<threshold> ]\n");
		fprintf(stderr, "               ...\n");
		fprintf(stderr, "               [ -u <template> a=<n>/<d> d=<n>/<d> tr=<threshold> ]\n");
		fprintf(stderr, "               [ -u <template> a=<n>/<d> d=<n>/<d> tr=<threshold> ]\n");
		fprintf(stderr, "               ...\n");
		fprintf(stderr, "               [ -bs <bSize> (def: %u) ]\n", bSize);
		fprintf(stderr, "               [ -bts <bTableSize> (def: %u) ]\n", blockTableSize);
		fprintf(stderr, "               [ -n <nGenSymbols> (def: nBasesDataFile) ]\n");
		fprintf(stderr, "               [ -o <genOutFile> ]\n");
		fprintf(stderr, "               [ -ic (inverted complement repeats)]\n");
		fprintf(stderr, "               [ -ns <nGenSequences> (def: %d) ]\n", DEFAULT_NUMBER_SEQUENCES);
		fprintf(stderr, "               [ -s <seed> (def: TIME(NULL)) ]\n");
		fprintf(stderr, "               [ -mc <maxCount> (def: %llu) ]\n", DEFAULT_PMODEL_MAX_COUNT);
		fprintf(stderr, "               [ -hs <hashSize> (def: %d) ]\n", hSize);
		fprintf(stderr, "               [ -conf <configFile> ]\n");	
		fprintf(stderr, "               [ -bInfo <blockInfoFile> ]\n");
		fprintf(stderr, "               <dataFile>\n");
		exit(1);
		}

	for(n = 1 ; n < xargc ; n++)
		if(strcmp("-v", xargv[n]) == 0)
			{
			verbose = 'y';
			break;
			}

	for(n = 1 ; n < xargc ; n++)
		if(strcmp("-t", xargv[n]) == 0)
			nCModels++;

	for(n = 1 ; n < xargc ; n++)
		if(strcmp("-u", xargv[n]) == 0)
			nCModels++;

	for(n = 1 ; n < xargc ; n++)
		if(strcmp("-bs", xargv[n]) == 0)
			{
			bSize = atoi(xargv[n+1]);
			break;
			}

	for(n = 1 ; n < xargc ; n++)
		if(strcmp("-bts", xargv[n]) == 0)
			{
			blockTableSize = atoi(xargv[n+1]);
			break;
			}

	for(n = 1 ; n < xargc ; n++) 
		if(strcmp("-n", xargv[n]) == 0)
			{
			nGenSymbols = atol(xargv[n+1]);
			break;
			}

	for(n = 1 ; n < argc ; n++) 
		if(strcmp("-ns", argv[n]) == 0)
			{
			numbSequences = atoi(argv[n+1]);
			break;
			}

	for(n = 1 ; n < xargc ; n++)
		if(strcmp("-ic", xargv[n]) == 0)
			{
			updateInvRepeats = 'y';
			break;
			}

	for(n = 1 ; n < xargc ; n++) 
		if(strcmp("-s", xargv[n]) == 0)
			{
			seed = atol(xargv[n+1]);
			break;
			}

	for(n = 1 ; n < xargc ; n++)
		if(!strcmp("-mc", xargv[n]))
			{
			maxCount = atol(xargv[n+1]);
			if(maxCount == 0 || maxCount > DEFAULT_PMODEL_MAX_COUNT)
				fprintf(stderr, "Warning (maxCount): counters may overflow\n");

			break;
			}

	for(n = 1 ; n < xargc ; n++)
		if(strcmp("-hs", xargv[n]) == 0)
			{
			hSize = atol(xargv[n+1]);
			break;
			}

	for(n = 1 ; n < argc ; n++)
		if(strcmp("-o", argv[n]) == 0)
			{
			strcpy(fNameGen, argv[n+1]);
			setFpGenOut = 'y';
			break;
			}

	for(n = 1 ; n < xargc ; n++)
		if(strcmp("-bInfo", xargv[n]) == 0)
			{
			fpBlockInfo = Fopen(xargv[n+1], "w");
			break;
			}

	if(setFpGenOut == 'n')
		{    
		#if defined(WIN32) || defined(MSDOS)
		fprintf(stderr,"Error: undifined output file\n");
		exit(1);
		#else
		fprintf(stderr,"Warning: you didn't specify an output file !\n");
		fprintf(stderr,"Warning: writing to /dev/null !\n");
		fpGenOut = fopen("/dev/null", "w");
		#endif
		}

	if(nCModels == 0)
		{
		fprintf(stderr,"Error: at least one context model must be specified\n");
		return 1;
		}

	cTemplate = (CTemplate *)Calloc(nCModels, sizeof(CTemplate));
	cModels = (CModel **)Calloc(nCModels, sizeof(CModel *));
	updateCModel = (char *)Malloc(nCModels * sizeof(char));
	cModelNats = (double *)Calloc(nCModels, sizeof(double));
	cModelTotalNats = (double *)Calloc(nCModels, sizeof(double));
	cModelUsage = (unsigned *)Calloc(nCModels, sizeof(unsigned));
	blockTable = (unsigned **)Calloc(nCModels + 1, sizeof(unsigned *));
	signalModelBlock = (unsigned *)Calloc(3, sizeof(unsigned));
	filesGen = (char **)Malloc(numbSequences * sizeof(char*));
	
	for(modelIndex = 0 ; modelIndex <= nCModels ; modelIndex++)
		blockTable[modelIndex] = (unsigned *)Calloc(blockTableSize + 1, sizeof(unsigned));
			

	signal = (Signal *)Calloc(1, sizeof(Signal));
		
	for(modelIndex = 0 ; modelIndex < nCModels ; modelIndex++)
		for(blockIndex = 0 ; blockIndex < blockTableSize ; blockIndex++)
			blockTable[modelIndex][blockIndex] = 1;
	
	cModel = 0;
	for(n = 1 ; n < xargc ; n++)
		{
		if(strcmp("-t", xargv[n]) == 0)
			{
			ParseListOfPositions(xargv[n+1], &cTemplate[cModel]);
			updateCModel[cModel++] = 'n';
			}

		if(strcmp("-u", xargv[n]) == 0)
			{
			ParseListOfPositions(xargv[n+1], &cTemplate[cModel]);
			updateCModel[cModel++] = 'y';
			}
		}

	fp = Fopen(argv[argc-1], "r");
	fclose(fp);

	sprintf(fName, "zcat -f %s", argv[argc-1]);
    	if((fp = popen(fName, "r")) == NULL)
		{
		fprintf(stderr, "Error: unable to open file\n");
		return 1;
		}

	totalBases = CountBases(fp, alphabet);

	if(nGenSymbols == 0)
		nGenSymbols = totalBases;

	ticCompress = clock();
	cpuTimeUsedCompress = ((double) (ticCompress-startCompress)) / CLOCKS_PER_SEC;
    	printf("Needed %g s for counting the bases\n", cpuTimeUsedCompress);
	printf("Evaluating %llu bases\n", totalBases);

	pclose(fp);
    	if((fp = popen(fName, "r")) == NULL)
		{
		fprintf(stderr, "Error: unable to open file\n");
		return 1;
		}

	deepestPosition = 0;
	for(cModel = 0 ; cModel < nCModels ; cModel++)
		{
		if(cTemplate[cModel].size == 0)
			continue;

		cTemplate[cModel].deepestPosition = cTemplate[cModel].position[0];
		for(n = 1 ; n < cTemplate[cModel].size ; n++)
			if(cTemplate[cModel].position[n] > cTemplate[cModel].deepestPosition)
				cTemplate[cModel].deepestPosition = cTemplate[cModel].position[n];

		if(deepestPosition < cTemplate[cModel].deepestPosition)
			deepestPosition = cTemplate[cModel].deepestPosition;
		}

	cModel = 0;
	for(n = 1 ; n < xargc ; n++)
		{
		if(strcmp("-t", xargv[n]) == 0 || strcmp("-u", xargv[n]) == 0)
			{
			if(sscanf(xargv[n+2], "a=%d/%d", &deltaNumEval, &deltaDenEval) !=2 && 
			  sscanf(xargv[n+3], "a=%d/%d", &deltaNumEval, &deltaDenEval) !=2 &&
			  sscanf(xargv[n+4], "a=%d/%d", &deltaNumEval, &deltaDenEval) !=2) 
                                {
                                deltaNumEval = DEFAULT_PMODEL_DELTA_NUM_EVALUATION;
                                deltaDenEval = DEFAULT_PMODEL_DELTA_DEN_EVALUATION;
                                }
			
			if(sscanf(xargv[n+2], "d=%d/%d", &deltaNumGen, &deltaDenGen) !=2 &&
			  sscanf(xargv[n+3], "d=%d/%d", &deltaNumGen, &deltaDenGen) !=2 &&
			  sscanf(xargv[n+4], "d=%d/%d", &deltaNumGen, &deltaDenGen) !=2)
				{
				deltaNumGen = DEFAULT_PMODEL_DELTA_NUM_GENERATION;
				deltaDenGen = DEFAULT_PMODEL_DELTA_DEN_GENERATION;
				}

			if(sscanf(xargv[n+2], "tr=%u", &threshold) != 1 && 
			  sscanf(xargv[n+3], "tr=%u", &threshold) != 1 && 
			  sscanf(xargv[n+4], "tr=%u", &threshold) != 1)
                                threshold = DEFAULT_THRESHOLD;

			cModels[cModel] = CreateCModel(cTemplate[cModel].size, N_SYMBOLS, N_SYMBOLS, deltaNumEval, deltaDenEval, deltaNumGen, deltaDenGen, maxCount, hSize, threshold);

 			if(threshold == 0)
				{				
				printf("Creating %llu probability models (template size: %d, alpha: %u/%u, delta = %u/%u)\n", 
				  cModels[cModel]->nPModels, cModels[cModel]->ctxSize, deltaNumEval, deltaDenEval, deltaNumGen, deltaDenGen);				
				}
			else
				{
				printf("Creating %llu probability models (template size: %d, alpha: %u/%u, delta = %u/%u, threshold = %u)\n", 
				  cModels[cModel]->nPModels, cModels[cModel]->ctxSize, deltaNumEval, deltaDenEval, deltaNumGen, deltaDenGen, threshold);
				}
	
			cModel++;
			}
		}

	pModel = CreatePModel(N_SYMBOLS);

	block = (Symbol *)Calloc(bSize + deepestPosition, sizeof(Symbol));		

	printf("Using blocks of %d bases (%llu blocks)\n", bSize, (totalBases - 1) / bSize + 1);

	nBlocks = 0;
	do
		{
		if(!(nBases = GetBlock(fp, block, bSize, deepestPosition, alphabet)))
			break;

		if(nCModels > 1)
			{
			for(n = deepestPosition ; n < deepestPosition + nBases ; n++)
				{
				symbol = block[n];
				for(cModel = 0 ; cModel < nCModels ; cModel++)
					{
					pModelIdx = GetPModelIdx(block + n, &cTemplate[cModel]);
					ComputePModel(cModels[cModel], pModel, pModelIdx);
					cModelNats[cModel] += PModelSymbolNats(pModel, symbol);
					}
				}

			bestCModelNats = cModelNats[0];
			bestCModel = 0;
			for(cModel = 1 ; cModel < nCModels ; cModel++)
				if(cModelNats[cModel] < bestCModelNats)
					{
					bestCModelNats = cModelNats[cModel];
					bestCModel = cModel;
					}

			if(bestCModelCount == blockTableSize - 1)
				{
				blockTable[bestCModel][blockTableSize - 1]++;
				bestCModelCount = 0;
				}
			else if(bestCModel == bestCModelTmp)
					bestCModelCount++;
				else
					{
					blockTable[bestCModel][bestCModelCount]++;
					bestCModelCount = 0;
					}

			bestCModelTmp = bestCModel;
			}

		blockNats = 0;
		cModelUsage[bestCModel]++;
		for(n = deepestPosition ; n < deepestPosition + nBases ; n++)
			{
			symbol = block[n];
			pModelIdx = GetPModelIdx(block + n, &cTemplate[bestCModel]);
			ComputePModel(cModels[bestCModel], pModel, pModelIdx);
			cModelTotalNats[bestCModel] += PModelSymbolNats(pModel, symbol);
			nNats = PModelSymbolNats(pModel, symbol);
			blockNats += nNats;

			UpdateCModelCounter(cModels[bestCModel], pModelIdx, symbol);
			if(updateInvRepeats == 'y')
				{
				pModelIdx = GetPModelIdxC(block + n, &cTemplate[bestCModel]);
				symbolC = 3 - block[n - cTemplate[bestCModel].deepestPosition];
				UpdateCModelCounter(cModels[bestCModel], pModelIdx, symbolC);
				}

			for(cModel = 0 ; cModel < nCModels ; cModel++)
				{
				if(cModels[cModel]->threshold.sizeThreshold != 0 &&
		                   idxBase > cModels[cModel]->threshold.sizeThreshold)
					RemoveCModelCounter(cModels[cModel], cModels[cModel]->threshold.idxOcc[cModels[cModel]->
					  threshold.indexThreshold], cModels[cModel]->threshold.symbols[cModels[cModel]->
					    threshold.indexThreshold]);
			                     
				if(cModels[cModel]->threshold.sizeThreshold != 0)
					{
					pModelIdx = GetPModelIdx(block + n, &cTemplate[cModel]);
		                	cModels[cModel]->threshold.idxOcc[cModels[cModel]->threshold.indexThreshold] = pModelIdx;
		                	cModels[cModel]->threshold.symbols[cModels[cModel]->threshold.indexThreshold] = symbol;

		                        if(cModels[cModel]->threshold.indexThreshold == cModels[cModel]->threshold.sizeThreshold - 1)
		                                cModels[cModel]->threshold.indexThreshold = 0;
		                        else
		                                cModels[cModel]->threshold.indexThreshold++;
					}	
				}

	
			for(cModel = 0 ; cModel < nCModels ; cModel++)
				{
				if(cModel == bestCModel)
					continue;

				if(updateCModel[cModel] == 'y' || cModels[cModel]->threshold.sizeThreshold != 0)
					{
					pModelIdx = GetPModelIdx(block + n, &cTemplate[cModel]);
					UpdateCModelCounter(cModels[cModel], pModelIdx, symbol);

					if(updateInvRepeats == 'y') 
						{
						pModelIdx = GetPModelIdxC(block + n,&cTemplate[cModel]);
						symbolC = 3 - block[n - cTemplate[cModel].deepestPosition];
						UpdateCModelCounter(cModels[cModel], pModelIdx, symbolC);
						}
					}
				}

			if(idxBase % (totalBases / 100) == 0 && totalBases >= 100)
				fprintf(stderr, "%3d%%\r", (int)(idxBase / (totalBases / 100)));

			idxBase++;
			}

		if(nCModels > 1)
			for(cModel = 0 ; cModel < nCModels ; cModel++)
				cModelNats[cModel] = 0;

		nBlocks++;
		}
	while(nBases == bSize);

	for(modelIndex = 0 ; modelIndex < nCModels ; modelIndex++)
		for(blockIndex = 0 ; blockIndex < blockTableSize ; blockIndex++)
			blockTable[nCModels][blockIndex] += blockTable[modelIndex][blockIndex];

	for(blockIndex = 0 ; blockIndex < blockTableSize ; blockIndex++)
		for(modelIndex = 0 ; modelIndex < nCModels ; modelIndex++)
			blockTable[modelIndex][blockTableSize] += blockTable[modelIndex][blockIndex];

	for(blockIndex = 0 ; blockIndex < blockTableSize ; blockIndex++)
		blockTable[nCModels][blockTableSize] += blockTable[nCModels][blockIndex];
	
	if(fpBlockInfo)
		for(blockIndex = 0 ; blockIndex < blockTableSize ; blockIndex++)
			{
			for(modelIndex = 0 ; modelIndex < nCModels ; modelIndex++)
				fprintf(fpBlockInfo, "%u\t", blockTable[modelIndex][blockIndex] - 1);
			fprintf(fpBlockInfo, "\n");
			}

	tacCompress = clock();

	fclose(fp);

	cpuTimeUsedCompress = ((double) (tacCompress - ticCompress)) / CLOCKS_PER_SEC;
    	printf("Needed %g s for evaluation\n", cpuTimeUsedCompress);

	fflush(stdout);

	startGenerate = clock(); 

	bufSize = deepestPosition + 1;
	buf = (Symbol *)Calloc(bufSize + 1, sizeof(Symbol));

	for(cModel = 0 ; cModel < nCModels ; cModel++)
		{
		cModels[cModel]->deltaNumEval = cModels[cModel]->deltaNumGen;
		cModels[cModel]->deltaDenEval = cModels[cModel]->deltaDenGen;
		}

	if(verbose == 'y')
		printf("Block table size: %d\n", blockTableSize);

	fLenght = strlen(fNameGen);

	for(nSeq = 0 ; nSeq < numbSequences ; nSeq++)
		{
		printf("Generating %llu bases", nGenSymbols);
		if(numbSequences > 1)
			printf(" for sequence %u", nSeq + 1);
		printf("\n");

		if(setFpGenOut == 'y')
			{
			filesGen[nSeq] = (char*) Malloc((fLenght+4) * sizeof(char));
			strcpy(filesGen[nSeq], fNameGen);
			if(numbSequences > 1)
				{
				sprintf(filesGen[nSeq] + fLenght, ".f");
				sprintf(filesGen[nSeq] + fLenght + 2, "%d", nSeq+1);
				}
			fpGenOut = fopen(filesGen[nSeq], "w");
			}

		//SEED
		if(setSeed == 'n')
			seed = time(NULL);
		
		srand48((long)seed);

		if(verbose == 'y')
			printf("Using seed: %lu\n", seed); 

		do
			{
			ShiftBuffer(buf, bufSize, 0);

			if(signal->nBlock == 0)
				GenCModel(blockTable, nCModels, blockTableSize, signal);

			pModelIdx = GetPModelIdx(buf + deepestPosition, &cTemplate[signal->cModel]);

			ComputePModel(cModels[signal->cModel], pModel, pModelIdx);
			symbol = GenSymbol(pModel);
			buf[bufSize - 1] = symbol; 

			OutputSymbol(fpGenOut, symbol);

			for(model = 0 ; model < nCModels ; model++)
				{
				if(cModels[model]->threshold.sizeThreshold != 0)
					RemoveCModelCounter(cModels[model], cModels[model]->threshold.idxOcc[cModels[model]->
					  threshold.indexThreshold], cModels[model]->threshold.symbols[cModels[model]->
					    threshold.indexThreshold]);
	   
				if(cModels[model]->threshold.sizeThreshold != 0)
					{
					pModelIdx = GetPModelIdx(buf + deepestPosition, &cTemplate[model]);
			        	cModels[model]->threshold.idxOcc[cModels[model]->threshold.indexThreshold] = pModelIdx;
			        	cModels[model]->threshold.symbols[cModels[model]->threshold.indexThreshold] = symbol;

			                if(cModels[model]->threshold.indexThreshold == cModels[model]->threshold.sizeThreshold - 1)
			                        cModels[model]->threshold.indexThreshold = 0;
			                else
			                        cModels[model]->threshold.indexThreshold++;

					UpdateCModelCounter(cModels[model], pModelIdx, symbol);
					}	
				}


			if(iSymbol % (nGenSymbols / 100) == 0 && nGenSymbols > 100)
				fprintf(stderr, "%3d%%\r", (int) (iSymbol / (nGenSymbols / 100)));

			if(nGenSymbols == ++iSymbol)
				break;

			signal->nBlock--;
			}
		while(nGenSymbols > iSymbol);

		if(setFpGenOut == 'y')
			fclose(fpGenOut);

		printf("Done!\n");

		iSymbol = 0;
		}

	ticGenerate = clock();
	cpuTimeUsedGenerate = ((double) (ticGenerate-startGenerate)) / CLOCKS_PER_SEC;
	printf("Needed %g s for generation\n", cpuTimeUsedGenerate);
	printf("Total memory: %.1f MB\n", TotalMemory() / 1024. / 1024);
	printf("Total time: %.3g s\n", cpuTimeUsedGenerate + cpuTimeUsedCompress);

	if(fpBlockInfo) fclose(fpBlockInfo);

	return EXIT_SUCCESS;
	}

