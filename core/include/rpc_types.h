#ifndef RPC_TYPES_H_
#define RPC_TYPES_H_

#include <stdint.h>

/**
 * @brief Type of handler function for registered RPC methods.
 *
 * @param args      Input arguments buffer.
 * @param args_len  Length of input arguments.
 * @param out_buf   Output buffer for response data.
 * @param out_cap   Capacity of @p out_buf.
 * @param out_len   Pointer to store actual response length.
 * @param timeout_ms Timeout for processing (in milliseconds).
 *
 * @return RPC_SUCCESS on success, or an error code (<0).
 */
typedef int (*rpc_fn_t)(const uint8_t* args, uint16_t alen,
                        uint8_t* out, uint16_t out_capacity,
                        uint16_t* out_len, uint32_t timeout_ms);

#endif /* RPC_TYPES_H_ */
