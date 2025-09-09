/**
 * @file    rpc_crc8.c
 * @brief   CRC8 computation implementation.
 *
 * This module implements the CRC8 calculation algorithm using
 * a bit-by-bit approach with configurable polynomial and initial value.
 */

#include "rpc_crc8.h"


/**
 * @brief Compute CRC8 checksum for given data.
 *
 * Implements a standard CRC8 calculation algorithm using bit-by-bit
 * processing. The algorithm handles each byte of input data and
 * processes each bit through the polynomial division.
 *
 * @param data Pointer to the data buffer.
 * @param len Length of data in bytes.
 * @param init Initial value for CRC calculation.
 * @param poly Polynomial to use for CRC calculation.
 * @return Computed CRC8 checksum.
 *
 * @note This implementation uses a bit-by-bit approach which is
 *       simple but may not be the most efficient for large data blocks.
 *       For better performance, consider a table-driven approach.
 */
uint8_t crc8_compute(const uint8_t* data, size_t len, uint8_t init, uint8_t poly)
{
	uint8_t crc = init;

	for (size_t i = 0; i < len; ++i) {
		crc ^= data[i];
		for (uint8_t b = 0; b < 8; ++b) {
			uint8_t mask = (crc & 0x80) ? 0xFF : 0x00;
			crc = (crc << 1) ^ (poly & mask);
		}
	}
	return crc;
}


