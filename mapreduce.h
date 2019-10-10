#ifndef MAPREDUCE_H
#define MAPREDUCE_H

// function pointer types used by library functions
typedef void (*Mapper)(char *file_name);
typedef void (*Reducer)(char *key, int partition_number);


// library functions you must define

void MR_Run(int num_files, char *filenames[],
            Mapper map, int num_mappers,
            Reducer concate, int num_reducers);

/*
MR_Emit takes a key and a value associated with it, and writes this pair to a 
specific partition which is determined by passing the key to MR_Partition 
*/
void MR_Emit(char *key, char *value);

// TODO: remember to cite source of hash function algorithm
unsigned long MR_Partition(char *key, int num_partitions);

/*
MR_ProcessPartition takes the index of the partition assigned to the thread
that runs it. It invokes the user-defined Reduce function in a loop, each 
time passing it the next unprocessed key. This continues until all keys in the 
partition are processed. 
*/
void MR_ProcessPartition(int partition_number);

/*
MR_GetNext takes a key and a partition number, and returns a value associated
with the key that exists in that partition.
*/
char *MR_GetNext(char *key, int partition_number);
#endif