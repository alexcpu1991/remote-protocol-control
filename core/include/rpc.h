/**
 * @file rpc.h
 * @brief Public API for Remote Procedure Call (RPC) framework.
 *
 * This header defines the top-level RPC API that allows applications
 * to initialize, start, register, and call remote functions.
 */

#ifndef RPC_H_
#define RPC_H_


#include <stdint.h>
#include "rpc_types.h"
#include "rpc_errors.h"
#include "rpc_osal.h"


/**
 * @brief Initialize the RPC system.
 *
 * Must be called before any other RPC functions.
 */
int rpc_init(void);


/**
 * @brief Start RPC tasks/threads.
 *
 * Typically creates worker threads or tasks for handling incoming RPC requests.
 */
void rpc_start(void);


/**
 * @brief Register a function to be callable remotely.
 *
 * @param name   Null-terminated function name (must not be empty).
 * @param fn     Pointer to handler function.
 *
 * @return RPC_SUCCESS on success, RPC_ERROR on failure.
 */
int  rpc_register(const char* name, rpc_fn_t fn);


/**
 * @brief Perform a remote procedure call (synchronous).
 *
 * Sends a request message, waits for a response, and copies
 * the result into the provided response buffer.
 *
 * @param name      Null-terminated function name to call.
 * @param args      Pointer to arguments buffer (may be NULL if no args).
 * @param args_len  Length of arguments.
 * @param resp_buf  Buffer to receive response data.
 * @param resp_len  In: capacity of @p resp_buf. Out: actual response length.
 * @param timeout_ms Timeout to wait for response (ms).
 *
 * @return RPC_SUCCESS on success, or an error code (<0).
 */
int rpc_request(const char* name, const void* args, uint16_t args_len,
			    void* resp_buf, uint16_t* resp_len, uint32_t timeout_ms);


/**
 * @brief Send a stream message (asynchronous, no response expected).
 *
 * @param name      Null-terminated function name.
 * @param args      Pointer to arguments buffer (may be NULL).
 * @param args_len  Length of arguments.
 *
 * @return RPC_SUCCESS on success, or an error code (<0).
 */
int rpc_stream(const char* name, const void* args, uint16_t args_len);


#endif /* RPC_H_ */
