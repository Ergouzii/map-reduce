# Map-Reduce introduction

MapReduce is a programming model and a distributed computing paradigm for large-scale data processing. It allows for applications to run various tasks in parallel, making them scalable and fault-tolerant. To use the MapReduce infrastructure, developers need to write just a little bit of code in addition to their program. They do not need to worry about how to parallelize their program; the MapReduce runtime will make this happen!

# Program analysis

## map-reduce

### Intermediate data structure

To store the intermediate key/value pairs, I have designed these two structs in `mapreduce.c`:

```
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
```

`K_V_Pair` is each key & value pair, and the pairs are in a singly linked list data structure.

`Pair_Table` is the container (it's a singly linked list) for `K_V_pair`. The reason to make it a singly linked list is that it can be extented infinitely, unlike array which has fixed size. It also has a `mutex` in it as a **synchronization primitive**.

### Time complexity

For my `MR_Emit`, the time complexity is `O(n)`. 

In worst case, it iterates throught the whole length of linked list, `O(n)`. 

In best case, it finds the head is `NULL` and end, `O(1)`. 

---

For `MR_GetNext`, the time complexity is `O(1)`. Once it gets input key, it simply get the next element of the input key and return it.

### global variables

`int NUM_PARTITIONS;`: same value as `num_reducers`, used to create reducers' threadpool, etc.

`Reducer REDUCER;`: since reducer is only passed in to `MR_Run`, I created this global var to let other functions get access to the reducer function.

`Pair_Table *PAIR_TABLES;`: An array that has a size of `NUM_PARTITIONS', it contains all the `Pair_Table` structures.

## Threadpool

The work queue data structure is defined in `threadpool.h` like this:

```
typedef struct ThreadPool_work_t {
    thread_func_t func;              // The function pointer
    void *arg;                       // The arguments for the function
    struct ThreadPool_work_t *next; // each work should know which work comes next
    off_t size;                     // file size of the work
} ThreadPool_work_t;

typedef struct {
    ThreadPool_work_t *head;
    int cur_size;                   // current num of works in queue
} ThreadPool_work_queue_t;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int max_thread_num;
    int shutdown;                   // whether treadpool is shutdown, 1 = yes, 0 = no
    pthread_t *threads;             // all threads
    ThreadPool_work_queue_t *work_queue; // working threads in queue
} ThreadPool_t;
```

The most basic unit is `ThreadPool_work_t`, which indicates each job in threadpool. It is a node in the singly linked list `ThreadPool_work_queue_t`. 

`ThreadPool_work_queue_t` can extend infinitely since it is a linked list, so we don't have to worry that too much work will fill up the queue.

Inside `ThreadPool_t`, `max_thread_num` keeps a record of how many threads are in the threadpool, so when the threadpool is destroy, I can know how many threads need to be waited/`join()`. `shutdown` is a flag to let make sure the threadpool is ONLY destroyed when `ThreadPool_destroy` is called, where `shutdown` is set to 1.

I have a `mutex` and a `cond` used as synchronization primitives. I use `mutex` whenever the shared data structures (e.g., work queue) is changed by whichever thread, so the data is not messed up by them. 'cond` is used to let `Thread_run` know when to wait for `ThreadPool_add_work` is done. I tested them right after finishing `threadpool.c` by writing a `main` and a `work_func`. `main` initializes the threadpool and `work_func` is the work to be added (it just `sleep` for 2 seconds). By printing out the threads' status in the functions, I trusted my threadpool is working fine.

### modification of `ThreadPool_add_work`

I modified `ThreadPool_add_work` to take one more argument `int isMapperWork`. The reason is that I am using the threadpool for both mapper and reducers, so `ThreadPool_add_work` needs a way to know whether the work to be added is a mapper or reducer; otherwise calling `stat()` on non-string `arg`s will cause errors.

### global variable

As for global variables in `threadpool.c`, I have only one called `thread_count`. It keeps track of how many threads have been created. It is used to avoid unexpected behaviors when not all threads are created.


# References:

**Thread pool:** 

https://blog.csdn.net/woxiaohahaa/article/details/51510747

https://github.com/Pithikos/C-Thread-Pool/blob/master/thpool.c

https://github.com/mbrossard/threadpool/blob/master/src/threadpool.c

**stat(2):**

https://docs.oracle.com/cd/E36784_01/html/E36872/stat-2.html

**hash function:**

assignment 2 instructions

**strcmp:**

https://www.tutorialspoint.com/c_standard_library/c_function_strcmp.htm

**sorted linked list:**

https://www.geeksforgeeks.org/given-a-linked-list-which-is-sorted-how-will-you-insert-in-sorted-way/


