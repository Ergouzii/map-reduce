# map-reducer introduction

# Note to marker:

Hi! Before you run my program, here is something I want to tell you:

When you run valgrind to check memory leaks, you might see `still reachable: 1,614 bytes in 4 blocks` in `LEAK SUMMARY`. 

I had been looking for the leak sources and here is what I got: 

```
==3256== 36 bytes in 1 blocks are still reachable in loss record 1 of 4
==3256==    at 0x4C2DB8F: malloc (vg_replace_malloc.c:299)
==3256==    by 0x401CED9: strdup (strdup.c:42)
==3256==    by 0x40185EE: _dl_load_cache_lookup (dl-cache.c:311)
==3256==    by 0x4009168: _dl_map_object (dl-load.c:2364)
==3256==    by 0x4015586: dl_open_worker (dl-open.c:237)
==3256==    by 0x4010573: _dl_catch_error (dl-error.c:187)
==3256==    by 0x4014DB8: _dl_open (dl-open.c:660)
==3256==    by 0x519A5AC: do_dlopen (dl-libc.c:87)
==3256==    by 0x4010573: _dl_catch_error (dl-error.c:187)
==3256==    by 0x519A663: dlerror_run (dl-libc.c:46)
==3256==    by 0x519A663: __libc_dlopen_mode (dl-libc.c:163)
==3256==    by 0x4E4B91A: pthread_cancel_init (unwind-forcedunwind.c:52)
==3256==    by 0x4E4BB03: _Unwind_ForcedUnwind (unwind-forcedunwind.c:126)
==3256== 
==3256== 36 bytes in 1 blocks are still reachable in loss record 2 of 4
==3256==    at 0x4C2DB8F: malloc (vg_replace_malloc.c:299)
==3256==    by 0x400BEF3: _dl_new_object (dl-object.c:165)
==3256==    by 0x400650C: _dl_map_object_from_fd (dl-load.c:1028)
==3256==    by 0x4008C26: _dl_map_object (dl-load.c:2498)
==3256==    by 0x4015586: dl_open_worker (dl-open.c:237)
==3256==    by 0x4010573: _dl_catch_error (dl-error.c:187)
==3256==    by 0x4014DB8: _dl_open (dl-open.c:660)
==3256==    by 0x519A5AC: do_dlopen (dl-libc.c:87)
==3256==    by 0x4010573: _dl_catch_error (dl-error.c:187)
==3256==    by 0x519A663: dlerror_run (dl-libc.c:46)
==3256==    by 0x519A663: __libc_dlopen_mode (dl-libc.c:163)
==3256==    by 0x4E4B91A: pthread_cancel_init (unwind-forcedunwind.c:52)
==3256==    by 0x4E4BB03: _Unwind_ForcedUnwind (unwind-forcedunwind.c:126)

...There are two more almost the same messages
```

After consulting with one of the TAs, I was told that this should a problem of `pthread` library. I am not super sure about this, but if you don't think it is my program's problem either, I will be happy if you give me these marks :)

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

Inside `ThreadPool_t`, `max_thread_num` keeps a record of how many threads are in the threadpool, so when the threadpool is destroy, I can know how many threads need to be waited/`join()`. `shutdown` is a flag to let make sure the threadpool is ONLY destroyed when `ThreadPool_destroy` is called, where `shutdown` is set to 1.

I have a `mutex` and a `cond` used as synchronization primitives. I use `mutex` whenever the shared data structures (e.g., work queue) is changed by whichever thread, so the data is not messed up by them. 'cond` is used to let `Thread_run` know when to wait for `ThreadPool_add_work` is done. I tested them right after finishing `threadpool.c` by writing a `main` and a `work_func`. `main` initializes the threadpool and `work_func` is the work to be added (it just `sleep` for 2 seconds). By printing out the threads' status in the functions, I trusted my threadpool is working fine.

I modified `ThreadPool_add_work` to take one more argument `isMapperWork`. The reason is that I am using the threadpool for both mapper and reducers, so `ThreadPool_add_work` needs a way to know whether the work to be added is a mapper or reducer; otherwise calling `stat()` on non-string `arg`s will cause errors.


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


