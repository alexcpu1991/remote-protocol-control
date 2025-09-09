/**
 * @file    rpc_osal.h
 * @brief   RPC OS Abstraction Layer interface.
 *
 * This header provides platform-independent OS abstraction for RPC system.
 * It defines interfaces for threads, queues, semaphores, and mutexes.
 */


#ifndef RPC_OSAL_H_
#define RPC_OSAL_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define OS_WAIT_FOREVER   0xFFFFFFFFU  /**< Wait forever timeout value */
#define OS_NO_WAIT        0            /**< No wait timeout value */

#define OS_TRUE           1            /**< Boolean true value */
#define OS_FALSE          0            /**< Boolean false value */


/* ---------- Thread ---------- */

/** Thread function pointer type */
typedef void* (*os_thread_fn)(void* arg);

/** Thread handle type */
typedef struct os_thread* os_thread_t;

/**
 * @brief Create a new thread.
 *
 * @param name Thread name (optional, implementation specific).
 * @param fn Thread function to execute.
 * @param arg Argument passed to thread function.
 * @param stack_size Stack size in bytes.
 * @param priority Thread priority (implementation specific).
 * @return Thread handle on success, NULL on failure.
 */
os_thread_t os_thread_create(const char* name,
                             os_thread_fn fn,
                             void* arg,
                             uint16_t stack_size,
                             uint8_t priority);


/* ---------- Queues ---------- */

/** Queue handle type */
typedef struct os_queue* os_queue_t;


/**
 * @brief Create a new queue.
 *
 * @param length Maximum number of items in queue.
 * @param item_size Size of each item in bytes.
 * @return Queue handle on success, NULL on failure.
 */
os_queue_t os_queue_create(size_t length, size_t item_size);


/**
 * @brief Send an item to the queue.
 *
 * @param q Queue handle.
 * @param item Pointer to item to send.
 * @param timeout_ms Timeout in milliseconds (OS_WAIT_FOREVER for blocking).
 * @return true on success, false on failure or timeout.
 */
bool os_queue_send(os_queue_t q, const void* item, uint32_t timeout_ms);


/**
 * @brief Receive an item from the queue.
 *
 * @param q Queue handle.
 * @param item Pointer to buffer for received item.
 * @param timeout_ms Timeout in milliseconds (OS_WAIT_FOREVER for blocking).
 * @return true on success, false on failure or timeout.
 */
bool os_queue_recv(os_queue_t q, void* item, uint32_t timeout_ms);


/* ---------- Binary Semaphores ---------- */

/** Binary semaphore handle type */
typedef struct os_sem*    os_sem_t;


/**
 * @brief Create a binary semaphore.
 *
 * @return Semaphore handle on success, NULL on failure.
 */
os_sem_t os_sem_create_binary(void);


/**
 * @brief Take (wait for) a binary semaphore.
 *
 * @param s Semaphore handle.
 * @param timeout_ms Timeout in milliseconds (OS_WAIT_FOREVER for blocking).
 * @return true on success, false on failure or timeout.
 */
bool os_sem_take(os_sem_t s, uint32_t timeout_ms);


/**
 * @brief Give (release) a binary semaphore.
 *
 * @param s Semaphore handle.
 */
void os_sem_give(os_sem_t s);


/* ---------- Mutex ---------- */

/** Mutex handle type */
typedef struct os_mutex* os_mutex_t;


/**
 * @brief Create a mutex.
 *
 * @return Mutex handle on success, NULL on failure.
 */
os_mutex_t os_mutex_create(void);


/**
 * @brief Lock a mutex.
 *
 * @param m Mutex handle.
 */
void os_mutex_lock(os_mutex_t m);


/**
 * @brief Unlock a mutex.
 *
 * @param m Mutex handle.
 */
void os_mutex_unlock(os_mutex_t m);


/* ---------- Misc ---------- */

/**
 * @brief Delay execution for specified milliseconds.
 *
 * @param ms Milliseconds to delay.
 */
void os_delay_ms(uint32_t ms);

#endif /* RPC_OSAL_H_ */
