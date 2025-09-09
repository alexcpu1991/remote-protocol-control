/**
 * @file    rpc_phy_linux.c
 * @brief   Linux FIFO implementation of RPC PHY layer.
 *
 * This module provides a Linux-specific physical layer implementation
 * using named pipes (FIFOs) for inter-process communication.
 */

#include "rpc_phy.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

const char* path_fifo_first;  /**< Path to the first FIFO (for sending) */
const char* path_fifo_second; /**< Path to the second FIFO (for receiving) */

static int fd_fifo_first;  /**< File descriptor for the first FIFO */
static int fd_fifo_second; /**< File descriptor for the second FIFO */


/**
 * @brief Create a named pipe (FIFO) with the specified path.
 * @param path Path to the FIFO to create.
 */
static void create_fifo(const char* path) {
	mkfifo(path, 0666);
}


/**
 * @brief Initialize the PHY layer.
 *
 * Creates and opens both FIFOs for bidirectional communication.
 *
 * @return RPC_SUCCESS on success, RPC_ERROR on failure.
 */
int rpc_phy_init(void) {
	create_fifo(path_fifo_first);
	create_fifo(path_fifo_second);

	/* Open fifo_first */
	fd_fifo_first = open(path_fifo_first, O_RDWR | O_NOCTTY);
	if (fd_fifo_first < 0) {
		RPC_LOG_ERROR("Error opening fifo_first");
		return RPC_ERROR;
	}

	/* Open fifo_second */
	fd_fifo_second = open(path_fifo_second, O_RDWR | O_NOCTTY);
	if (fd_fifo_second < 0) {
		RPC_LOG_ERROR("Error opening fifo_second");
		return RPC_ERROR;
	}

	return RPC_SUCCESS;
}


/**
 * @brief Send data through the PHY layer.
 * @param data Pointer to data to send.
 * @param len Length of data to send.
 * @return Number of bytes sent on success, negative value on error.
 */
int rpc_phy_send(const uint8_t *data, size_t len) {
	int res;
	res = write(fd_fifo_first, data, len);
	return res;
}


/**
 * @brief Receive data from the PHY layer.
 * @param data Pointer to buffer for received data.
 * @param len Maximum length of data to receive.
 * @return Number of bytes received on success, negative value on error.
 */
int rpc_phy_receive(uint8_t *data, size_t len) {
	int res;
	res = read(fd_fifo_second, data, len);
	return res;
}


/**
 * @brief Deinitialize the PHY layer.
 *
 * Closes both FIFO file descriptors.
 */
void rpc_phy_deinit(void) {
	/* Close the fifo's */
	close(fd_fifo_first);
	close(fd_fifo_second);
}
