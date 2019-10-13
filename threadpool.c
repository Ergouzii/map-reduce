#include "threadpool.h"
#include <stdbool.h>

ThreadPool_t *ThreadPool_create(int num) {

    // make new threadpool (tp)
    ThreadPool_t *tp;
    tp = (ThreadPool_t *) malloc(sizeof(ThreadPool_t));
    if (tp == NULL) {
        perror("ThreadPool_create failed, no memory for the pool.\n");
        return NULL;
    }

    // init tp struct variables
    tp -> max_thread_num = num;
    tp -> shutdown = 0;
    tp -> threads = (pthread_t *) malloc(sizeof(pthread_t) * num);

    // init work queue
    tp -> work_queue = (ThreadPool_work_queue_t *) \
                malloc(sizeof(ThreadPool_work_queue_t) * num); // TODO: what is queue's max size?
    tp -> work_queue -> cur_size = 0;
    tp -> work_queue -> front = NULL;
    tp -> work_queue -> rear = NULL;

    // init synchronization primitives
    pthread_mutex_init(&(tp -> tp_mutex), NULL);
    pthread_cond_init(&(tp -> tp_cond), NULL);
    pthread_mutex_init(&(tp -> work_queue -> queue_mutex), NULL);

    // create pthreads
    for (int i = 0; i < num; i++) {
        pthread_create(&(tp -> threads[i]), NULL, Thread_run, NULL);
    }
}

void ThreadPool_destroy(ThreadPool_t *tp) {
    if (tp == NULL) return -1;

    if (tp -> shutdown == 1) return -1; // avoid shutting down multiple times
    tp -> shutdown = 1; // make sure shutdown is positive

    pthread_cond_broadcast(&(tp -> tp_cond)); // wake waiting threads

    // destroy threads
    for (int i = 0; i < tp -> max_thread_num; i++) {
        pthread_join(tp -> threads[i], NULL);
    }
    free(tp -> threads);

    // destroy work queue
    ThreadPool_work_t *temp_work, *cur_work;
    cur_work = tp -> work_queue -> front;
    while (cur_work != NULL) {
        temp_work = cur_work;
        cur_work = cur_work -> next; // TODO: is front.next going towards rear?
        free(temp_work);
    }

    // destroy synchronization primitives
    pthread_mutex_destroy(&(tp -> tp_mutex));
	pthread_cond_destroy(&(tp -> tp_cond));
 
	free(tp);
	// point tp to NULL after free
	tp = NULL;
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg) {
    // create a new work
    ThreadPool_work_t *new_work = (ThreadPool_work_t *) malloc(sizeof(ThreadPool_work_t));
    new_work -> func = func;
    new_work -> arg = arg;
    new_work -> next = NULL;

    // put new work to queue
    // TODO: add new work in LJF order
    pthread_mutex_lock(&(tp -> work_queue -> queue_mutex));
    ThreadPool_work_t *head = tp -> work_queue -> front;
    if (head == NULL) {
        tp -> work_queue -> front = new_work;
    } else {
        while (head -> next != NULL) {
            head = head -> next;
        }
        head -> next = new_work;
    }
    pthread_mutex_unlock(&(tp -> work_queue -> queue_mutex));
    // pthread_cond_signal(&(tp -> work_queue -> cond));
}

ThreadPool_work_t *ThreadPool_get_work(ThreadPool_t *tp) {

}

void *Thread_run(ThreadPool_t *tp) {

}
