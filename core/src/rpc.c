/**
 * @file rpc.c
 * @brief Implementation of the public RPC API.
 *
 * This file contains the top-level functions of the RPC framework.
 * Most of the work is delegated to the lower layers:
 *  - transport (rpc_transport.c)
 *  - link (rpc_link.c)
 *  - physical I/O (rpc_phy.c)
 */

#include "rpc.h"
#include "rpc_log.h"
#include "rpc_transport.h"


/**
 * @brief Initialize the RPC system.
 *
 * Sets up transport, link, physical and OS resources. Must be called before
 * rpc_start() or any other RPC operation.
 */
int rpc_init(void) {
	int res = RPC_SUCCESS;

	RPC_LOG_INFO("===== RPC Init =====");
	RPC_LOG_INFO("===== PRC Log level = %d =====", RPC_LOG_LEVEL);

	rpc_trans_init(); // Transport Init
	rpc_link_init(); // Link Init
	res = rpc_phy_init(); // PHY Init

	if (RPC_IS_ERROR(res)) {
		RPC_LOG_ERROR("PHY Level Fail Init");
		res =  RPC_ERROR;
	}

	return res;
}

/**
 * @brief Start RPC threads/tasks.
 *
 * Launches worker threads that process incoming RPC requests.
 * After calling this function, the RPC system is ready for use.
 */
void rpc_start(void) {
	rpc_transport_start_thread();
	rpc_worker_start_thread();
	rpc_rx_start_thread();
	rpc_tx_start_thread();
	os_delay_ms(1000);
}


/**
 * @brief Start RPC threads/tasks.
 *
 * Launches worker threads that process incoming RPC requests.
 * After calling this function, the RPC system is ready for use.
 */
int rpc_register(const char* name, rpc_fn_t fn)
{
	int res;

	res = register_fn(name, fn);

	if (RPC_IS_ERROR(res)) {
		RPC_LOG_ERROR("Function registration error: %s", name);
	} else {
		RPC_LOG_INFO("Function registration completed successfully: %s", name);
	}

	return res;
}


/**
 * @brief Perform a synchronous RPC request (request/response).
 *
 * @copydoc rpc_request()
 */
int rpc_request(const char* name, const void* args, uint16_t args_len,
			    void* resp_buf, uint16_t* resp_len, uint32_t timeout_ms) {
	return rpc_trans_request(name, args, args_len,
			                 resp_buf, resp_len, timeout_ms);
}


/**
 * @brief Send a one-way stream message (no response expected).
 *
 * @copydoc rpc_stream()
 */
int rpc_stream(const char* name, const void* args, uint16_t args_len) {
	return rpc_trans_stream(name, args, args_len);
}

