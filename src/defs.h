#ifndef DEFS_H_INCLUDED
#define DEFS_H_INCLUDED

#define N_SYMBOLS                               4
#define DEFINE_MAX_FILE_NAME                    256
#define MAX_LINE_SIZE                           512
#define DEFAULT_PMODEL_DELTA_NUM_GENERATION     1
#define DEFAULT_PMODEL_DELTA_DEN_GENERATION     10
#define DEFAULT_PMODEL_DELTA_NUM_EVALUATION     1
#define DEFAULT_PMODEL_DELTA_DEN_EVALUATION     1
#define	DEFAULT_BLOCK_SIZE_EVALUATION           100
#define DEFAULT_BLOCK_TABLE_SIZE_GENERATION     10000
#define	DEFAULT_ARGS_PROTECTION                 3
#define DEFAULT_PMODEL_MAX_COUNT                ((1ULL << (sizeof(ACCounter) * 8)) - 1)
#define DEFAULT_THRESHOLD                       0
#define HASHING_TABLE_SIZE                      19999999
#define DEFAULT_NUMBER_SEQUENCES                1
#define MAX_ARRAY_MEMORY                        1024     //size in MB
#define MAX_FILENAME_SIZE                       512
#define ARRAY_MODE                              0
#define HASH_TABLE_MODE                         1

#endif
