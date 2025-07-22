# Network Interface and Event Loop (net.c)

## Module Purpose
This module implements the core network infrastructure for RAIDA servers, providing high-performance asynchronous I/O using epoll, non-blocking socket management, cross-thread communication, and support for both TCP and UDP protocols. It includes specialized handling for the DDoS-resistant integrity protocol and manages concurrent client connections through thread pool integration.

## Constants and Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_FDS` | Variable | Maximum number of file descriptors for connection tracking |
| `MAXEPOLLSIZE` | Variable | Maximum number of events per epoll_wait call |
| `RAIDA_EPOLL_TIMEOUT` | Variable | Timeout for epoll_wait operations (milliseconds) |
| `MOD_QUEUE_SIZE` | 1024 | Size of modification queue for cross-thread communication |
| `RAIDA_SERVER_RCV_TIMEOUT` | 32 | Timeout for inter-RAIDA server communication (seconds) |

## Connection States
| State | Description |
|-------|-------------|
| `STATE_WANT_READ_HEADER` | Connection waiting to read request header |
| `STATE_WANT_READ_BODY` | Connection waiting to read request body |
| `STATE_PROCESSING` | Request being processed by worker thread |
| `STATE_WANT_WRITE` | Connection ready to write response |
| `STATE_DONE` | Connection finished, ready to be closed |

## Data Structures

### Connection Information Structure
| Field | Type | Description |
|-------|------|-------------|
| `sk` | Integer | Socket file descriptor |
| `state` | Connection State | Current connection state for non-blocking I/O |
| `sa` | Socket Address Pointer | Client address (UDP only, NULL for TCP) |
| `ip` | String[16] | Client IP address for logging |
| `read_buf` | Byte Array | Buffer for reading request headers |
| `body` | Byte Pointer | Dynamically allocated request body buffer |
| `write_buf` | Byte Pointer | Response buffer for writing |
| `bytes_to_read` | Integer | Expected bytes for current read operation |
| `bytes_read` | Integer | Bytes actually read so far |
| `bytes_to_write` | Integer | Total bytes to write in response |
| `bytes_written` | Integer | Bytes already written |
| `start_time` | Timestamp | Connection start time for performance measurement |

### Modification Queue
| Field | Type | Description |
|-------|------|-------------|
| `mod_queue` | Connection Pointer Array | Circular buffer for connections ready for write |
| `mod_queue_head` | Integer | Head pointer for queue operations |
| `mod_queue_tail` | Integer | Tail pointer for queue operations |
| `mod_queue_mutex` | Mutex | Thread safety for queue operations |

## Core Functionality

### 1. Initialize and Listen Sockets (`init_and_listen_sockets`)
**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Sets up complete network infrastructure including epoll instance, TCP/UDP listening sockets, cross-thread communication, and main event loop.

**Process:**
1. **Epoll Setup:**
   - Creates epoll instance for efficient event monitoring
   - Sets up event structures for socket monitoring
   - Configures edge-triggered mode for optimal performance

2. **Cross-Thread Communication:**
   - Creates eventfd for worker thread to main thread signaling
   - Adds eventfd to epoll for immediate notification
   - Sets up modification queue for write-ready connections

3. **TCP Socket Initialization:**
   - Creates non-blocking TCP listening socket
   - Binds to configured port with SO_REUSEADDR
   - Starts listening with maximum backlog
   - Adds to epoll for new connection monitoring

4. **UDP Socket Initialization:**
   - Creates non-blocking UDP socket for client communication
   - Binds to same port as TCP for protocol consistency
   - Adds to epoll for datagram monitoring

5. **Main Event Loop:**
   - Runs continuous epoll_wait loop until shutdown
   - Dispatches events to appropriate handlers
   - Handles new TCP connections, UDP datagrams, and cross-thread signals
   - Manages connection lifecycle and resource cleanup

**Performance Features:**
- **Edge-Triggered Epoll:** Minimal system calls and maximum throughput
- **Non-Blocking I/O:** Prevents blocking on slow clients
- **Event-Driven Architecture:** Scales to thousands of concurrent connections

**Used By:** Server main function, network initialization

**Dependencies:** Socket system calls, epoll interface, threading system

### 2. Handle New TCP Connection (`handle_new_tcp_connection`)
**Parameters:**
- TCP listening socket descriptor (integer)

**Returns:** None

**Purpose:** Accepts new TCP connections, configures them for non-blocking operation, and integrates them into the event monitoring system.

**Process:**
1. **Connection Acceptance Loop:**
   - Accepts multiple connections in single event notification
   - Handles EAGAIN/EWOULDBLOCK for edge-triggered operation
   - Extracts client IP address for logging and security

2. **Socket Configuration:**
   - Sets socket to non-blocking mode
   - Enables TCP keepalive with optimized parameters
   - Configures keepalive timing for connection health

3. **Connection State Setup:**
   - Allocates connection information structure
   - Initializes connection state to STATE_WANT_READ_HEADER
   - Sets up buffer management for header reading

4. **Epoll Integration:**
   - Adds new connection to epoll monitoring
   - Configures for EPOLLIN events initially
   - Sets up connection pointer for event dispatch

**Security Features:**
- **IP Address Logging:** All connections logged with source IP
- **Resource Limits:** Connection limits prevent resource exhaustion
- **Keep-Alive Management:** Automatic cleanup of dead connections

**Used By:** Main event loop TCP event handling

**Dependencies:** TCP socket operations, connection management

### 3. Handle Connection Event (`handle_connection_event`)
**Parameters:**
- Connection information pointer
- Event flags (32-bit integer)

**Returns:** None

**Purpose:** Dispatches connection events to appropriate read/write handlers based on current connection state and event type.

**Process:**
1. **Error Condition Handling:**
   - Detects EPOLLERR and EPOLLHUP conditions
   - Closes connections with network errors
   - Logs error conditions for debugging

2. **Read Event Processing:**
   - Calls read handler for EPOLLIN events
   - Manages header and body reading phases
   - Handles partial reads and connection state transitions

3. **Write Event Processing:**
   - Calls write handler for EPOLLOUT events
   - Manages response transmission to clients
   - Handles partial writes and connection completion

**State Management:**
- **Event-Driven State Machine:** Connection state determines event handling
- **Atomic State Transitions:** State changes are atomic and consistent
- **Error Recovery:** Proper state cleanup on error conditions

**Used By:** Main event loop event dispatch

**Dependencies:** Read/write handlers, connection state management

### 4. Handle Read Operations (`handle_read`)
**Parameters:**
- Connection information pointer

**Returns:** None (static function)

**Purpose:** Manages non-blocking read operations for both request headers and bodies, handling partial reads and protocol validation.

**Process:**
1. **Read State Management:**
   - **Header Phase:** Reads fixed-size protocol header
   - **Body Phase:** Reads variable-size request body
   - Handles transitions between read phases

2. **Non-Blocking Read Loop:**
   - Attempts to read remaining bytes for current phase
   - Handles EAGAIN/EWOULDBLOCK for partial reads
   - Detects connection closure by peer

3. **Protocol Validation:**
   - Validates complete header when fully received
   - Allocates body buffer based on header information
   - Validates body size limits and format

4. **Worker Thread Dispatch:**
   - Transitions to STATE_PROCESSING when request complete
   - Submits complete request to thread pool for processing
   - Removes connection from epoll monitoring during processing

**Security Features:**
- **Size Validation:** Request sizes validated against maximum limits
- **Protocol Validation:** Headers validated before body allocation
- **Resource Protection:** Dynamic allocation only after validation

**Used By:** Connection event handler for read events

**Dependencies:** Protocol validation, thread pool, memory management

### 5. Handle Write Operations (`handle_write`)
**Parameters:**
- Connection information pointer

**Returns:** None (static function)

**Purpose:** Manages non-blocking write operations for sending responses to clients, handling partial writes and connection completion.

**Process:**
1. **Write State Validation:**
   - Ensures connection is in STATE_WANT_WRITE
   - Validates write buffer and size information
   - Handles invalid state conditions gracefully

2. **Non-Blocking Write Loop:**
   - Attempts to write remaining response data
   - Handles EAGAIN/EWOULDBLOCK for partial writes
   - Tracks bytes written versus total response size

3. **Completion Handling:**
   - Detects when complete response has been sent
   - Records performance statistics
   - Closes connection after successful response

**Performance Features:**
- **Partial Write Handling:** Efficiently manages large responses
- **Zero-Copy Potential:** Optimized for kernel zero-copy when available
- **Immediate Cleanup:** Resources freed as soon as response complete

**Used By:** Connection event handler for write events

**Dependencies:** Statistics recording, connection cleanup

### 6. Cross-Thread Communication

#### Arm Socket for Write (`arm_socket_for_write`)
**Parameters:**
- Connection information pointer

**Returns:** None

**Purpose:** Thread-safe mechanism for worker threads to signal main I/O thread that a connection is ready for writing.

**Process:**
1. **Queue Management:**
   - Adds connection to modification queue in thread-safe manner
   - Handles queue overflow conditions
   - Maintains circular buffer semantics

2. **Event Signaling:**
   - Writes to eventfd to wake up main I/O thread
   - Provides immediate notification of write-ready connections
   - Handles signaling errors gracefully

**Thread Safety:**
- **Mutex Protection:** Queue operations protected by mutex
- **Atomic Signaling:** Eventfd provides atomic cross-thread signaling
- **Queue Overflow Handling:** Prevents deadlock on queue full conditions

**Used By:** Protocol layer after response preparation

**Dependencies:** Threading synchronization, eventfd operations

#### Process Write Queue (`process_write_queue`)
**Parameters:** None

**Returns:** None (static function)

**Purpose:** Processes queue of connections that worker threads have marked as ready for writing.

**Process:**
1. **Queue Draining:**
   - Dequeues all pending write-ready connections
   - Processes multiple connections per eventfd signal
   - Maintains queue consistency during processing

2. **Epoll Modification:**
   - Modifies socket registration to listen for EPOLLOUT events
   - Changes from EPOLLIN to EPOLLOUT monitoring
   - Handles epoll modification errors

3. **State Validation:**
   - Ensures connections are still in valid write state
   - Handles connections that may have been closed
   - Maintains consistent connection state

**Used By:** Main event loop when eventfd signaled

**Dependencies:** Epoll operations, connection state management

### 7. UDP Protocol Handling

#### Handle UDP Request (`handle_udp_request`)
**Parameters:**
- UDP socket descriptor (integer)

**Returns:** None

**Purpose:** Processes UDP datagrams including specialized handling for DDoS-resistant integrity protocol and regular client requests.

**Process:**
1. **Datagram Reception Loop:**
   - Receives multiple datagrams per event notification
   - Extracts client address information
   - Handles socket errors and buffer management

2. **Integrity Protocol Fast Path:**
   - Detects integrity vote requests (command ID 7)
   - Provides specialized fast processing for integrity votes
   - Bypasses normal command queue for performance

3. **Regular Request Processing:**
   - Validates protocol headers and request format
   - Allocates connection structure for UDP processing
   - Submits to thread pool for command processing

4. **Immediate Response:**
   - Sends UDP responses directly without connection state
   - Handles response formatting and addressing
   - Provides immediate resource cleanup

**Specialized Features:**
- **Integrity Fast Path:** Optimized processing for network integrity operations
- **Stateless Operation:** No connection state maintained for UDP
- **Immediate Processing:** Some operations processed without thread pool

**Used By:** Main event loop UDP event handling

**Dependencies:** Integrity system, protocol validation, thread pool

#### Handle UDP Vote Request (`handle_udp_vote_request`)
**Parameters:**
- UDP socket descriptor (integer)
- Request buffer (byte array)
- Client address structure pointer

**Returns:** None (static function)

**Purpose:** Provides ultra-fast processing of integrity vote requests, bypassing normal command processing for optimal performance.

**Process:**
1. **Root Hash Collection:**
   - Builds current root hash collection for all denominations
   - Uses cached Merkle tree roots when available
   - Handles missing roots with zero padding

2. **Comparison Operation:**
   - Compares local roots with peer's submitted roots
   - Determines match/no-match vote result
   - Provides instant consensus vote

3. **Response Generation:**
   - Creates minimal response (1 byte vote + 16 byte nonce echo)
   - Echoes peer's nonce for replay protection
   - Sends immediate UDP response

**Performance Optimization:**
- **Minimal Processing:** Optimized for sub-millisecond response
- **Cache Utilization:** Uses cached data whenever possible
- **Direct Response:** Bypasses all normal command processing

**Used By:** UDP request handler for integrity votes

**Dependencies:** Integrity system root access

### 8. Socket Configuration Functions

#### Initialize TCP Socket (`init_tcp_socket`)
**Parameters:** None

**Returns:** Socket descriptor (integer, -1 on failure)

**Purpose:** Creates and configures TCP listening socket with optimal settings for RAIDA server operation.

**Process:**
1. **Socket Creation:**
   - Creates TCP socket with appropriate protocol family
   - Sets SO_REUSEADDR for rapid server restart capability
   - Configures socket for optimal performance

2. **Non-Blocking Configuration:**
   - Sets socket to non-blocking mode for async operation
   - Validates configuration success
   - Handles configuration errors gracefully

3. **Binding and Listening:**
   - Binds to configured port on all interfaces
   - Starts listening with maximum connection backlog
   - Validates bind and listen operations

**Used By:** Network initialization

**Dependencies:** Configuration system, socket operations

#### Initialize UDP Socket (`init_udp_socket`)
**Parameters:** None

**Returns:** Socket descriptor (integer, -1 on failure)

**Purpose:** Creates and configures UDP socket for client communication and integrity protocol.

**Process:**
1. **Socket Creation:**
   - Creates UDP socket for datagram communication
   - Sets non-blocking mode for event-driven operation
   - Configures socket options for optimal performance

2. **Address Binding:**
   - Binds to same port as TCP for protocol consistency
   - Binds to all interfaces for maximum accessibility
   - Validates binding operation success

**Used By:** Network initialization

**Dependencies:** Configuration system, socket operations

### 9. Connection Management

#### Allocate Connection Info (`alloc_ci`)
**Parameters:**
- Socket descriptor (integer)

**Returns:** Connection info pointer (NULL on failure)

**Purpose:** Allocates and initializes connection information structure for new connections.

**Process:**
1. **Memory Allocation:**
   - Allocates connection structure from heap
   - Initializes all fields to safe default values
   - Sets up initial connection state

2. **State Initialization:**
   - Sets initial state to STATE_WANT_READ_HEADER
   - Configures buffer management for header reading
   - Records connection start time for statistics

3. **Resource Setup:**
   - Associates socket descriptor with connection
   - Prepares for protocol processing
   - Sets up performance measurement

**Used By:** TCP connection acceptance, UDP request processing

**Dependencies:** Memory management, timing functions

#### Free Connection Info (`free_ci`)
**Parameters:**
- Connection info pointer

**Returns:** None

**Purpose:** Safely deallocates all resources associated with a connection.

**Process:**
1. **Buffer Cleanup:**
   - Frees dynamically allocated body buffer
   - Frees response buffer if allocated
   - Releases socket address structure

2. **Memory Deallocation:**
   - Frees main connection structure
   - Nullifies pointers to prevent reuse
   - Ensures complete resource cleanup

**Used By:** Connection cleanup, error handling

**Dependencies:** Memory management

#### Close Connection (`close_connection`)
**Parameters:**
- Connection info pointer

**Returns:** None (static function)

**Purpose:** Cleanly closes connection and releases all associated resources.

**Process:**
1. **Epoll Cleanup:**
   - Removes socket from epoll monitoring
   - Handles epoll removal errors gracefully
   - Ensures no further events for this socket

2. **Socket Closure:**
   - Closes socket file descriptor
   - Removes from connection tracking array
   - Releases socket resources

3. **Resource Cleanup:**
   - Calls connection info cleanup function
   - Logs connection closure for debugging
   - Ensures no resource leaks

**Used By:** Error handling, normal connection completion

**Dependencies:** Epoll operations, resource management

### 10. Socket Utility Functions

#### Set Non-Blocking (`set_nonblocking`)
**Parameters:**
- File descriptor (integer)

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Configures socket for non-blocking operation essential for async I/O.

**Process:**
1. **Flag Retrieval:**
   - Gets current socket flags using fcntl
   - Handles flag retrieval errors
   - Validates current socket state

2. **Flag Modification:**
   - Adds O_NONBLOCK flag to existing flags
   - Sets modified flags using fcntl
   - Validates flag setting operation

**Used By:** Socket initialization, connection setup

**Dependencies:** File control operations

#### Set Blocking (`set_blocking`)
**Parameters:**
- File descriptor (integer)

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Converts socket back to blocking mode when needed for specific operations.

**Process:**
1. **Flag Retrieval:**
   - Gets current socket flags
   - Validates flag retrieval operation

2. **Flag Modification:**
   - Removes O_NONBLOCK flag from existing flags
   - Sets modified flags to enable blocking
   - Validates flag modification success

**Used By:** Specialized operations requiring blocking behavior

**Dependencies:** File control operations

## Performance Characteristics

### Scalability Features
- **Edge-Triggered Epoll:** Minimizes system calls for maximum throughput
- **Non-Blocking I/O:** Prevents blocking on slow clients
- **Connection Pooling:** Efficient reuse of connection structures
- **Zero-Copy Optimization:** Minimizes data copying where possible

### Memory Management
- **Dynamic Allocation:** Buffers allocated based on actual needs
- **Resource Tracking:** All resources tracked for proper cleanup
- **Leak Prevention:** Systematic cleanup prevents memory leaks
- **Bounded Usage:** Connection limits prevent memory exhaustion

### Network Optimization
- **TCP Keep-Alive:** Automatic detection of dead connections
- **Nagle Algorithm Management:** Optimized for RAIDA protocol characteristics
- **Buffer Sizing:** Optimal buffer sizes for typical request patterns
- **Batch Processing:** Multiple events processed per system call

## Security Considerations

### Connection Security
- **Rate Limiting Ready:** Infrastructure supports connection rate limiting
- **Resource Limits:** Maximum connections prevent resource exhaustion
- **IP Logging:** All connections logged with source IP for security analysis
- **Timeout Management:** Prevents resource holding by slow clients

### Protocol Security
- **Size Validation:** All request sizes validated before processing
- **Format Validation:** Protocol format validated before resource allocation
- **Error Handling:** Secure error handling prevents information leakage

### DDoS Protection
- **Integrity Fast Path:** Specialized handling prevents integrity protocol abuse
- **Resource Bounds:** All resource usage bounded to prevent exhaustion
- **Early Validation:** Invalid requests rejected before expensive processing

## Error Handling and Recovery

### Network Error Handling
- **Connection Failures:** Graceful handling of network connectivity issues
- **Socket Errors:** Proper handling of all socket error conditions
- **Resource Exhaustion:** Graceful degradation when resources limited

### Memory Error Handling
- **Allocation Failures:** Graceful handling of memory allocation failures
- **Buffer Overflows:** Prevention of buffer overflow conditions
- **Resource Leaks:** Systematic prevention of memory and socket leaks

### Protocol Error Handling
- **Invalid Requests:** Proper rejection of malformed requests
- **Size Violations:** Enforcement of protocol size limits
- **State Violations:** Handling of invalid connection state transitions

## Dependencies and Integration

### Required Modules
- **Protocol Layer:** Request/response processing and validation
- **Thread Pool:** Worker thread management for request processing
- **Configuration System:** Network settings and operational parameters
- **Statistics System:** Performance and operational metrics
- **Integrity System:** Merkle tree root access for fast integrity votes

### External System Dependencies
- **Epoll Interface:** Linux epoll for efficient event monitoring
- **Socket System:** TCP/UDP socket operations
- **Threading System:** POSIX threading for synchronization
- **File System:** File descriptor management

### Used By
- **Server Main:** Primary network interface for RAIDA server
- **Protocol Handlers:** Network transport for all command processing
- **Integrity System:** Network communication for healing operations
- **Administrative Tools:** Network interface for management operations

### Cross-File Dependencies
- **Protocol Module:** Request parsing and response generation
- **Configuration Module:** Network settings and server parameters
- **Statistics Module:** Performance measurement and reporting
- **Integrity Module:** Fast path integrity vote processing
- **Thread Pool:** Work distribution for request processing

## Threading and Concurrency

### Thread Architecture
- **Single I/O Thread:** Main thread handles all network I/O
- **Worker Thread Pool:** Separate threads for request processing
- **Cross-Thread Communication:** Safe communication between I/O and worker threads

### Synchronization Mechanisms
- **Eventfd:** Atomic signaling between threads
- **Mutex Protection:** Critical sections protected by mutexes
- **Lock-Free Queues:** Circular buffer for cross-thread communication
- **Connection Isolation:** Each connection processed independently

### Concurrency Control
- **Non-Blocking Operations:** I/O thread never blocks on network operations
- **State Machines:** Connection state machines prevent race conditions
- **Resource Protection:** Shared resources protected by appropriate locking

## Administrative Interface

### Monitoring and Statistics
- **Connection Tracking:** Real-time visibility into active connections
- **Performance Metrics:** Detailed timing and throughput measurements
- **Error Reporting:** Comprehensive error logging and reporting
- **Resource Usage:** Memory and socket usage monitoring

### Configuration Control
- **Port Configuration:** Configurable listening ports
- **Timeout Settings:** Adjustable timeout values for various operations
- **Connection Limits:** Configurable maximum connection counts
- **Buffer Sizes:** Tunable buffer sizes for performance optimization

This network module provides the high-performance, scalable foundation for all RAIDA server communication, supporting thousands of concurrent connections while maintaining security, reliability, and optimal performance through advanced event-driven architecture and careful resource management.