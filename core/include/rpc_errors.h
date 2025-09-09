/**
 * @file    rpc_errors.h
 * @brief   RPC Error codes and utility macros.
 *
 * This header defines standard error codes for RPC operations
 * and provides utility macros for error handling.
 */


#ifndef RPC_ERRORS_H_
#define RPC_ERRORS_H_


// === RPC Error Codes ===

#define RPC_SUCCESS                  0 /**< Operation completed successfully */
#define RPC_ERROR                   -1 /**< General unspecified error */
#define RPC_ERROR_OVERFLOW          -2 /**< Buffer overflow or size exceeded */
#define RPC_ERROR_TIMEOUT           -3 /**< Operation timed out */
#define RPC_ERROR_INVALID_ARGS      -4 /**< Invalid arguments provided */


// === Utility Macros ===

/**
 * @def RPC_IS_SUCCESS(code)
 * @brief Check if return code indicates success.
 * @param code Error code to check.
 * @return true if code >= 0 (success), false otherwise.
 */
#define RPC_IS_SUCCESS(code) ((code) >= 0)

/**
 * @def RPC_IS_ERROR(code)
 * @brief Check if return code indicates error.
 * @param code Error code to check.
 * @return true if code < 0 (error), false otherwise.
 */
#define RPC_IS_ERROR(code)   ((code) < 0)

#endif /* RPC_ERRORS_H_ */
