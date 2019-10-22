#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "mapreduce.h"
#include "threadpool.h"

void MR_Run(int num_files, char *filenames[], 
            Mapper map, int num_mappers,
            Reducer concate, int num_reducers) {

    ThreadPool_t *tp = ThreadPool_create(num_mappers); // create a threadpool for mappers
    
    // add each file_name to work queue
    for (int i = 0; i < num_files; i++) {
        // make sure add_work is working
        assert(ThreadPool_add_work(tp, map, filenames[i]) != false);
    }

}

/*
MR_Emit takes a key and a value associated with it, and writes this pair to a 
specific partition which is determined by passing the key to MR_Partition 
*/
void MR_Emit(char *key, char *value) {
    char *key;
    
}

// source: assignment 2 instructions
unsigned long MR_Partition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0') {
        hash = hash * 33 + c;
    }
    return hash % num_partitions;
}

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
