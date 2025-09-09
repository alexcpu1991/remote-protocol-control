/**
 * @file    rpc_osal_linux.c
 * @brief   Linux implementation of RPC OS Abstraction Layer.
 *
 * This module provides Linux-specific implementation of OSAL using pthreads.
 */

#include "rpc_osal.h"
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* --- Helpers --- */

/**
 * @brief Convert relative timeout to absolute timespec.
 *
 * @param ts Output timespec structure.
 * @param timeout_ms Timeout in milliseconds.
 */
static void abs_time_ms(struct timespec* ts, uint32_t timeout_ms) {
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec  += timeout_ms / 1000;
    ts->tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec += 1;
        ts->tv_nsec -= 1000000000L;
    }
}


/* --- Thread --- */

/** Thread structure for Linux implementation */
struct os_thread {
    pthread_t tid;
};


/**
 * @brief Create a new thread (Linux implementation).
 */
os_thread_t os_thread_create(const char* name, // thread name (ignored)
                             os_thread_fn fn, // thread function
                             void* arg, // function argument
                             uint16_t stack_size, // stack size
                             uint8_t priority) // priority (ignored)
{
    (void)name;
    (void)priority;

    os_thread_t t = calloc(1, sizeof(*t));
    if (!t) return NULL;

    pthread_attr_t attr;
    pthread_attr_init(&attr); // priority (ignored)
    if (stack_size > 0)
        pthread_attr_setstacksize(&attr, stack_size); // Set stack size

    if (pthread_create(&t->tid, &attr, fn, arg) != 0) { // Basic thread creation
        free(t);
        return NULL;
    }

    pthread_attr_destroy(&attr); // Freeing attribute resources
    return t;
}


/* ---------- Queues (ring buffer) ---------- */

/** Queue structure for Linux implementation */
struct os_queue {
    uint8_t* buf;             /**< Data buffer */
    size_t item_size;         /**< Size of one item */
    size_t capacity;          /**< Maximum capacity */
    size_t head;              /**< Read index */
    size_t tail;              /**< Write index */
    size_t count;             /**< Current item count */
    pthread_mutex_t m;        /**< Mutex for synchronization */
    pthread_cond_t not_empty; /**< Condition variable: not empty */
    pthread_cond_t not_full;  /**< Condition variable: not full */
};


/**
 * @brief Create a new queue (Linux implementation).
 */
os_queue_t os_queue_create(size_t length, size_t item_size) {
    struct os_queue* q = calloc(1, sizeof(*q));
    if (!q) return NULL;

    q->buf = malloc(length * item_size);
    if (!q->buf) { free(q); return NULL; }

    q->item_size = item_size; q->capacity = length;
    pthread_mutex_init(&q->m, NULL);
    pthread_cond_init (&q->not_empty, NULL);
    pthread_cond_init (&q->not_full, NULL);
    return q;
}


/**
 * @brief Send an item to the queue (Linux implementation).
 */
bool os_queue_send(os_queue_t q, const void* item, uint32_t timeout_ms) {
    if (!q || !item) return false;

    pthread_mutex_lock(&q->m); // Capture the mutex

    // We wait until a place becomes available
    while (q->count == q->capacity) {
        if (timeout_ms == 0) { pthread_mutex_unlock(&q->m); return false; } // We are not waiting
        struct timespec ts;
        abs_time_ms(&ts, timeout_ms);
        int r = pthread_cond_timedwait(&q->not_full, &q->m, &ts);
        if (r == ETIMEDOUT) { pthread_mutex_unlock(&q->m); return false; }
    }

    // Copy data to buffer
    memcpy(q->buf + q->tail * q->item_size, item, q->item_size);
    q->tail = (q->tail + 1) % q->capacity; // Ring buffer
    q->count++;

    pthread_cond_signal(&q->not_empty); // Signal: "data appeared!"
    pthread_mutex_unlock(&q->m); // Release the mutex
    return true;
}


/**
 * @brief Receive an item from the queue (Linux implementation).
 */
bool os_queue_recv(os_queue_t q, void* item, uint32_t timeout_ms) {
    if (!q || !item) return false;

    pthread_mutex_lock(&q->m); // Release the mutex

    // Wait for the data to appear
    while (q->count == 0) {
        if (timeout_ms == 0) { pthread_mutex_unlock(&q->m); return false; }
        struct timespec ts;
        abs_time_ms(&ts, timeout_ms);
        int r = pthread_cond_timedwait(&q->not_empty, &q->m, &ts);
        if (r == ETIMEDOUT) { pthread_mutex_unlock(&q->m); return false; }
    }

    // Copy data from the buffer
    memcpy(item, q->buf + q->head * q->item_size, q->item_size);
    q->head = (q->head + 1) % q->capacity; q->count--;

    pthread_cond_signal(&q->not_full); // Signal: "a place has appeared!"
    pthread_mutex_unlock(&q->m); // Release the mutex
    return true;
}


/* ---------- Binary Semaphores ---------- */

/** Binary semaphore structure for Linux implementation */
struct os_sem {
    pthread_mutex_t m;  /**< Mutex for synchronization */
    pthread_cond_t  c;  /**< Condition variable for waiting */
    int count;          /**< Semaphore state: 0 - busy, 1 - free */
};


/**
 * @brief Create a binary semaphore (Linux implementation).
 */
os_sem_t os_sem_create_binary(void) {
    struct os_sem* s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    pthread_mutex_init(&s->m, NULL);
    pthread_cond_init (&s->c, NULL);
    s->count = 0;
    return s;
}


/**
 * @brief Take a binary semaphore (Linux implementation).
 */
bool os_sem_take(os_sem_t s, uint32_t timeout_ms) {
    pthread_mutex_lock(&s->m); // Capture the mutex

    // Wait until the semaphore is free
    while (s->count == 0) { // While busy (count == 0)
        if (timeout_ms == 0) { pthread_mutex_unlock(&s->m); return false; } // Non-blocking mode
        struct timespec ts;
        abs_time_ms(&ts, timeout_ms);
        int r = pthread_cond_timedwait(&s->c, &s->m, &ts);
        if (r == ETIMEDOUT) { pthread_mutex_unlock(&s->m); return false; } // Wait with timeout
    }

    s->count = 0; // Capture the semaphore!
    pthread_mutex_unlock(&s->m);  // Release the mutex
    return true;
}


/**
 * @brief Give a binary semaphore (Linux implementation).
 */
void os_sem_give(os_sem_t s) {
    pthread_mutex_lock(&s->m); // Release the mutex
    s->count = 1; // Release the semaphore!
    pthread_cond_signal(&s->c); // Signal to those waiting
    pthread_mutex_unlock(&s->m); // Release the mutex
}


/* ---------- Mutex ---------- */

/** Mutex structure for Linux implementation */
struct os_mutex {
	pthread_mutex_t m;
};


/**
 * @brief Create a mutex (Linux implementation).
 */
os_mutex_t os_mutex_create(void) {
    os_mutex_t mu = calloc(1, sizeof(*mu));
    if (!mu) return NULL;
    pthread_mutex_init(&mu->m, NULL);
    return mu;
}


/**
 * @brief Lock a mutex (Linux implementation).
 */
void os_mutex_lock(os_mutex_t m)   { pthread_mutex_lock(&m->m); }


/**
 * @brief Unlock a mutex (Linux implementation).
 */
void os_mutex_unlock(os_mutex_t m) { pthread_mutex_unlock(&m->m); }


/* ---------- Misc ---------- */

/**
 * @brief Delay execution (Linux implementation).
 */
void os_delay_ms(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}
