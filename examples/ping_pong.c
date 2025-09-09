/**
 * @file    main.c
 * @brief   RPC Ping-Pong example application.
 *
 * @details This demo application demonstrates basic RPC functionality
 * with client-server communication using named pipes.
 *
 * @usage   To run this demo:
 *          - Terminal 1: ./rpc_app --server
 *          - Terminal 2: ./rpc_app --client
 *
 * @note    The server must be started before the client.
 *          Both applications will communicate via FIFOs in /tmp/
 *
 * @example
 *          Server output:
 *          ===== RPC Server Activated =====
 *
 *          Client output:
 *          ===== RPC Client Activated =====
 *          Response: pong
 *          Response: pong
 *          ...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "rpc.h"

/** First FIFO file path */
#define PATH_FIFO_FIRST    "/tmp/fifo_first"

/** Second FIFO file path */
#define PATH_FIFO_SECOND   "/tmp/fifo_second"

#define CLIENT_SEND_DELAY    1000
#define CLIENT_BUF_SIZE       100

extern const char* path_fifo_first;
extern const char* path_fifo_second;


/**
 * @brief Application operation modes.
 */
typedef enum {
	RPC_HELP,   /**< Show help message */
	RPC_SERVER, /**< Run as RPC server */
	RPC_CLIENT  /**< Run as RPC client */
} rpc_mode_t;


// === RPC Handler Example ===

/**
 * @brief Ping RPC handler function.
 *
 * Demonstrates a simple RPC handler that responds with "pong".
 *
 * @param args Input arguments (unused).
 * @param alen Arguments length (unused).
 * @param out Output buffer.
 * @param out_capacity Output buffer capacity.
 * @param out_len Output length.
 * @param timeout_ms Timeout (unused).
 * @return RPC_SUCCESS on success, RPC_ERROR_OVERFLOW if buffer too small.
 */
int handler_fn_ping(const uint8_t* args, uint16_t alen,
                     uint8_t* out, uint16_t out_capacity,
                     uint16_t* out_len,
                     uint32_t timeout_ms)
{
	(void)args; (void)alen; (void)timeout_ms;

	const char* resp = "pong";
	size_t need = strlen(resp);

	if (need > out_capacity) {
		return RPC_ERROR_OVERFLOW;
	}

	memcpy(out, resp, need);
	*out_len = (uint16_t)need;

	return RPC_SUCCESS;
}


/**
 * @brief Main application entry point.
 *
 * Parses command line arguments and starts RPC in client or server mode.
 *
 * @param argc Argument count.
 * @param argv Argument values.
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on error.
 */
int main(int argc, char* argv[]) {

	rpc_mode_t rpc_mode;

	// Parse command line arguments
	switch (argc) {
		case 1:
			printf("Not enough arguments. One argument is required: --server or --client\n");
			return EXIT_FAILURE;
			break;
		case 2:
			if (strcmp(argv[1], "--server") == 0 ||  strcmp(argv[1], "-s") == 0) {
				rpc_mode = RPC_SERVER;
				path_fifo_first = PATH_FIFO_FIRST;
				path_fifo_second = PATH_FIFO_SECOND;
				printf("===== RPC Server Activated =====\n");
			} else if (strcmp(argv[1], "--client") == 0 ||  strcmp(argv[1], "-c") == 0) {
				rpc_mode = RPC_CLIENT;
				path_fifo_first = PATH_FIFO_SECOND;
				path_fifo_second = PATH_FIFO_FIRST;
				printf("===== RPC Client Activated =====\n");
			} else if (strcmp(argv[1], "--help") == 0 ||  strcmp(argv[1], "-h") == 0) {
				rpc_mode = RPC_HELP;
				printf("One argument is required: --server or --client\n");
				return EXIT_FAILURE;
			} else {
				printf("Invalid argument. One argument is required: --server or --client\n");
				return EXIT_FAILURE;
			}
			break;
		default:
			printf("Too many arguments. One argument is required: --server or --client\n");
			return EXIT_FAILURE;
	}


	// Initialize RPC system
	int res = rpc_init();
	if (res < 0) {
		return EXIT_FAILURE;
	}

	// Server-specific initialization
	if (rpc_mode == RPC_SERVER) {
		res = rpc_register("ping", handler_fn_ping); // Регистрируем локальные RPC-функции

		if (res < 0) {
			return EXIT_FAILURE;
		}
	}

	// Start RPC threads
	rpc_start();

	// Client loop: send ping requests periodically
	if (rpc_mode == RPC_CLIENT) {
		uint8_t resp[CLIENT_BUF_SIZE];
		uint16_t rlen = sizeof(resp);
		int rc = RPC_SUCCESS;

		while (rc == RPC_SUCCESS) {
			rlen = sizeof(resp);
			rc = rpc_request("ping", NULL, 0, resp, &rlen, 1000);

			if (rc == RPC_SUCCESS) {
				printf("Response: ");
				for (int i = 0; i < rlen; i++) {
					printf("%c", (char)resp[i]);
				}
				printf("\n\n");
				os_delay_ms(CLIENT_SEND_DELAY);
			}
		}
	} else {
		// Server loop: wait indefinitely
		while (1) {
			os_delay_ms(OS_WAIT_FOREVER);
		}
	}

	return EXIT_SUCCESS;
}
