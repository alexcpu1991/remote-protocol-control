/**
 * @file    rpc_config.h
 * @brief   RPC System Configuration.
 *
 * This header contains all configurable parameters for the RPC system.
 * Adjust these values according to your application requirements.
 */


#ifndef RPC_CONFIG_H_
#define RPC_CONFIG_H_

#include <stdint.h>


// === Log level ===

/** Default log level for the RPC system */
#define RPC_LOG_LEVEL    RPC_LOG_LEVEL_DEBUG


// === Function Configuration ===

/** Minimum function name length */
#define MIN_FUNC_NAME_LEN             1

/** Maximum function name length */
#define MAX_FUNC_NAME_LEN            32

/** Maximum arguments/response size in bytes */
#define MAX_FUNC_ARGS_RESP_SIZE      64


// === System Limits ===

/** Maximum number of registered functions */
#define NUM_REG_FUNC                 16

/** Size of the request waiter table */
#define REQ_TABLE_SIZE                8

/** Number of RPC worker threads */
#define RPC_WORKER_COUNT              1


// === Queue Configuration ===

/** Depth of link-to-transport queue */
#define Q_LINK_TO_TRANS_DEPTH        16

/** Depth of transport-to-link queue */
#define Q_TRANS_TO_LINK_DEPTH        16

/** Depth of RPC request queue for workers */
#define Q_RPC_REQUEST_DEPTH          16


// === Timeout Configuration ===

/** Default request timeout in milliseconds */
#define REQ_TIMEOUT_MS_DEFAULT      200

/** Default handler execution timeout in milliseconds */
#define HANDLER_TIMEOUT_MS_DEFAULT  150

#endif /* RPC_CONFIG_H_ */
