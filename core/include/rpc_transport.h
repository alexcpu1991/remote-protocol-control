/**
 * @file    rpc_transport.h
 * @brief   RPC Transport Layer interface.
 *
 * This header defines the transport layer interface for RPC communication,
 * including message types, function registration, and API functions.
 */

#ifndef RPC_TRANSPORT_H_
#define RPC_TRANSPORT_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "rpc_errors.h"
#include "rpc_link.h"
#include "rpc_log.h"
#include "rpc_osal.h"
#include "rpc_types.h"
#include "rpc_config.h"


// === Transport Message Types ===

#define MSG_REQ       0x0B /**< Request message type */
#define MSG_STREAM    0x0C /**< Stream message type (no response expected) */
#define MSG_RESP      0x16 /**< Response message type */
#define MSG_ERR       0x21 /**< Error message type */


// === Function Prototypes ===

/**
 * @brief Register a function in the RPC registry.
 * @param name Function name to register.
 * @param fn Function pointer.
 * @return RPC_SUCCESS on success, RPC_ERROR on failure.
 */
int register_fn(const char* name, rpc_fn_t fn);


/**
 * @brief Initialize the transport layer.
 *
 * Creates queues, waiter tables, and initializes all transport layer components.
 * Must be called before any other transport layer operations.
 */
void rpc_trans_init(void);


/**
 * @brief Synchronous remote function call.
 *
 * Sends a request and waits for response with timeout.
 *
 * @param name Function name to call.
 * @param args Arguments buffer.
 * @param args_len Arguments length.
 * @param resp_buf Response buffer.
 * @param resp_len Response length (input: capacity, output: actual length).
 * @param timeout_ms Timeout in milliseconds.
 * @return RPC_SUCCESS on success, error code on failure.
 */
int rpc_trans_request(const char* name,
			          const void* args, uint16_t args_len,
			          void* resp_buf, uint16_t* resp_len,
			          uint32_t timeout_ms);


/**
 * @brief Send a stream message (no response expected).
 *
 * @param name Function name.
 * @param args Arguments buffer.
 * @param args_len Arguments length.
 * @return RPC_SUCCESS on success, error code on failure.
 */
int rpc_trans_stream(const char* name, const void* args, uint16_t args_len);


/**
 * @brief Start the transport layer thread.
 *
 * Creates and starts the thread that handles message processing
 * between link and transport layers.
 */
void rpc_transport_start_thread(void);


/**
 * @brief Start RPC worker threads.
 *
 * Creates and starts multiple worker threads for parallel
 * request processing.
 */
void rpc_worker_start_thread(void);


#endif /* RPC_TRANSPORT_H_ */
