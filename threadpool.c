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
    tp -> work_queue -> tail = NULL;

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
 
	free(tp);
	// point tp to NULL after freeing
	tp = NULL;
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg) {
    bool is_added = false;

    // create a new work
    ThreadPool_work_t *new_work = (ThreadPool_work_t *) malloc(sizeof(ThreadPool_work_t));
    new_work -> func = func;
    new_work -> arg = arg;
    new_work -> next = NULL;
    // TODO: init work size here using stat(2)

    // put new work to queue
    // TODO: add new work in LJF order
    pthread_mutex_lock(&(tp -> mutex));
    if (tp -> work_queue -> head == NULL) { // if queue is empty
        tp -> work_queue -> head = new_work;
    } else { // if queue not empty
        ThreadPool_work_t *cur = tp -> work_queue -> head;
        while (cur -> next != NULL) {
            cur = cur -> next;
        }
        cur -> next = new_work; // add new_work to head
    }
    // TODO: LJF is here, am I doing opposite direction? head is largest: 3->2->1
    // struct stat buffer;
    // assert(stat(arg, &buffer) == 0); // make sure stat(2) is successfully runned
    // new_work -> size = buffer.st_size;

    // // put new work to queue
    // pthread_mutex_lock(&(tp -> mutex));
    // ThreadPool_work_t *cur = tp -> work_queue -> head;

    // if (cur == NULL) { // if queue is empty
    //     cur = new_work;
    // } else if (new_work -> size <= cur -> size) { // if new_work has no larger size than cur/head
    //     new_work -> next = cur; // put the new work to cur's left side
    //     cur = new_work;
    // } else { // if queue not empty and new_work size larger than cur/head
    //     while ((cur -> next != NULL) &&
    //             (new_work -> size > cur -> next -> size)) {
    //         cur = cur -> next;
    //     }
    //     new_work -> next = cur -> next;
    //     cur -> next = new_work; // add new_work to head
    // }
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

    printf("Thread_run: Thread %lu is ready\n", pthread_self());
    ThreadPool_work_t *cur_work;
    while (1) {
        pthread_mutex_lock(&(tp -> mutex));
        // if none work left
        if (tp -> work_queue -> head == NULL) { 
            if (tp -> shutdown == 0) { // if pool is not shutdown
                printf("Thread_run: Thread %lu is waiting\n", pthread_self());
                pthread_cond_wait(&(tp -> cond), &(tp -> mutex));
                pthread_mutex_unlock(&(tp -> mutex));
            } else { // if shutdown pool
                pthread_mutex_unlock(&(tp -> mutex));
                printf("Thread_run: Thread %lu is done\n", pthread_self());
                pthread_exit(NULL);
            } 
        } else {
            // when work queue is not empty
            printf("Thread_run: Thread %lu is going to work\n", pthread_self());

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

// *********************tester function*************************
// void my_func(void *arg) {
//     printf("my_func: Thread %lu working on %d*\n", pthread_self(), *(int *) arg);
//     sleep(2);
//     return;
// }

// int main() {
//     ThreadPool_t *tp = ThreadPool_create(3);

//     int *num_work = (int *) malloc(sizeof(int) * 10);
//     for (int i = 0; i < 10; i++) {
//         num_work[i] = i;
//         ThreadPool_add_work(tp, my_func, &num_work[i]);
//     }

//     ThreadPool_destroy(tp);

//     free(num_work);

//     return 0;
// }
