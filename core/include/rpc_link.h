/**
 * @file    rpc_link.h
 * @brief   RPC Link Layer interface definitions.
 *
 * This header defines the link layer protocol for RPC communication,
 * including frame format, special bytes, and interface functions.
 */

#ifndef RPC_LINK_H_
#define RPC_LINK_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "rpc_crc8.h"
#include "rpc_errors.h"
#include "rpc_log.h"
#include "rpc_osal.h"
#include "rpc_phy.h"

#include "rpc_config.h"

// === Link Layer Special Bytes ===

#define SOF 0xFA     /** Start Of Frame (header beginning) */
#define SOD 0xFB     /** Start Of Data (payload beginning) */
#define EOF_ 0xFE    /** End Of Frame */


// === Frame Size Definitions ===

#define HEADER_SIZE         4    /** Header size: SOF + len_l + len_h + hdr_crc */
#define SOD_SIZE            1    /** Start Of Data marker size */
#define CRC_PKT_SIZE        1    /** Packet CRC field size */
#define EOF_SIZE            1    /** End Of Frame marker size */
#define TYPE_MSG_SIZE       1    /** Message type field size */
#define SEQ_MSG_SIZE        1    /** Sequence number field size */
#define TERM_SIZE           1    /** String terminator size */

/** Minimum payload size: type + seq + min_func_name + terminator */
#define MIN_PAYLOAD_SIZE    (TYPE_MSG_SIZE + SEQ_MSG_SIZE + MIN_FUNC_NAME_LEN + TERM_SIZE)

/** Maximum payload size: type + seq + max_func_name + terminator + max_args */
#define MAX_PAYLOAD_SIZE    (TYPE_MSG_SIZE + SEQ_MSG_SIZE + MAX_FUNC_NAME_LEN + TERM_SIZE \
		                     + MAX_FUNC_ARGS_RESP_SIZE)

/** Maximum packet length: SOD + max_payload + pkt_crc + EOF */
#define MAX_PKT_LEN         (SOD_SIZE + MAX_PAYLOAD_SIZE + CRC_PKT_SIZE + EOF_SIZE)

/** Minimum packet length: SOD + min_payload + pkt_crc + EOF */
#define MIN_PKT_LEN         (SOD_SIZE + MIN_PAYLOAD_SIZE + CRC_PKT_SIZE + EOF_SIZE)

// === Data Structures ===

/**
 * @brief Link layer payload event structure.
 *
 * This structure represents a complete payload packet ready for
 * upper layers (without SOD, packet CRC, and EOF markers).
 */
typedef struct {
	uint8_t payload[MAX_PAYLOAD_SIZE];    /**< Payload data buffer */
	size_t payload_len;                   /**< Actual payload length */
} link_payload_t;


// === Function Prototypes ===

/**
 * @brief Initialize the link layer parser.
 *
 * Resets the parser state machine to initial conditions.
 */
void rpc_link_init(void);

/**
 * @brief Feed raw bytes to the link layer parser.
 *
 * Processes incoming bytes through the state machine. When a complete
 * frame is successfully assembled, it sends the payload to the
 * qLinkToTrans queue.
 *
 * @param data Pointer to raw byte data.
 * @param len Number of bytes to process.
 */
void rpc_link_feed_bytes(const uint8_t* data, size_t len);

/**
 * @brief Build a link frame from payload and send via PHY layer.
 *
 * Constructs a complete link frame with header, payload, and CRC,
 * then sends it through the designated TX callback.
 *
 * @param payload Pointer to payload data.
 * @param len Length of payload data.
 * @return RPC_SUCCESS on success, RPC_ERROR on failure.
 */
int rpc_link_build_frame(const uint8_t* payload, size_t len);

/**
 * @brief Start the RX thread for link layer.
 *
 * Creates and starts the high-priority thread that handles
 * incoming data from PHY to LINK layer.
 */
void rpc_rx_start_thread(void);

/**
 * @brief Start the TX thread for link layer.
 *
 * Creates and starts the high-priority thread that handles
 * outgoing data from LINK to PHY layer.
 */
void rpc_tx_start_thread(void);

#endif /* RPC_LINK_H_ */
