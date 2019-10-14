#include "threadpool.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

ThreadPool_t *ThreadPool_create(int num) {

    // make new threadpool (tp)
    ThreadPool_t *tp;
    tp = (ThreadPool_t *) malloc(sizeof(ThreadPool_t));
    if (tp == NULL) {
        perror("ThreadPool_create failed, no memory for the pool.\n");
        return NULL;
    }

    // init threadpool variables
    tp -> max_thread_num = num;
    tp -> shutdown = 0;
    tp -> threads = (pthread_t *) malloc(sizeof(pthread_t) * num);

    // init work queue
    tp -> work_queue = (ThreadPool_work_queue_t *) \
                malloc(sizeof(ThreadPool_work_queue_t) * num); // TODO: what is queue's max size?
    tp -> work_queue -> cur_size = 0;
    tp -> work_queue -> head = NULL;
    tp -> work_queue -> tail = NULL;

    // init synchronization primitives
    pthread_mutex_init(&(tp -> tp_mutex), NULL);
    pthread_cond_init(&(tp -> tp_cond), NULL);
    pthread_mutex_init(&(tp -> work_queue -> queue_mutex), NULL);
    pthread_cond_init(&(tp -> work_queue -> queue_cond), NULL);

    // create pthreads
    for (int i = 0; i < num; i++) {
        pthread_create(&(tp -> threads[i]), NULL, Thread_run, NULL);
    }
    return NULL;
}

void ThreadPool_destroy(ThreadPool_t *tp) {
    if (tp == NULL) return;

    if (tp -> shutdown == 1) return; // avoid shutting down multiple times
    tp -> shutdown = 1; // make sure shutdown is positive

    pthread_cond_broadcast(&(tp -> tp_cond)); // wake waiting threads

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
        cur_work = cur_work -> next; // TODO: is head.next going towards tail?
        free(temp_work);
    }

    // destroy synchronization primitives
    pthread_mutex_destroy(&(tp -> tp_mutex));
	pthread_cond_destroy(&(tp -> tp_cond));
    pthread_mutex_destroy(&(tp -> work_queue -> queue_mutex));
    pthread_cond_destroy(&(tp -> work_queue -> queue_cond));
 
	free(tp);
	// point tp to NULL after freeing
	tp = NULL;
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg) {
    // create a new work
    ThreadPool_work_t *new_work = (ThreadPool_work_t *) malloc(sizeof(ThreadPool_work_t));
    new_work -> func = func;
    new_work -> arg = arg;
    new_work -> next = NULL;
    // TODO: init work size here using stat(2)

    // put new work to queue
    // TODO: add new work in LJF order
    pthread_mutex_lock(&(tp -> work_queue -> queue_mutex));
    ThreadPool_work_t *head = tp -> work_queue -> head;
    if (head == NULL) { // if queue is empty
        head = new_work;
    } else { // if queue not empty
        while (head -> next != NULL) {
            head = head -> next;
        }
        head -> next = new_work; // add new_work to head
    }
    tp -> work_queue -> cur_size++; // update size of queue

    pthread_mutex_unlock(&(tp -> work_queue -> queue_mutex));
    pthread_cond_signal(&(tp -> work_queue -> queue_cond)); // wake a waiting thread to do work

    // TODO: return a bool indicating add_work succeed or fail
    return NULL;
}

ThreadPool_work_t *ThreadPool_get_work(ThreadPool_t *tp) {
    return NULL;
}

void *Thread_run(ThreadPool_t *tp) {
    printf("Thread %p is ready\n", pthread_self());
    ThreadPool_work_t *cur_work;
    while (1) {
        pthread_mutex_lock(&(tp -> work_queue -> queue_mutex)); // lock the queue

        // if none work left
        if (tp -> work_queue -> head == NULL) { 
            if (tp -> shutdown == 0) { // if shutdown pool
                printf("Thread %p is waiting\n", pthread_self());
                pthread_cond_wait(&(tp -> work_queue -> queue_cond), &(tp -> work_queue -> queue_mutex));
            } else { // if not shutdown pool
                pthread_mutex_unlock(&(tp -> work_queue -> queue_mutex));
                printf("Thread %p is done\n", pthread_self());
                pthread_exit(NULL);
            }   
        }

        // when work queue is not empty
        printf("Thread %p is going to work\n", pthread_self());
        if (tp -> work_queue -> head == NULL) { // make sure head is not NULL
            perror("work queue head is NULL\n");
        } 

        cur_work = tp -> work_queue -> head;
        tp -> work_queue -> head = tp -> work_queue -> head -> next;
        pthread_mutex_unlock(&(tp -> work_queue -> queue_mutex));
        (cur_work -> func)(cur_work -> arg); // run the cur work's func
        free(cur_work);
        cur_work = NULL;
    }
}

// **************************************************************
void my_func(void *arg) {
    printf("*Thread %p working on %d*\n", pthread_self(), *(int *) arg);
    sleep(1);
    return;
}

int main() {
    ThreadPool_t *tp = ThreadPool_create(3);

    int *num_work = (int *) malloc(sizeof(int) * 10);
    for (int i = 0; i < 10; i++) {
        num_work[i] = i;
        ThreadPool_add_work(tp, my_func, &num_work[i]);
    }

    sleep(5);

    ThreadPool_destroy(tp);

    free(num_work);

    return 0;
}