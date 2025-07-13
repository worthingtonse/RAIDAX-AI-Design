# Network Interface Header Definitions (net.h)

## Module Purpose
This header file defines the complete interface for the high-performance, event-driven network layer supporting both TCP and UDP protocols. It establishes constants for performance tuning, security limits, timeout configurations, and declares all functions needed for connection management, event handling, and cross-thread communication in the RAIDA server network system.



#### conn_info_t Required Fields (connection context)
socketId: integer,                     // Socket file descriptor or ID
  readBuffer: byte[],                   // Buffer for incoming data
  readBufferLength: integer,            // Length of current read data
  writeBuffer: byte[],                  // Buffer for outgoing response
  writeBufferLength: integer,           // Length of data to send
  state: string,                        // Connection state (e.g. "READ_HEADER", "READ_BODY")
  clientAddress: string,               // Client IP address (for UDP/TCP)
  createdAt: timestamp                  // Creation time for timeout tracking


## Network Performance Constants

### Event System Configuration
- **RAIDA_EPOLL_TIMEOUT:** 10000 milliseconds - Timeout for event notification system
- **MAXEPOLLSIZE:** 10000 events - Maximum events that can be processed per wait operation
- **MAX_FDS:** 65535 - Maximum file descriptors the system can handle simultaneously

### Security and Resource Protection
- **MAX_BODY_SIZE:** 65536 bytes (64KB) - Maximum allowed request body size to prevent memory exhaustion attacks
- **SOCKET_TIMEOUT:** 2 seconds - Individual socket operation timeout

## Function Interface Declarations

### Main Network Functions

#### `init_and_listen_sockets`
**Parameters:** None
**Returns:** Integer status code (0 for success, negative for error)
**Purpose:** Initializes complete network infrastructure and runs main event loop until server shutdown

**Functionality:**
- Sets up event notification system (epoll on Linux)
- Creates and configures TCP and UDP listening sockets
- Implements main event processing loop
- Handles graceful shutdown and resource cleanup
- Manages cross-thread communication for write operations

#### `init_tcp_socket`
**Parameters:** None
**Returns:** Integer socket descriptor (negative on error)
**Purpose:** Creates and configures TCP listening socket for client connections

**Configuration Features:**
- Non-blocking operation mode for event-driven processing
- SO_REUSEADDR option for quick server restart capability
- TCP keep-alive with customized parameters (60s idle, 10s interval, 5 probes)
- Binding to configured port with maximum connection backlog

#### `init_udp_socket`
**Parameters:** None
**Returns:** Integer socket descriptor (negative on error)
**Purpose:** Creates and configures UDP socket for connectionless request processing

**Configuration Features:**
- Non-blocking mode for efficient event processing
- Same port as TCP for dual-protocol support
- Optimized for high-throughput connectionless operations

### Socket Configuration Utilities

#### `set_nonblocking`
**Parameters:**
- File descriptor (integer socket identifier)

**Returns:** Integer status code (0 for success, negative for error)
**Purpose:** Configures socket for non-blocking operation using system calls

**Used By:**
- TCP listening socket configuration
- UDP socket configuration
- Client connection setup

#### `set_blocking`
**Parameters:**
- File descriptor (integer socket identifier)

**Returns:** Integer status code (0 for success, negative for error)
**Purpose:** Reverts socket to blocking mode for specific operations

**Used By:**
- External connections to other RAIDA servers
- Synchronous operations requiring blocking behavior

### Connection Management

#### `alloc_ci`
**Parameters:**
- Socket descriptor (integer file descriptor)

**Returns:** Pointer to connection information structure (NULL on allocation failure)
**Purpose:** Allocates and initializes connection information structure for new connections

**Initialization Features:**
- Complete structure zeroing for clean state
- Socket descriptor assignment
- Initial state configuration for header reading
- Timestamp recording for performance monitoring
- Buffer size configuration for protocol processing

#### `free_ci`
**Parameters:**
- Connection information structure pointer

**Returns:** None
**Purpose:** Frees all resources associated with a connection

**Cleanup Responsibilities:**
- Socket address structure deallocation (UDP connections)
- Request body buffer cleanup
- Response output buffer cleanup
- Write buffer cleanup for TCP responses
- Main connection structure deallocation
- Null pointer safety handling

### Event Handling Functions

#### `handle_new_tcp_connection`
**Parameters:**
- TCP listening socket descriptor (integer)

**Returns:** None
**Purpose:** Accepts and configures new TCP connections with proper initialization

**NOTE:** Header signature discrepancy - implementation doesn't use epoll_fd parameter declared in header

**Process Features:**
- Multiple connection acceptance per event notification
- Client IP address extraction and logging
- Connection information structure setup
- Event system registration for new connections
- Error handling with proper cleanup

#### `handle_udp_request`
**Parameters:**
- UDP socket descriptor (integer)

**Returns:** None
**Purpose:** Processes incoming UDP datagrams with immediate response capability

**NOTE:** Header signature discrepancy - implementation doesn't use epoll_fd parameter declared in header

**Process Features:**
- Multiple datagram processing per event
- Client address extraction for response routing
- Immediate request processing (no thread pool delay)
- Direct response transmission
- Memory management for datagram buffers

#### `handle_connection_event`
**Parameters:**
- Connection information structure pointer
- Event mask (32-bit unsigned integer indicating event types)

**Returns:** None
**Purpose:** Routes connection events to appropriate read/write handlers

**NOTE:** Header signature discrepancy - implementation doesn't use epoll_fd parameter declared in header

**Event Types Handled:**
- EPOLLIN: Data available for reading
- EPOLLOUT: Socket ready for writing
- EPOLLERR/EPOLLHUP: Error conditions requiring connection cleanup
- Invalid event combinations

### Cross-Thread Communication

#### `arm_socket_for_write`
**Parameters:**
- Connection information structure pointer

**Returns:** None
**Purpose:** Signals main I/O thread that worker thread has completed request processing

**Thread Context:** Called by worker threads after processing requests to notify I/O thread

**Functionality:**
- Thread-safe queue management for write notifications
- Event-based signaling to main I/O thread
- Enables efficient cross-thread communication
- Prevents busy-waiting or polling for write readiness

**Mechanism Detail:** Uses Linux `eventfd` (or cross-platform equivalent) to notify main thread of ready-to-write sockets. Worker threads enqueue the `conn_info_t (connection context)*` into a lock-free or mutex-protected queue and write to the eventfd. Main thread reads the eventfd and processes write-ready sockets.


## Header File Signature Corrections

### Implementation vs. Declaration Discrepancies
**Important Note:** The header file contains function signatures that differ from the actual implementation:

#### Affected Functions
1. **`handle_new_tcp_connection`**
   - Header declares: `handle_new_tcp_connection(int tsocket, int epoll_fd)`
   - Implementation uses: `handle_new_tcp_connection(int tsocket)`

2. **`handle_udp_request`**
   - Header declares: `handle_udp_request(int usocket, int epoll_fd)`
   - Implementation uses: `handle_udp_request(int usocket)`

3. **`handle_connection_event`**
   - Header declares: `handle_connection_event(conn_info_t *ci, int epoll_fd, uint32_t events)`
   - Implementation uses: `handle_connection_event(conn_info_t *ci, uint32_t events)`

#### Recommendation
**Follow Implementation Signatures:** When implementing in other languages, use the actual function signatures from the implementation rather than the header declarations for these three functions.

## Data Structure Dependencies

### Required External Types
- **Connection Information Structure:** Complete connection state management from protocol layer
- **Socket Address Structures:** System network address types
- **Event Mask Types:** 32-bit unsigned integer for event notification
- **File Descriptor Types:** Integer types for socket management

### Protocol Integration Requirements
- Connection structure must include state management fields
- Protocol header size constants for buffer management
- Request/response processing integration points
- Error code definitions for network operation failures

## Performance and Scalability Features

### Event-Driven Architecture
- **Non-Blocking Operations:** All socket operations use non-blocking mode
- **Edge-Triggered Events:** Minimal event notification overhead
- **Batch Processing:** Multiple events processed per system call
- **Efficient State Management:** Minimal overhead per connection

### Resource Management
- **Connection Limits:** MAX_FDS enforces system-wide connection limits
- **Memory Limits:** MAX_BODY_SIZE prevents memory exhaustion attacks
- **Timeout Management:** Automatic cleanup of stale connections
- **File Descriptor Management:** Proper cleanup prevents descriptor exhaustion

### Concurrency Support
- **Single I/O Thread:** Main thread handles all network events
- **Worker Thread Pool:** Request processing distributed across multiple threads
- **Cross-Thread Communication:** Efficient signaling for write completion
- **Lock-Free I/O:** Network operations don't require synchronization

## Security Considerations

### Input Validation
- **Request Size Limits:** MAX_BODY_SIZE prevents oversized requests
- **Connection Limits:** Resource exhaustion prevention through connection caps
- **Timeout Enforcement:** Prevents resource hoarding through timeouts
- **Address Validation:** Client IP logging for security monitoring

### Resource Protection
- **Memory Management:** Careful allocation and cleanup prevents memory leaks
- **File Descriptor Protection:** Proper cleanup prevents descriptor exhaustion
- **Error Handling:** Secure error responses prevent information leakage
- **Buffer Overflow Protection:** Size validation prevents buffer overflows

### Network Security
- **Protocol Validation:** All requests validated before processing
- **Connection Tracking:** Global tracking enables monitoring and control
- **Rate Limiting Framework:** Foundation for implementing rate controls
- **Audit Logging:** Connection events logged for security analysis

## Integration Dependencies

### Required System APIs
- **Event Notification:** Platform-specific event system (epoll on Linux)
- **Socket Operations:** POSIX socket API for network operations
- **Cross-Thread Signaling:** Platform event signaling (eventfd on Linux)
- **Memory Management:** System allocation and deallocation functions

### External Module Dependencies
- **Protocol Layer:** Request/response validation, command processing
- **Configuration System:** Network parameters, port configuration
- **Thread Pool System:** Worker thread management for request processing
- **Logging System:** Debug output, error reporting, security logging
- **Statistics System:** Request counting, performance monitoring

### Provided Interface
- **Network Entry Point:** Primary server network initialization
- **Connection Management:** Complete connection lifecycle handling
- **Event Processing:** Efficient event-driven request handling
- **Cross-Thread Communication:** Worker thread integration support

## Platform Abstraction Requirements

### Operating System Dependencies
- **Event Systems:** Linux epoll, BSD kqueue, Windows IOCP equivalents
- **Socket APIs:** POSIX socket operations with platform variations
- **Signal Systems:** Cross-thread communication mechanisms
- **File Descriptor Management:** Platform-specific FD handling

### Portability Considerations
- **Event Notification:** Abstract event system for cross-platform support
- **Socket Configuration:** Platform-specific socket option handling
- **Thread Communication:** Cross-platform signaling mechanisms
- **Resource Limits:** Platform-appropriate resource limit handling

## Error Handling Standards

### Return Code Conventions
- **Success Operations:** Return 0 for successful completion
- **Error Conditions:** Return negative values for various error types
- **Pointer Functions:** Return NULL on allocation or lookup failures
- **Resource Cleanup:** All error paths include proper resource cleanup

### Error Recovery Strategies
- **Connection Errors:** Automatic connection cleanup and resource reclamation
- **System Errors:** Graceful handling of system call failures
- **Memory Errors:** Safe degradation when memory allocation fails
- **Protocol Errors:** Secure error responses before connection termination

This network interface header provides the complete foundation for implementing high-performance, scalable network communication in any programming language while maintaining the architectural integrity and security characteristics of the original RAIDA server design.