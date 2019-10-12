#include "threadpool.h"

ThreadPool_t *ThreadPool_create(int num) {

    // make new thread pool
    ThreadPool_t *pool;
    pool = (struct ThreadPool_t*) malloc(sizeof(ThreadPool_t));
    if (pool == NULL) {
        perror("ThreadPool_create failed, no memory for the pool.\n");
        return NULL;
    }

    pthread_mutex_init(&(pool -> pool_mutex), NULL);
    pthread_cond_init(&(pool -> pool_cond), NULL);

    // init work queue
    ThreadPool_work_queue_t *work_queue;
    work_queue->cur_size = 0;
    work_queue->front = NULL;
    work_queue->rear = NULL;
    

}

void ThreadPool_destroy(ThreadPool_t *tp) {

}
