#include "threadpool.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

int thread_count;

ThreadPool_t *ThreadPool_create(int num) {

    // make new threadpool (tp)
    ThreadPool_t *tp;
    tp = (ThreadPool_t *) malloc(sizeof(ThreadPool_t));
    assert(tp != NULL);

    // init threadpool variables
    tp -> max_thread_num = num;
    tp -> shutdown = 0;
    tp -> threads = (pthread_t *) malloc(sizeof(pthread_t) * num);

    // init work queue
    tp -> work_queue = (ThreadPool_work_queue_t *) \
                malloc(sizeof(ThreadPool_work_queue_t) * 1);
    tp -> work_queue -> cur_size = 0;
    tp -> work_queue -> head = NULL;

    // init synchronization primitives
    pthread_mutex_init(&(tp -> mutex), NULL);
    pthread_cond_init(&(tp -> cond), NULL);
    thread_count = 0;
    // create pthreads
    for (int i = 0; i < num; i++) {
        pthread_create(&(tp -> threads[i]), NULL, (void *)Thread_run, tp);
    }
    while (thread_count != num) {
        // spin while not all threads are created
    }
    return tp;
}

void ThreadPool_destroy(ThreadPool_t *tp) {
    assert(tp != NULL);

    assert(tp -> shutdown != 1); // avoid shutting down multiple times
    tp -> shutdown = 1; // make sure shutdown is positive

    pthread_cond_broadcast(&(tp -> cond)); // wake waiting threads

    // destroy threads
    for (int i = 0; i < tp -> max_thread_num; i++) {
        pthread_join(tp -> threads[i], NULL);
    }
    free(tp -> threads);

    // destroy work queue
    ThreadPool_work_t *temp_work, *cur_work;
    cur_work = tp -> work_queue -> head;
    while (cur_work != NULL) {
        temp_work = cur_work;
        cur_work = cur_work -> next;
        free(temp_work);
    }

    // destroy synchronization primitives
    pthread_mutex_destroy(&(tp -> mutex));
	pthread_cond_destroy(&(tp -> cond));
 
    free(tp -> work_queue);
	free(tp);
	// point tp to NULL after freeing
	tp = NULL;
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg, int isMapperWork) {

    pthread_mutex_lock(&(tp -> mutex));

    bool is_added = false;

    // create a new work
    ThreadPool_work_t *new_work = (ThreadPool_work_t *) malloc(sizeof(ThreadPool_work_t));
    new_work -> func = func;
    new_work -> arg = arg;
    new_work -> next = NULL;

    struct stat buffer;
    
    if (isMapperWork == 1) { // when arg is a filename (working for mappers)
        if (stat(arg, &buffer) == 0) { 
            new_work -> size = buffer.st_size;
        }
    } else { // when arg is not a filename (working for reducers)
        new_work -> size = 0;
    }

    // put new work to queue
    if ((tp -> work_queue -> head == NULL) || 
        (new_work -> size > tp -> work_queue -> head -> size)) {
        new_work -> next = tp -> work_queue -> head;; // put the new work to cur's left side
        tp -> work_queue -> head = new_work;
    } else { // if queue not empty and new_work size larger than cur/head
        ThreadPool_work_t *cur = tp -> work_queue -> head;
        while ((cur -> next != NULL) &&
                (new_work -> size <= cur -> next -> size)) {
            cur = cur -> next;
        }
        new_work -> next = cur -> next;
        cur -> next = new_work;
    }
    is_added = true;
    tp -> work_queue -> cur_size++; // update size of queue

    pthread_mutex_unlock(&(tp -> mutex));
    pthread_cond_signal(&(tp -> cond)); // wake a waiting thread to do work

    return is_added;
}

ThreadPool_work_t *ThreadPool_get_work(ThreadPool_t *tp) {
    return tp -> work_queue -> head; // return current work
}

void *Thread_run(ThreadPool_t *tp) {
    pthread_mutex_lock(&(tp -> mutex));
    thread_count++;
    pthread_mutex_unlock(&(tp -> mutex));

    ThreadPool_work_t *cur_work;
    while (1) {
        pthread_mutex_lock(&(tp -> mutex));
        // if none work left
        if (tp -> work_queue -> head == NULL) { 
            if (tp -> shutdown == 0) { // if pool is not shutdown
                pthread_cond_wait(&(tp -> cond), &(tp -> mutex));
                pthread_mutex_unlock(&(tp -> mutex));
            } else { // if shutdown pool
                pthread_mutex_unlock(&(tp -> mutex));
                pthread_exit(NULL);
            } 
        } else {
            // when work queue is not empty
            assert(tp -> work_queue -> cur_size != 0); // make sure queue is not empty

            cur_work = ThreadPool_get_work(tp);
            tp -> work_queue -> cur_size--; // decrease queue size after get work
            tp -> work_queue -> head = tp -> work_queue -> head -> next; // move head to next
            pthread_mutex_unlock(&(tp -> mutex));
            (cur_work -> func)(cur_work -> arg); // run the cur work's func
            free(cur_work);
            cur_work = NULL;
        }     
    }
}
