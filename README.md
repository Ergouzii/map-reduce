# map-reducer

# Program analysis

## Intermediate data structure in mapreduce

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

## Time complexity

For my `MR_Emit`, the time complexity is `O(n)`. 

In worst case, it iterates throught the whole length of linked list, `O(n)`. 

In best case, it finds the head is `NULL` and end, `O(1)`. 

---

For `MR_GetNext`, the time complexity is `O(1)`. Once it gets input key, it simply get the next element of the input key and return it.

## Work queue in threadpool

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

Inside `ThreadPool_t`, I have a `mutex` and a `cond`, they are used as synchronization primitives. `max_thread_num` keeps a record of how many threads are in the threadpool, so when the threadpool is destroy, I can know how many threads need to be waited/`join()`. `shutdown` is just a 



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


