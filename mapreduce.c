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
    K_V_Pair *cur;
    pthread_mutex_t mutex;
} Pair_Table;

int NUM_PARTITIONS;
Reducer REDUCER;
Pair_Table *PAIR_TABLES;

void MR_Run(int num_files, char *filenames[], 
            Mapper map, int num_mappers,
            Reducer concate, int num_reducers) {

    NUM_PARTITIONS = num_reducers;
    REDUCER = concate;
    
    // initialize each element in PAIR_TABLES
    PAIR_TABLES = (Pair_Table *)(malloc(NUM_PARTITIONS * sizeof(Pair_Table)));
    for (int i = 0; i < NUM_PARTITIONS; i++) {
        PAIR_TABLES[i].head = NULL;
        PAIR_TABLES[i].cur = NULL;
        pthread_mutex_init(&(PAIR_TABLES[i].mutex), NULL);
    } 

    // create a mapper threadpool
    ThreadPool_t *mapper_tp = ThreadPool_create(num_mappers); 

    // add each file_name to work queue
    for (int i = 0; i < num_files; i++) {
        // make sure add_work is working
        assert(ThreadPool_add_work(mapper_tp, (thread_func_t)map, filenames[i], 1) != false);
    }

    ThreadPool_destroy(mapper_tp); // destroy mapper tp

    // create reducer threadpool
    ThreadPool_t *reducer_tp = ThreadPool_create(num_reducers);

    // add work to reducer's work queue
    for (long i = 0; i < num_reducers; i++) {
        // make sure add_work is working
        assert(ThreadPool_add_work(reducer_tp, (thread_func_t)MR_ProcessPartition, (void *)i, 0) != false);
    }

    ThreadPool_destroy(reducer_tp); // destroy reducer tp

    for (int i = 0; i < NUM_PARTITIONS; i++) {
        while (PAIR_TABLES[i].head != NULL) {
            free(PAIR_TABLES[i].head -> key);
            free(PAIR_TABLES[i].head -> value);
            K_V_Pair *temp = PAIR_TABLES[i].head -> next;
            free(PAIR_TABLES[i].head);
            PAIR_TABLES[i].head = temp;
        }
    }
    free(PAIR_TABLES);
}

/*
MR_Emit takes a key and a value associated with it, and writes this pair to a 
specific partition which is determined by passing the key to MR_Partition 
*/
void MR_Emit(char *key, char *value) {

    // init new_pair
    K_V_Pair *new_pair = (K_V_Pair *)(malloc(sizeof(K_V_Pair)));
    new_pair -> key = (char *)(malloc(strlen(key) + 1));
    strcpy(new_pair -> key, key);
    new_pair -> value = (char *)(malloc(strlen(value) + 1));
    strcpy(new_pair -> value, value);
    new_pair -> next = NULL;

    int partition_num = MR_Partition(key, NUM_PARTITIONS);

    pthread_mutex_lock(&(PAIR_TABLES[partition_num].mutex));

    // insertion sort
    if ((PAIR_TABLES[partition_num].head == NULL) || 
        (strcmp(PAIR_TABLES[partition_num].head -> key, new_pair -> key) > 0)) {
        new_pair -> next = PAIR_TABLES[partition_num].head;
        PAIR_TABLES[partition_num].head = new_pair;
        PAIR_TABLES[partition_num].cur = PAIR_TABLES[partition_num].head;
    } else {
        K_V_Pair *cur = PAIR_TABLES[partition_num].head;
        while ((cur -> next != NULL) && 
                (strcmp(cur -> next -> key, new_pair -> key) <= 0)) {
            cur = cur -> next;
        }
        new_pair -> next = cur -> next;
        cur -> next = new_pair;
    }

    pthread_mutex_unlock(&(PAIR_TABLES[partition_num].mutex));
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

void MR_ProcessPartition(int partition_number) {

    if (PAIR_TABLES[partition_number].cur == NULL) {
        ;
    } else {
        while (PAIR_TABLES[partition_number].cur != NULL && PAIR_TABLES[partition_number].cur -> next != NULL) {
            REDUCER(PAIR_TABLES[partition_number].cur -> key, partition_number);
        }
    }
}

char *MR_GetNext(char *key, int partition_number) {
    
    if (PAIR_TABLES[partition_number].cur == NULL) {
        ;
    } else if (strcmp(PAIR_TABLES[partition_number].cur -> key, key) == 0) { // if cur matches given key
        char *temp = PAIR_TABLES[partition_number].cur -> value; // copy value before going to next pair
        PAIR_TABLES[partition_number].cur = PAIR_TABLES[partition_number].cur -> next;
        return temp;
    } else if (strcmp(PAIR_TABLES[partition_number].cur -> key, key) != 0) {
        ;
    }

    return NULL;
}
