/**
 * @file    rpc_transport.c
 * @brief   RPC Transport Layer implementation.
 *
 * This module implements the transport layer of the RPC system, including:
 * - Function registration and lookup
 * - Message serialization/deserialization
 * - Request/response synchronization
 * - Worker thread management
 * - Stream message handling
 */


#include "rpc_transport.h"


// === Worker Structure ===

/**
 * @brief RPC request structure used by worker threads.
 */
typedef struct {
    uint8_t type;                            /**< Message type: REQ, RESP, ERR, STREAM */
    uint8_t seq;                             /**< Sequence number of the request */
    char name[MAX_FUNC_NAME_LEN + 1];        /**< Function name */
    uint8_t args[MAX_FUNC_ARGS_RESP_SIZE];   /**< Function arguments */
    uint16_t alen;                           /**< Length of arguments */
} rpc_request_t;

static os_queue_t qRpcRequests;   /**< Shared queue for all RPC workers */
static uint8_t worker_count = 0;  /**< Worker counter (for numbering threads) */
static os_mutex_t s_worker_count; /**< Mutex to protect worker_count */


// === Function Registry ===

/**
 * @brief Registered function entry.
 */
typedef struct {
	const char* name;    /**< Function name (static string) */
	rpc_fn_t fn;         /**< Function pointer */
} reg_entry_t;

static reg_entry_t s_reg[NUM_REG_FUNC]; /**< Registry of functions */
static size_t s_reg_count = 0;          /**< Number of registered functions */
static os_mutex_t s_reg_mtx;            /**< Mutex for registry access */


// === Response Waiters ===

/**
 * @brief Waiter structure for pending RPC requests.
 */
typedef struct {
	uint8_t seq;              /**< Sequence number */
	os_sem_t done;            /**< Semaphore signaled when response is ready */
	int result_code;          /**< Result code (0 = OK, <0 = error) */
	uint8_t* resp_buf;        /**< Response buffer pointer */
	uint16_t* resp_len;       /**< Pointer to actual response length */
	uint16_t resp_buf_cap;    /**< Capacity of the response buffer */
	bool in_use;              /**< Marks if this waiter is active */
} waiter_t;

static waiter_t s_wait[REQ_TABLE_SIZE]; /**< Array of waiters */
static os_mutex_t s_wait_mtx;           /**< Mutex for waiter access */


// === Inter-layer queues ===

os_queue_t qLinkToTrans; /**< Queue from link layer to transport */
os_queue_t qTransToLink; /**< Queue from transport to link layer */


// === Helper Functions ===

/**
 * @brief Find a registered function by name.
 *
 * @param name Function name.
 * @return Index of function or RPC_ERROR if not found.
 */
static int find_reg(const char* name)
{
	int idx = RPC_ERROR;

	os_mutex_lock(s_reg_mtx);
	for (size_t i = 0; i < s_reg_count; i++) {
		if (strncmp(s_reg[i].name, name, MAX_FUNC_NAME_LEN) == 0) {
			idx = (int)i;
			break;
		}
	}
	os_mutex_unlock(s_reg_mtx);

	return idx;
}

int register_fn(const char* name, rpc_fn_t fn)
{
	int rc = RPC_ERROR;

	os_mutex_lock(s_reg_mtx);
	if (s_reg_count < (sizeof(s_reg)/sizeof(s_reg[0]))) {
		s_reg[s_reg_count].name = name;
		s_reg[s_reg_count].fn = fn;
		s_reg_count++;
		rc = RPC_SUCCESS;
	}
	os_mutex_unlock(s_reg_mtx);

	return rc;
}


/**
 * @brief Build a transport message.
 *
 * @param type Message type (MSG_REQ, MSG_RESP, MSG_ERR, MSG_STREAM).
 * @param seq Sequence number.
 * @param name Function name.
 * @param args Pointer to arguments buffer.
 * @param alen Length of arguments.
 * @param out Output buffer.
 * @param olen Output buffer capacity.
 * @return Size of the serialized payload, 0 on error.
 */
static size_t rpc_trans_build_msg(uint8_t type, uint8_t seq, const char* name,
                                  const uint8_t* args, uint16_t alen,
                                  uint8_t* out, size_t olen)
{
	// Check input arguments
    if (!out || !name) return 0;

    // Check message type
    if (type != MSG_REQ && type != MSG_RESP && type != MSG_ERR && type != MSG_STREAM) {
        return 0;
    }

    // Check name
    size_t nlen = strlen(name);
    if (nlen < MIN_FUNC_NAME_LEN || nlen > MAX_FUNC_NAME_LEN)
        return 0;

    // Checking arguments
    if (alen > MAX_FUNC_ARGS_RESP_SIZE)
        return 0;

    // Checking arguments
    size_t need = TYPE_MSG_SIZE + SEQ_MSG_SIZE + nlen + TERM_SIZE + alen;

    if (need < MIN_PAYLOAD_SIZE || need > MAX_PAYLOAD_SIZE)
        return 0;

    if (need > olen)
        return 0;

    // Serialization
    size_t pos = 0;
    out[pos++] = type;
    out[pos++] = seq;

    memcpy(&out[pos], name, nlen);
    pos += nlen;

    out[pos++] = '\0'; // terminating null of name

    if (alen && args) {
        memcpy(&out[pos], args, alen);
        pos += alen;
    }

    return pos; // final payload size
}


/**
 * @brief Parse a transport message.
 *
 * @param in Input buffer.
 * @param ilen Input length.
 * @param type Output: message type.
 * @param seq Output: sequence number.
 * @param name Output: pointer to function name.
 * @param args Output: pointer to arguments.
 * @param alen Output: arguments length.
 * @return RPC_SUCCESS on success, RPC_ERROR otherwise.
 */
static int rpc_trans_parse_msg(const uint8_t* in, size_t ilen,
					           uint8_t* type, uint8_t* seq,
					           const char** name, const uint8_t** args, uint16_t* alen)
{
	// Checking pointers
	if (!in || !type || !seq || !name || !args || !alen)
		return RPC_ERROR;

	// Payload Boundaries
	if (ilen < MIN_PAYLOAD_SIZE || ilen > MAX_PAYLOAD_SIZE)
		return RPC_ERROR;

	 uint8_t t = in[0];
	 uint8_t s = in[1];

	 // Valid message types
	if (t != MSG_REQ && t != MSG_RESP && t != MSG_ERR && t != MSG_STREAM)
		return RPC_ERROR;

	*type = t;
	*seq  = s;

	// Function name starts with offset=2
	const size_t name_start = 2;
	if (name_start >= ilen)
		return RPC_ERROR;

	// Finding null terminator
    const void* term = memchr(in + name_start, '\0', ilen - name_start);
    if (!term)
        return RPC_ERROR; // no string null terminator

    size_t nlen = (const uint8_t*)term - (in + name_start);

    // Check name length
    if (nlen < MIN_FUNC_NAME_LEN || nlen > MAX_FUNC_NAME_LEN) return RPC_ERROR;

	*name = (const char*)&in[name_start];

	// Arguments start immediately after '\0'
	size_t i = name_start + nlen + 1;
	if (i > ilen)
		return RPC_ERROR;

	size_t arg_len = ilen - i;
	if (arg_len > MAX_FUNC_ARGS_RESP_SIZE)
		return RPC_ERROR;

	*alen = (uint16_t)arg_len;
	*args = &in[i];

	return RPC_SUCCESS;
}


/**
 * @brief Initialize waiter structures.
 */
static void rpc_trans_init_waiter(void) {
	s_wait_mtx = os_mutex_create();

	for (int i = 0; i < REQ_TABLE_SIZE; i++) {
		s_wait[i].in_use = false;
		s_wait[i].done = os_sem_create_binary();
	}
}


/**
 * @brief Allocate a waiter for new request.
 *
 * @param out_seq Pointer to store allocated sequence number.
 * @param out_w Pointer to store waiter reference.
 * @return RPC_SUCCESS on success, RPC_ERROR otherwise.
 */
static int rpc_trans_alloc_waiter(uint8_t* out_seq, waiter_t** out_w)
{
	static uint8_t s_next_seq = 1;

	for (int attempt = 0; attempt < 255; attempt++) {
		os_mutex_lock(s_wait_mtx);

		uint8_t s = s_next_seq++;
		if (s_next_seq == 0) s_next_seq = 1; // skip 0
		for (int i = 0; i < REQ_TABLE_SIZE; i++) {
			if (!s_wait[i].in_use) {
				s_wait[i].in_use = true;
				s_wait[i].seq = s;
				*out_seq = s;
				*out_w = &s_wait[i];
				os_mutex_unlock(s_wait_mtx);
				return RPC_SUCCESS;
			}
		}

		os_mutex_unlock(s_wait_mtx);
		os_delay_ms(1);
	}

	return RPC_ERROR;
}


/**
 * @brief Find waiter by sequence number.
 *
 * @param seq Sequence number.
 * @return Pointer to waiter or NULL if not found.
 */
static waiter_t* rpc_trans_find_waiter(uint8_t seq)
{
	waiter_t *ret = NULL;

	os_mutex_lock(s_wait_mtx);
	for (int i = 0; i < REQ_TABLE_SIZE; i++) {
		if (s_wait[i].in_use && s_wait[i].seq == seq) ret = &s_wait[i];
	}
	os_mutex_unlock(s_wait_mtx);

	return ret;
}


/**
 * @brief Free waiter after completion.
 *
 * @param w Waiter pointer.
 */
static void rpc_trans_free_waiter(waiter_t* w)
{
	if (!w) return;

	os_mutex_lock(s_wait_mtx);
	w->in_use = false;
	os_mutex_unlock(s_wait_mtx);
}



/**
 * @brief Initialize the RPC transport layer.
 *
 * Creates mutexes, initializes waiter table, and creates inter-layer queues.
 */
void rpc_trans_init(void)
{
	s_worker_count = os_mutex_create();
	s_reg_mtx = os_mutex_create();
	rpc_trans_init_waiter();
	qLinkToTrans = os_queue_create(Q_LINK_TO_TRANS_DEPTH, sizeof(link_payload_t));
	qTransToLink = os_queue_create(Q_TRANS_TO_LINK_DEPTH, sizeof(link_payload_t));
	qRpcRequests = os_queue_create(Q_RPC_REQUEST_DEPTH, sizeof(rpc_request_t));

}


/**
 * @brief Send an RPC request and wait for response.
 *
 * @param name Function name to call.
 * @param args Arguments buffer.
 * @param args_len Arguments length.
 * @param resp_buf Response buffer.
 * @param resp_len Response length (input: capacity, output: actual length).
 * @param timeout_ms Timeout in milliseconds.
 * @return RPC_SUCCESS on success, error code on failure.
 */
int rpc_trans_request(const char* name,
			 const void* args, uint16_t args_len,
			 void* resp_buf, uint16_t* resp_len,
			 uint32_t timeout_ms)
{
    RPC_LOG_TRACE("RPC call started: %s, args_len: %u, timeout: %u ms",
                  name ? name : "(null)", args_len, timeout_ms);

    // Function name validation
    if (!name) {
        RPC_LOG_ERROR("RPC call failed: name is NULL");
        return RPC_ERROR;
    }

    size_t nlen = strlen(name);
    if (nlen < MIN_FUNC_NAME_LEN || nlen > MAX_FUNC_NAME_LEN) {
        RPC_LOG_ERROR("Invalid RPC function name length: %zu", nlen);
        return RPC_ERROR;
    }

    // Validate the response buffer
    if (!resp_buf || !resp_len) {
        RPC_LOG_ERROR("Invalid response buffer (NULL pointer), function: %s", name);
        return RPC_ERROR;
    }

    if (*resp_len < MAX_FUNC_ARGS_RESP_SIZE) {
        RPC_LOG_ERROR("Response buffer too small (%u < %u), function: %s",
                      *resp_len, MAX_FUNC_ARGS_RESP_SIZE, name);
        return RPC_ERROR;
    }

    // Allocate a waiter
	uint8_t seq = 0;
	waiter_t* w = NULL;
	if (rpc_trans_alloc_waiter(&seq, &w) != 0) {
		RPC_LOG_ERROR("No free waiters available for RPC call: %s", name);
		return RPC_ERROR;
	}
	RPC_LOG_DEBUG("Allocated waiter, sequence: %u", seq);

	// Initialize the waiter
	w->resp_buf = (uint8_t*)resp_buf;
	w->resp_len = resp_len;
	w->resp_buf_cap = *resp_len;

	// Forming a message
	link_payload_t lp;
	lp.payload_len = rpc_trans_build_msg(MSG_REQ, seq, name,
			                             (const uint8_t*)args, args_len,
										 lp.payload, sizeof(lp.payload));
	if (!lp.payload_len) {
		RPC_LOG_ERROR("Failed to build message for RPC: %s, args_len: %u", name, args_len);
		rpc_trans_free_waiter(w);
		return RPC_ERROR;
	}
	RPC_LOG_DEBUG("Message built successfully, size: %zu bytes", lp.payload_len);

	// Send to link-layer queue
	if (os_queue_send(qTransToLink, &lp, OS_WAIT_FOREVER) != OS_TRUE) {
		RPC_LOG_ERROR("Failed to send message to qTransToLink: %s", name);
		rpc_trans_free_waiter(w);
		return RPC_ERROR;
	}
	RPC_LOG_TRACE("Message sent to link layer");

	// Waiting for response
	uint32_t actual_timeout = timeout_ms ? timeout_ms : REQ_TIMEOUT_MS_DEFAULT;
	RPC_LOG_DEBUG("Waiting for response, timeout: %u ms", actual_timeout);
	if (os_sem_take(w->done, actual_timeout) != OS_TRUE) {
		RPC_LOG_ERROR("RPC call timeout: %s, sequence: %u, timeout: %u ms",
				      name, seq, actual_timeout);
		rpc_trans_free_waiter(w);
		return RPC_ERROR;
	}

	// Read the result
	int rc = w->result_code;

	// Logging depending on the result
    if (RPC_IS_SUCCESS(rc)) {
        RPC_LOG_INFO("RPC call succeeded: %s, response length: %u\n",
                    name, (resp_len && *resp_len) ? *resp_len : 0);
    } else {
        RPC_LOG_ERROR("RPC call failed: %s, error code: %d, response length: %u\n",
                    name, rc, (resp_len && *resp_len) ? *resp_len : 0);
    }

	rpc_trans_free_waiter(w);
	return rc;
}


/**
 * @brief Send an RPC stream message (no response expected).
 *
 * @param name Function name to call.
 * @param args Arguments buffer.
 * @param args_len Arguments length.
 * @return RPC_SUCCESS on success, error code on failure.
 */
int rpc_trans_stream(const char* name, const void* args, uint16_t args_len)
{
    RPC_LOG_TRACE("RPC stream started: %s, args_len: %u",
                  name ? name : "(null)", args_len);

    // Check function name
    if (!name) {
        RPC_LOG_ERROR("RPC stream failed: name is NULL");
        return RPC_ERROR;
    }

    size_t nlen = strlen(name);
    if (nlen < MIN_FUNC_NAME_LEN || nlen > MAX_FUNC_NAME_LEN) {
        RPC_LOG_ERROR("Invalid RPC function name length: %zu", nlen);
        return RPC_ERROR;
    }

    // Generate message (without waiter)
    link_payload_t lp;
    lp.payload_len = rpc_trans_build_msg(MSG_STREAM, 0, name,
                                         (const uint8_t*)args, args_len,
                                         lp.payload, sizeof(lp.payload));
    if (!lp.payload_len) {
        RPC_LOG_ERROR("Failed to build STREAM message: %s, args_len: %u", name, args_len);
        return RPC_ERROR;
    }
    RPC_LOG_DEBUG("STREAM message built successfully, size: %zu bytes", lp.payload_len);

    // Send to lower level queue
    if (os_queue_send(qTransToLink, &lp, OS_WAIT_FOREVER) != OS_TRUE) {
        RPC_LOG_ERROR("Failed to send STREAM message to qTransToLink: %s", name);
        return RPC_ERROR;
    }

    RPC_LOG_TRACE("STREAM message sent: %s", name);
    return RPC_SUCCESS;
}


/**
 * @brief Handle incoming messages from link layer.
 *
 * This function resolves waiters (for RESP/ERR) or
 * enqueues requests to worker threads (for REQ/STREAM).
 *
 * @param p Pointer to payload.
 * @param n Payload size.
 */
static void rpc_trans_handle_incoming(const uint8_t* p, size_t n)
{
	RPC_LOG_TRACE("Handling incoming message, size: %zu bytes", n);

	uint8_t type = 0, seq = 0;
	const char* name = NULL;
	const uint8_t* args = NULL;
	uint16_t alen = 0;

	// Parsing the message
	if (rpc_trans_parse_msg(p, n, &type, &seq, &name, &args, &alen) != 0) {
		RPC_LOG_ERROR("Failed to parse message, size: %lu bytes", n);
		return; // Incorrect format - ignore
	}
	RPC_LOG_INFO("Parsed message: type=%s, seq=%u, name=%s, args_len=%u",
	              (type == MSG_REQ) ? "REQUEST" :
	              (type == MSG_STREAM) ? "STREAM" :
	              (type == MSG_RESP) ? "RESPONSE" : "ERROR",
				  seq, name ? name : "NULL", alen);

	// === Handling RESPONSE / ERROR messages ===
	if (type == MSG_RESP || type == MSG_ERR) {

	    waiter_t* w = rpc_trans_find_waiter(seq);
	    if (w) {
	        int rc = (type == MSG_RESP) ? RPC_SUCCESS : RPC_ERROR;

	        if (alen > w->resp_buf_cap) {
	        	// Buffer is smaller than required - error
	            RPC_LOG_ERROR("Response buffer overflow: need=%u, cap=%u, seq=%u",
	                          (unsigned)alen, (unsigned)w->resp_buf_cap, seq);
	            w->result_code = RPC_ERROR_OVERFLOW;
	            if (w->resp_len) {
	                *w->resp_len = 0; // nothing copied
	            }
	            os_sem_give(w->done);
	            return; // exit
	        }

	        // Data is placed - copy
	        if (w->resp_buf && alen > 0) {
	            memcpy(w->resp_buf, args, alen);
	        }
	        if (w->resp_len) {
	            *w->resp_len = (uint16_t)alen;
	        }
	        w->result_code = rc;

	        os_sem_give(w->done);
	        RPC_LOG_INFO("Waiter awakened for seq: %u", seq);
	    } else {
	        RPC_LOG_ERROR("No waiter found for response, seq: %u", seq);
	    }
	    return;
	}

	// === Processing REQUEST / STREAM messages ===
	if (type == MSG_REQ || type == MSG_STREAM) {
		rpc_request_t req = {0};
		req.type = type;
		req.seq  = seq;
		strncpy(req.name, name, MAX_FUNC_NAME_LEN);
		memcpy(req.args, args, alen);
		req.alen = alen;

		if (os_queue_send(qRpcRequests, &req, 0) != OS_TRUE) {
			RPC_LOG_ERROR("qRpcRequests full, drop request: %s", req.name);
		}
	}
}


/**
 * @brief Worker thread function for processing requests.
 *
 * Processes RPC requests from the queue, calls registered functions,
 * and sends responses back.
 *
 * @param arg Not used.
 * @return NULL
 */
static void* ThreadRPCWorker(void* arg)
{
    rpc_request_t req;

    os_mutex_lock(s_worker_count);
    uint8_t worker_num = ++worker_count; // assign worker number
    os_mutex_unlock(s_worker_count);

    RPC_LOG_INFO("[Worker %u] thread started", worker_num);

    for (;;) {
        if (os_queue_recv(qRpcRequests, &req, OS_WAIT_FOREVER) == OS_TRUE) {
            RPC_LOG_INFO("[Worker %u] Handling request: %s, seq=%u",
                         worker_num, req.name, req.seq);

            int idx = find_reg(req.name);
            uint8_t out[MAX_FUNC_ARGS_RESP_SIZE];
            uint16_t olen = 0;
            int rc = RPC_ERROR;

            // Find and call a registered function
            if (idx >= 0 && s_reg[idx].fn) {
            	RPC_LOG_TRACE("[Worker %u] Found handler for: %s", worker_num, name);
                rc = s_reg[idx].fn(req.args, req.alen,
                		           out, sizeof(out), &olen,
                		           HANDLER_TIMEOUT_MS_DEFAULT);

                // insurance in case of wrong handler
                if (olen > sizeof(out)) {
                	RPC_LOG_ERROR("[Worker %u] BUG: handler returned olen=%u > cap=%u, name=%s",
                			      worker_num, olen, (unsigned)sizeof(out), req.name);
                    rc = RPC_ERROR_OVERFLOW;
                    olen = 0; // we do not return corrupted data
                }
            }

            if (req.type == MSG_REQ) {
            	// Generating a response
                link_payload_t lp;
                if (rc == RPC_SUCCESS) {
                    lp.payload_len = rpc_trans_build_msg(MSG_RESP, req.seq, req.name, out, olen,
                                                         lp.payload, sizeof(lp.payload));
                    RPC_LOG_INFO("[Worker %u] Built response message, size: %zu bytes",
                    		     worker_num, lp.payload_len);
                } else {
                	const char* emsg = (idx < 0) ? "NOFUNC" :
									   (rc == RPC_ERROR_OVERFLOW) ? "OVERFLOW" :
									   (rc == RPC_ERROR_INVALID_ARGS) ? "INVALID_ARGS" :
									   (rc == RPC_ERROR_TIMEOUT) ? "TIMEOUT" : "FAIL";
                    lp.payload_len = rpc_trans_build_msg(MSG_ERR, req.seq, req.name,
                                                         (const uint8_t*)emsg, (uint16_t)strlen(emsg),
                                                         lp.payload, sizeof(lp.payload));
                    RPC_LOG_ERROR("[Worker %u] Built error message: %s", worker_num, emsg);
                }

                // Sending a response
                if (lp.payload_len) {
                	if (os_queue_send(qTransToLink, &lp, OS_WAIT_FOREVER) != OS_TRUE) {
						RPC_LOG_ERROR("[Worker %u] Failed to send response to qTransToLink, seq: %u",
								      worker_num, req.seq);
					}
                }
            } else {
            	// === STREAM ===
                RPC_LOG_INFO("[Worker %u] STREAM processed (no response), name=%s",
                             worker_num, req.name);
            }
        }
    }
    return NULL;
}


/**
 * @brief Start RPC worker threads.
 *
 * Creates and starts multiple worker threads for parallel
 * request processing.
 */
void rpc_worker_start_thread(void)
{
	for (int i = 0; i < RPC_WORKER_COUNT; i++) {
		char name[16];
		snprintf(name, sizeof(name), "RPC_Worker%d", i);
		os_thread_create(name, ThreadRPCWorker, NULL, 1024, 2);
	}
}


/**
 * @brief Transport layer thread function.
 *
 * Processes messages from link layer queue:
 * - If response/error: resolves waiting request (by sequence number)
 * - If request/stream: forwards to worker threads via queue
 *
 * @param arg Thread argument (unused).
 * @return NULL.
 */
static void* ThreadTrans(void* arg)
{
	(void)arg;
	link_payload_t m;

	RPC_LOG_INFO("Transport thread started");

	for (;;) {
		if (os_queue_recv(qLinkToTrans, &m, OS_WAIT_FOREVER) == OS_TRUE) {
			RPC_LOG_DEBUG("Received message from link layer, size: %zu bytes", m.payload_len);
			rpc_trans_handle_incoming(m.payload, m.payload_len);
			RPC_LOG_TRACE("Message processing completed");
		}
	}
	return NULL;
}


static os_thread_t sThreadTrans;

/**
 * @brief Start the transport layer thread.
 */
void rpc_transport_start_thread(void)
{
	sThreadTrans = os_thread_create("trans", ThreadTrans, NULL, 1024, 2);
}

