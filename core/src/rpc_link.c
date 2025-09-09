/**
 * @file    rpc_link.c
 * @brief   RPC Link Layer implementation.
 *
 * This module handles the link layer of the RPC system, including:
 * - Frame parsing using a state machine
 * - Frame construction with CRC
 * - Communication with transport layer via queues
 * - RX/TX thread management
 */

#include "rpc_link.h"


extern os_queue_t qLinkToTrans;  /** External queue for messages from link to transport layer */
extern os_queue_t qTransToLink;  /** External queue for messages from transport to link layer */


/**
 * @brief Parser state enumeration.
 */
typedef enum {
	ST_WAIT_SOF,     /**< Waiting for Start Of Frame */
	ST_READ_LEN1,    /**< Reading length byte 1 (LSB) */
	ST_READ_LEN2,    /**< Reading length byte 2 (MSB) */
	ST_READ_HDRCRC,  /**< Reading header CRC */
	ST_WAIT_SOD,     /**< Waiting for Start Of Data */
	ST_READ_PAYLOAD, /**< Reading payload data */
	ST_READ_PKTCRC,   /**< Reading packet CRC */
	ST_WAIT_EOF      /**< Waiting for End Of Frame */
} st_t;


/**
 * @brief Parser context structure.
 */
static struct {
	st_t st;                            /**< Current parser state */
	uint16_t length;                    /**< Packet length from SOD to EOF */
	uint8_t hdr[3];                     /**< Header buffer: SOF + len_l + len_h */
	size_t payload_pos;                 /**< Current payload position */
	uint8_t payload[MAX_PAYLOAD_SIZE];  /**< Payload buffer */
} P;


/**
 * @brief Initialize the link layer parser.
 *
 * Resets the parser state machine to initial conditions and clears all buffers.
 */
void rpc_link_init(void)
{
	memset(&P, 0, sizeof(P));
	P.st = ST_WAIT_SOF;
}


/**
 * @brief Reset the parser to initial state.
 *
 * Clears all internal state and prepares parser for new frame processing.
 */
static void rpc_link_reset_parser(void)
{
	P.st = ST_WAIT_SOF;
	P.payload_pos = 0;
	P.length = 0;
}


/**
 * @brief Feed bytes to the link layer parser state machine.
 *
 * Processes incoming bytes through the state machine. When a complete
 * frame is successfully assembled, it sends the payload to the
 * qLinkToTrans queue.
 *
 * @param d Pointer to raw byte data.
 * @param n Number of bytes to process.
 */
void rpc_link_feed_bytes(const uint8_t* d, size_t n)
{
	RPC_LOG_TRACE("Feeding %zu bytes to link layer parser", n);

	for (size_t i = 0; i < n; i++) {
		uint8_t b = d[i];
		RPC_LOG_TRACE("Processing byte: 0x%02X, state: %d", b, P.st);

		switch (P.st) {
			case ST_WAIT_SOF:
				if (b == SOF) {
					RPC_LOG_DEBUG("SOF detected: 0x%02X", b);
					P.hdr[0] = b;
					P.st = ST_READ_LEN1;
				} else {
					RPC_LOG_ERROR("Waiting for SOF, got: 0x%02X", b);
				}
				break;
			case ST_READ_LEN1:
				P.hdr[1] = b;
				P.st = ST_READ_LEN2;
				break;
			case ST_READ_LEN2:
				P.hdr[2] = b;
				P.length = ((uint16_t)P.hdr[2] << 8) | P.hdr[1];
				RPC_LOG_DEBUG("Packet length: %u bytes", P.length);

				/* Checking the correctness of the packet length */
				if (P.length < MIN_PKT_LEN || P.length > MAX_PKT_LEN) {
					RPC_LOG_ERROR("Invalid packet length: %u (min: %u, max: %u)",
								  P.length, MIN_PKT_LEN, MAX_PKT_LEN);
					rpc_link_reset_parser();
					break;
				}

				P.st = ST_READ_HDRCRC;
				break;
			case ST_READ_HDRCRC:
				uint8_t hdr_crc = crc8_compute(P.hdr, 3, CRC8_INIT, CRC8_POLY);
				if (hdr_crc != b) {
					RPC_LOG_ERROR("Header CRC mismatch! Expected: 0x%02X, Got: 0x%02X", hdr_crc, b);
					rpc_link_reset_parser();
					break;
				}
				P.st = ST_WAIT_SOD;
				break;
			case ST_WAIT_SOD:
				if (b == SOD) {
					P.payload_pos = 0;
					P.st = ST_READ_PAYLOAD;
				} else {
					RPC_LOG_ERROR("Expected SOD (0x%02X), got: 0x%02X", SOD, b);
					rpc_link_reset_parser();
				}
				break;
			case ST_READ_PAYLOAD:
				if (P.payload_pos < MAX_PAYLOAD_SIZE && P.payload_pos < (size_t)(P.length - 3)) {
					// length includes: [SOD] payload[...] [pkt_crc8] [EOF]
					// We only read the payload, the last 2 bytes will go to the next states
					P.payload[P.payload_pos++] = b;

					// If you have already typed the whole body (payload_len == length-3),
					// then we wait for pkt_crc
					if (P.payload_pos == (size_t)(P.length - 3)) {
						P.st = ST_READ_PKTCRC;
					}
				} else {
					// overflow/inconsistent length
					RPC_LOG_ERROR("Payload overflow!");
					rpc_link_reset_parser();
				}
				break;
			case ST_READ_PKTCRC: {
				// Calculate the CRC of the packet by [SOD + payload]
				uint8_t tmp[MAX_PAYLOAD_SIZE + 1];
				tmp[0] = SOD;
				memcpy(&tmp[1], P.payload, P.payload_pos);
				uint8_t pkt_crc = crc8_compute(tmp, P.payload_pos + 1, CRC8_INIT, CRC8_POLY);
				if (pkt_crc != b) {
					RPC_LOG_ERROR("Packet CRC mismatch! Expected: 0x%02X, Got: 0x%02X", pkt_crc, b);
					rpc_link_reset_parser();
					break;
				}
				P.st = ST_WAIT_EOF;
				break;
			}
			case ST_WAIT_EOF:
				if (b == EOF_) {
					RPC_LOG_INFO("Frame received successfully, payload size: %zu bytes", P.payload_pos);
					link_payload_t lp;
					lp.payload_len = P.payload_pos;
					memcpy(lp.payload, P.payload, lp.payload_len);
					if (os_queue_send(qLinkToTrans, &lp, OS_WAIT_FOREVER) != OS_TRUE) {
						RPC_LOG_ERROR("Failed to send payload to transport queue");
					}
				} else {
					 RPC_LOG_ERROR("Expected EOF (0x%02X), got: 0x%02X", EOF_, b);
				}
				rpc_link_reset_parser();
				break;
			}
	}
}


/**
 * @brief Build a link frame from payload and send to PHY layer.
 *
 * Constructs a complete link frame with:
 * - Header (SOF + length + header CRC)
 * - Start of Data marker (SOD)
 * - Payload data
 * - Packet CRC
 * - End of Frame marker (EOF)
 *
 * @param payload Pointer to payload data.
 * @param len Length of payload data.
 * @return RPC_SUCCESS on success, RPC_ERROR on failure.
 */
int rpc_link_build_frame(const uint8_t* payload, size_t len)
{
	if (payload == NULL || len > MAX_PAYLOAD_SIZE || len < MIN_PAYLOAD_SIZE) {
		RPC_LOG_ERROR("Invalid arguments");
		return RPC_ERROR;
	}

	uint8_t frame[HEADER_SIZE + MAX_PKT_LEN];
	size_t pos = 0;

	frame[pos++] = SOF;
	// length = SOD + payload(len) + pkt_crc + EOF => len + 3
	uint16_t L = (uint16_t)(len + 3);
	frame[pos++] = (uint8_t)(L & 0xFF);
	frame[pos++] = (uint8_t)(L >> 8);

	uint8_t hdr_crc = crc8_compute(frame, 3, CRC8_INIT, CRC8_POLY);
	frame[pos++] = hdr_crc;

	frame[pos++] = SOD;

	memcpy(&frame[pos], payload, len);
	pos += len;

	uint8_t pkt_crc = crc8_compute(&frame[4], len + 1, CRC8_INIT, CRC8_POLY); // [SOD..payload]
	frame[pos++] = pkt_crc;
	frame[pos++] = EOF_;

	int res = rpc_phy_send(frame, pos);
	if (res < 0) {
		RPC_LOG_ERROR("Error send frame");
		return RPC_ERROR;
	}

	RPC_LOG_INFO("Frame sending successful");

	return RPC_SUCCESS;
}


/**
 * @brief RX thread function (PHY → LINK).
 *
 * High priority thread that:
 * - Periodically reads from PHY layer receive buffer
 * - Feeds bytes to link layer parser state machine
 *
 * @param arg Thread argument (unused).
 * @return NULL.
 */
static void* ThreadRX(void* arg)
{
	(void)arg;
	int res;
	uint8_t b;

	RPC_LOG_INFO("RX thread started");

	for (;;) {
		res = rpc_phy_receive(&b, 1);

		if (res < 0) {
			RPC_LOG_ERROR("Failed to receive data from PHY layer: error %d", res);
			continue;
		}

		rpc_link_feed_bytes(&b, res);
	}
	return NULL;
}

static os_thread_t sThreadRX;

/**
 * @brief Start the RX thread for link layer.
 *
 * Creates and starts the high-priority thread that handles
 * incoming data from PHY to LINK layer.
 */
void rpc_rx_start_thread(void)
{
	sThreadRX = os_thread_create("rx", ThreadRX, NULL, 1024, 2);
}


/**
 * @brief TX thread function (LINK → PHY).
 *
 * High priority thread that:
 * - Reads messages from transport layer queue
 * - Builds link frames using rpc_link_build_frame()
 * - Sends frames to PHY layer
 *
 * @param arg Thread argument (unused).
 * @return NULL.
 */
static void* ThreadTX(void* arg)
{
	(void)arg;
	link_payload_t m;

	RPC_LOG_INFO("TX thread started");

	for (;;) {
		if (os_queue_recv(qTransToLink, &m, OS_WAIT_FOREVER) == OS_TRUE) {
			RPC_LOG_DEBUG("Received message from transport layer, size: %zu bytes", m.payload_len);
			rpc_link_build_frame(m.payload, m.payload_len);
		}
	}
	return NULL;
}

static os_thread_t sThreadTX;

/**
 * @brief Start the TX thread for link layer.
 *
 * Creates and starts the high-priority thread that handles
 * outgoing data from LINK to PHY layer.
 */
void rpc_tx_start_thread(void)
{
	sThreadTX = os_thread_create("tx", ThreadTX, NULL, 1024, 2);
}
