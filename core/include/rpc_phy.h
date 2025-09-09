/**
 * @file    rpc_phy.h
 * @brief   RPC Physical Layer abstract interface.
 *
 * This header defines the abstract physical layer interface for RPC communication.
 * It provides platform-independent functions for data transmission and reception.
 * Concrete implementations must be provided for specific hardware platforms.
 */

#ifndef RPC_PHY_H_
#define RPC_PHY_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "rpc_log.h"
#include "rpc_errors.h"


// === Function Prototypes ===

/**
 * @brief Initialize the physical layer.
 *
 * Performs hardware-specific initialization of the communication channel.
 * This function must be called before any other PHY layer operations.
 *
 * @return RPC_SUCCESS on success, RPC_ERROR on failure.
 */
int rpc_phy_init(void);

/**
 * @brief Send data through the physical layer.
 *
 * Transmits raw data bytes over the communication channel.
 * This is a blocking operation that may wait for transmission completion.
 *
 * @param data Pointer to the data buffer to send.
 * @param len Number of bytes to send.
 * @return Number of bytes actually sent on success, negative value on error.
 */
int rpc_phy_send(const uint8_t *data, size_t len);

/**
 * @brief Receive data from the physical layer.
 *
 * Receives raw data bytes from the communication channel.
 * This is a blocking operation that waits for incoming data.
 *
 * @param data Pointer to the buffer for storing received data.
 * @param len Maximum number of bytes to receive.
 * @return Number of bytes actually received on success, negative value on error.
 */
int rpc_phy_receive(uint8_t *data, size_t len);

/**
 * @brief Deinitialize the physical layer.
 *
 * Releases resources and shuts down the communication channel.
 * This function should be called when the PHY layer is no longer needed.
 */
void rpc_phy_deinit(void);


#endif /* RPC_PHY_H_ */
