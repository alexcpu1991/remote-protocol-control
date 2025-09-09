/**
 * @file    rpc_crc8.h
 * @brief   CRC8 computation interface and configuration.
 *
 * This header provides the interface for CRC8 calculation functions
 * and defines standard CRC8 parameters.
 */

#ifndef INC_RPC_CRC8_H_
#define INC_RPC_CRC8_H_

#include <stddef.h>
#include <stdint.h>

// === CRC8 Parameters ===

/**
 * @brief Standard CRC-8/ATM polynomial (0x07).
 *
 * Polynomial: x^8 + x^2 + x + 1
 * Commonly used in communication protocols.
 */
#define CRC8_POLY 0x07

/**
 * @brief Standard CRC-8 initialization value (0x00).
 *
 * Initial value for CRC calculation.
 */
#define CRC8_INIT 0x00


/**
 * @brief Compute CRC8 checksum for given data.
 *
 * This function calculates a CRC8 checksum using the specified polynomial
 * and initial value. It can be used with standard parameters (CRC8_POLY, CRC8_INIT)
 * or custom parameters for specific protocols.
 *
 * @param data Pointer to the data buffer.
 * @param len Length of data in bytes.
 * @param init Initial value for CRC calculation.
 * @param poly Polynomial to use for CRC calculation.
 * @return Computed CRC8 checksum.
 */
uint8_t crc8_compute(const uint8_t* data, size_t len, uint8_t init, uint8_t poly);


#endif /* INC_RPC_CRC8_H_ */
