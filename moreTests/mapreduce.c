#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "mapreduce.h"
#include "threadpool.h"

typedef struct K_V_Pair {
    char *key;
    char *value;
    struct K_V_Pair *next;
} K_V_Pair;

typedef struct {
    K_V_Pair *head;
    pthread_mutex_t mutex;
} Pair_Table;

int NUM_PARTITIONS;
Reducer REDUCER;
Pair_Table *PAIR_TABLES;

void MR_Run(int num_files, char *filenames[], 
            Mapper map, int num_mappers,
            Reducer concate, int num_reducers) {

    printf("here\n");

    NUM_PARTITIONS = num_reducers;
    REDUCER = concate;
    
    // initialize each element in PAIR_TABLES
    PAIR_TABLES = (Pair_Table *)(malloc(NUM_PARTITIONS * sizeof(Pair_Table))); //TODO: free it!
    for (int i = 0; i < NUM_PARTITIONS; i++) {
        Pair_Table new_table;
        PAIR_TABLES[i] = new_table;
        PAIR_TABLES[i].head = NULL;
        pthread_mutex_init(&(PAIR_TABLES[i].mutex), NULL);
    }
    
    // create a mapper threadpool
    ThreadPool_t *mapper_tp = ThreadPool_create(num_mappers); 

    printf("%d\n", num_files);

    // add each file_name to work queue
    for (int i = 0; i < num_files; i++) {
        // make sure add_work is working
        printf("%d*\n",i);
        assert(ThreadPool_add_work(mapper_tp, (thread_func_t)map, filenames[i]) != false);
    }

    ThreadPool_destroy(mapper_tp); // destroy mapper tp


    // create reducer threadpool
    ThreadPool_t *reducer_tp = ThreadPool_create(num_reducers);

    // add work to reducer's work queue
    for (long i = 0; i < num_reducers; i++) {
        // make sure add_work is working
        assert(ThreadPool_add_work(reducer_tp, (thread_func_t)MR_ProcessPartition, (void *)i) != false);
    }

    ThreadPool_destroy(reducer_tp); // destroy reducer tp

}

/*
MR_Emit takes a key and a value associated with it, and writes this pair to a 
specific partition which is determined by passing the key to MR_Partition 
*/
void MR_Emit(char *key, char *value) {
    K_V_Pair *new_pair = (K_V_Pair *)(malloc(sizeof(K_V_Pair) * 1));
    new_pair -> key = key;
    new_pair -> value = value;
    new_pair -> next = NULL;

    int partition_num = MR_Partition(key, NUM_PARTITIONS);
    printf("partition_num: %d\n", partition_num);
    Pair_Table pair_table = PAIR_TABLES[partition_num];

    pthread_mutex_lock(&(pair_table.mutex));

    // insert new_pair to matching partition
    K_V_Pair *cur = pair_table.head;

    // insertion sort
    if (cur == NULL) {
        cur = new_pair;
    } else if (strcmp(cur -> key, new_pair -> key) <= 0) {
        new_pair -> next = cur;
        cur = new_pair;
    } else {
        while ((cur -> next != NULL) && 
                (strcmp(new_pair -> key, cur -> next -> key) > 0)) {
            cur = cur -> next;
        }
        new_pair -> next = cur -> next;
        cur -> next = new_pair;
    }

    free(new_pair);
    
    pthread_mutex_unlock(&(pair_table.mutex));

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
void MR_ProcessPartition(int partition_number) {
    // find corresponding partition
    K_V_Pair *cur = PAIR_TABLES[partition_number].head;
    if (cur == NULL) {
        printf("\ncur is NULL!\n");
    } else {
        while (cur -> next != NULL) {
            REDUCER(cur -> key, partition_number);
            cur = cur -> next;
        }
    }
}

/*
MR_GetNext takes a key and a partition number, and returns a value associated
with the key that exists in that partition.
TODO: delete cur_pair after getting value?
*/
char *MR_GetNext(char *key, int partition_number) {
    // find corresponding partition
    K_V_Pair *cur = PAIR_TABLES[partition_number].head;
    while (cur -> next != NULL) {
        // find matching k and return its value
        if (strcmp(cur -> key, key) == 0) {
            return cur -> value;
        }
    }
    return NULL;
}
