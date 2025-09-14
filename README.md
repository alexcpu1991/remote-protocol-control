# RPC Framework

Lightweight Remote Procedure Call framework for embedded systems and Linux environments.

## Features

- **Lightweight RPC Framework** — optimized for microcontrollers and embedded Linux.  
- **Layered Architecture** — clean separation into PHY → Link → Transport → RPC API.  
- **Reliable Framing** — start/end markers, length field, and CRC8 checks for header and payload integrity.  
- **Concurrent Requests** — supports multiple `rpc_request()` calls from different threads safely.  
- **Asynchronous Streams** — `rpc_stream()` for fire-and-forget style messages (no response expected).  
- **Worker Pool** — incoming RPC requests processed concurrently by a pool of worker threads.  
- **Easy Porting** — to support a new platform/OS, implement the interfaces in `rpc_phy.h` (physical layer) and `rpc_osal.h` (OS abstraction layer) under `platform/<your_platform>`.
- **Current implementation**: Linux POSIX in `platform/linux/rpc_osal_linux.c` 
  with named pipes transport in `platform/linux/rpc_phy_linux.c`.
- **Configurable** — limits and timeouts defined in `rpc_config.h` (`MAX_FUNC_NAME_LEN`, buffer sizes, default timeouts, etc.).  
- **Logging System** — pluggable logging with levels: TRACE, INFO, DEBUG, ERROR.   
- **Simple API** — just `rpc_init()`, `rpc_register()`, `rpc_request()`, and `rpc_stream()`.  
- **Example Application** — `ping_pong.c` demonstrates client-server RPC communication over Linux named pipes. 


## 📂 Project Structure

```
remote-protocol-control/
├── core                         # Core RPC library (platform-independent)
│   ├── include 
│   │   ├── rpc_config.h         # User configuration
│   │   ├── rpc_crc8.h           # CRC8 calculation
│   │   ├── rpc_errors.h         # Common error codes
│   │   ├── rpc.h                # Main API header
│   │   ├── rpc_link.h           # Link layer
│   │   ├── rpc_log.h            # Logging system
│   │   ├── rpc_osal.h           # OS abstraction layer
│   │   ├── rpc_phy.h            # Physical layer interface
│   │   ├── rpc_transport.h      # Transport layer
│   │   └── rpc_types.h          # Shared typedefs
│   └── src
│       ├── rpc.c                # Core RPC implementation
│       ├── rpc_crc8.c           # CRC8 calculation
│       ├── rpc_link.c           # Link layer implementation
│       └── rpc_transport.c      # Transport layer implementation
├── docs                         # Documentation
├── examples                     # Usage examples
│   ├── CMakeLists.txt           
│   └── ping_pong.c              # Ping-Pong example
└── platform                     # Platform-specific implementations
    └── linux                    
        ├── rpc_osal_linux.c     # Linux OS abstraction
        └── rpc_phy_linux.c      # Linux physical layer  
```


## 📖 API Overview

### Initialization & Control

**Initialize RPC system** (must be called first):  
```c
int rpc_init(void);
```

**Start RPC worker threads/tasks**:
```c  
void rpc_start(void);
```

### Function Registration
**Register function for remote calls**  
Registers a local RPC handler under a unique name.  
Must be called after rpc_start().

```c
int rpc_register(const char* name, rpc_fn_t fn);
```

### Remote Procedure Call

**Synchronous request/response**  
Thread-safe function that sends a request to the remote side and blocks until either a response is received or the timeout expires.  
Multiple threads can safely issue requests in parallel — each request is tracked independently by sequence numbers.
```c
int rpc_request(const char* name, const void* args, uint16_t args_len,
                void* resp_buf, uint16_t* resp_len, uint32_t timeout_ms);
```

**Asynchronous stream (fire-and-forget)**  
Sends a message to the remote side without expecting a response.   
Useful for telemetry, logging, or event notifications.
```c
int rpc_stream(const char* name, const void* args, uint16_t args_len);
```

### Handler Function Signature
**RPC function handler prototype**  
Called in the context of a worker thread.  
Must return RPC_SUCCESS or an appropriate error code (RPC_ERROR_*).
```c
typedef int (*rpc_fn_t)(const uint8_t* args, uint16_t alen,
                        uint8_t* out, uint16_t out_capacity,
                        uint16_t* out_len, uint32_t timeout_ms);
```


## 🔧 Configuration
Edit `core/include/rpc_config.h` to customize:
- Queue sizes and depths
- Timeout values
- Logging levels
- Memory allocation settings

## 📊 Logging Levels
Set `RPC_LOG_LEVEL` in `core/include/rpc_config.h`:
- RPC_LOG_LEVEL_NONE  - No logging
- RPC_LOG_LEVEL_ERROR - Critical errors only
- RPC_LOG_LEVEL_INFO  - Informational messages
- RPC_LOG_LEVEL_DEBUG - Detailed debug output
- RPC_LOG_LEVEL_TRACE - Verbose tracing


## 🏓 Ping-Pong Example
Basic RPC interaction demonstration:
- 🟢 Server registers the ping function
- 🔵 Client periodically calls ping
- 🟢 Server responds with pong to each request
- 🔵 Client receives response and prints it to console

### Prerequisites

- Linux environment
- CMake ≥ 3.10
- GCC compiler
- Make build system

### Building

```bash
git clone https://github.com/alexcpu1991/remote-protocol-control.git
cd remote-protocol-control/examples
mkdir build && cd build
cmake ..
make
```

### Running
Terminal 1 - Start Server:
```bash
./ping_pong --server
```
Output:
```bash
===== RPC Server Activated =====
```
Terminal 2 - Start Client
```bash
./ping_pong --client
```
Output:
```bash
 ===== RPC Client Activated =====
 Response: pong
 Response: pong
 ...
```
👉 Note: The server must be started before the client.


## 📄 License
MIT License - see LICENSE file for details.

## 📞 Support
For issues and questions:
- Create an Issue
- Check Documentation for detailed guides