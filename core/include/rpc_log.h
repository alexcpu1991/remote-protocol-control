/**
 * @file    rpc_log.h
 * @brief   RPC Logging System.
 *
 * This header provides a configurable logging system for RPC components
 * with multiple log levels, conditional compilation, and colored output.
 */


#ifndef RPC_LOG_H_
#define RPC_LOG_H_

#include <stdio.h>

#include "rpc_config.h"


// === Log Levels ===

#define RPC_LOG_LEVEL_NONE   0 /**< No logging */
#define RPC_LOG_LEVEL_ERROR  1 /**< Error level logging */
#define RPC_LOG_LEVEL_INFO   2 /**< Info level logging */
#define RPC_LOG_LEVEL_DEBUG  3 /**< Debug level logging */
#define RPC_LOG_LEVEL_TRACE  4 /**< Trace level logging (most verbose) */


// === Current Log Level Configuration ===

/**
 * @brief Current log level (can be changed at runtime).
 *
 * Default value is RPC_LOG_LEVEL_INFO if not defined elsewhere.
 * Can be overridden by compiler definitions.
 */
#ifndef RPC_LOG_LEVEL
#define RPC_LOG_LEVEL RPC_LOG_LEVEL_INFO  /**< Default log level */
#endif


// === Conditional Logging Macros ===

/**
 * @def RPC_LOG_ERROR(...)
 * @brief Log error messages (red color).
 *
 * Messages at this level indicate critical errors that prevent
 * normal operation. Compiled only when RPC_LOG_LEVEL >= RPC_LOG_LEVEL_ERROR.
 */
#if (RPC_LOG_LEVEL >= RPC_LOG_LEVEL_ERROR)
#define RPC_LOG_ERROR(...) do { \
    printf("\033[1;31m[RPC_ERR]\033[0m "); \
    printf(" [%s:%s] ", __FILE__, __func__); \
    printf(__VA_ARGS__); \
    printf("\n"); \
} while (0)
#else
#define RPC_LOG_ERROR(...)
#endif


/**
 * @def RPC_LOG_INFO(...)
 * @brief Log informational messages (green color).
 *
 * Messages at this level indicate normal operation events.
 * Compiled only when RPC_LOG_LEVEL >= RPC_LOG_LEVEL_INFO.
 */
#if (RPC_LOG_LEVEL >= RPC_LOG_LEVEL_INFO)
#define RPC_LOG_INFO(...) do { \
    printf("\033[1;32m[RPC_INFO]\033[0m "); \
    printf(" [%s:%s] ", __FILE__, __func__); \
    printf(__VA_ARGS__); \
    printf("\n"); \
} while (0)
#else
#define RPC_LOG_INFO(...)
#endif


/**
 * @def RPC_LOG_DEBUG(...)
 * @brief Log debug messages.
 *
 * Messages at this level provide detailed debugging information.
 * Compiled only when RPC_LOG_LEVEL >= RPC_LOG_LEVEL_DEBUG.
 */
#if (RPC_LOG_LEVEL >= RPC_LOG_LEVEL_DEBUG)
#define RPC_LOG_DEBUG(...) do { \
    printf("[RPC_DBG] "); \
    printf(" [%s:%s] ", __FILE__, __func__); \
    printf(__VA_ARGS__); \
    printf("\n"); \
} while (0)
#else
#define RPC_LOG_DEBUG(...)
#endif


/**
 * @def RPC_LOG_TRACE(...)
 * @brief Log trace messages (most verbose).
 *
 * Messages at this level provide extremely detailed tracing
 * of internal operations. Compiled only when RPC_LOG_LEVEL >= RPC_LOG_LEVEL_TRACE.
 */
#if (RPC_LOG_LEVEL >= RPC_LOG_LEVEL_TRACE)
#define RPC_LOG_TRACE(...) do { \
    printf("[RPC_TRC] "); \
    printf(" [%s:%s] ", __FILE__, __func__); \
    printf(__VA_ARGS__); \
    printf("\n"); \
} while (0)
#else
#define RPC_LOG_TRACE(...)
#endif


// === Conditional Logging Macros ===

/**
 * @def RPC_LOG_ERROR_IF(cond, ...)
 * @brief Log error message only if condition is true.
 * @param cond Condition to check.
 * @param ... printf-style format string and arguments.
 */
#define RPC_LOG_ERROR_IF(cond, ...) do { \
    if (cond) RPC_LOG_ERROR(__VA_ARGS__); \
} while (0)


/**
 * @def RPC_LOG_INFO_IF(cond, ...)
 * @brief Log info message only if condition is true.
 * @param cond Condition to check.
 * @param ... printf-style format string and arguments.
 */
#define RPC_LOG_INFO_IF(cond, ...) do { \
    if (cond) RPC_LOG_INFO(__VA_ARGS__); \
} while (0)

#endif /* RPC_LOG_H_ */
