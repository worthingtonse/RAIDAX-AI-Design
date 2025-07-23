# Network Interface and Event Loop (net.c)

## Module Purpose
This module implements the core network infrastructure for RAIDA servers, providing high-performance asynchronous I/O using epoll, non-blocking socket management, cross-thread communication, UDP performance optimizations through object pooling, and support for both TCP and UDP protocols with specialized handling for the DDoS-resistant integrity protocol.

## Constants and Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_FDS` | 65535 | Maximum number of file descriptors for connection tracking |
| `MAXEPOLLSIZE` | 10000 | Maximum number of events per epoll_wait call |
| `RAIDA_EPOLL_TIMEOUT` | 10000 | Timeout for epoll_wait operations (milliseconds) |
| `MOD_QUEUE_SIZE` | 1024 | Size of modification queue for cross-thread communication |
| `MAX_BODY_SIZE` | 65536 | Maximum allowed request body size (64KB) |
| `SOCKET_TIMEOUT` | 2 | Socket timeout for operations (seconds) |
| `UDP_CI_POOL_SIZE` | 4096 | Pre-allocated connection info structs for UDP performance |

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
| `is_udp_pooled` | Boolean | Flag indicating if struct is from UDP object pool |

### Modification Queue
| Field | Type | Description |
|-------|------|-------------|
| `mod_queue` | Connection Pointer Array[MOD_QUEUE_SIZE] | Circular buffer for connections ready for write |
| `mod_queue_head` | Integer | Head pointer for queue operations |
| `mod_queue_tail` | Integer | Tail pointer for queue operations |
| `mod_queue_mutex` | Mutex | Thread safety for queue operations |

### UDP Object Pool
| Field | Type | Description |
|-------|------|-------------|
| `udp_ci_pool` | Connection Info Array[UDP_CI_POOL_SIZE] | Pre-allocated connection structures |
| `udp_ci_stack` | Connection Pointer Array[UDP_CI_POOL_SIZE] | Stack of available pool items |
| `udp_ci_stack_top` | Integer | Top of stack pointer |
| `udp_ci_pool_mutex` | Mutex | Thread safety for pool operations |

## Core Functionality

### 1. Initialize UDP Connection Info Pool (`init_udp_ci_pool`)
**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Initializes object pool for UDP connection structures to optimize performance under high traffic loads.

**Process:**
1. **Pool Initialization:**
   - Initializes pool mutex for thread-safe operations
   - Sets stack top to -1 (empty stack)
   - Prepares for pool item initialization

2. **Pre-allocation Process:**
   - For each pool slot (0 to UDP_CI_POOL_SIZE-1):
     - Pre-allocates sockaddr structure for each pool item
     - Handles allocation failures with proper cleanup
     - Adds successfully allocated items to available stack

3. **Stack Management:**
   - Pushes each initialized item onto available stack
   - Maintains stack top pointer for efficient access
   - Ensures all items are ready for immediate use

**Performance Benefits:**
- **Memory Pre-allocation:** Eliminates malloc/free overhead during request processing
- **Cache Efficiency:** Pre-allocated structures improve cache locality
- **Reduced Latency:** No allocation delays during high-traffic periods
- **Thread Safety:** Pool operations are fully thread-safe

**Used By:** Server initialization

**Dependencies:** Memory allocation, threading system

### 2. Get UDP Connection Info from Pool (`get_udp_ci_from_pool`)
**Parameters:**
- Socket descriptor (integer)

**Returns:** Connection info pointer (NULL if pool exhausted, static function)

**Purpose:** Fast, thread-safe allocation of connection structure from pre-allocated pool.

**Process:**
1. **Pool Access:**
   - Acquires pool mutex for exclusive access
   - Checks if stack is empty (stack_top == -1)
   - Returns NULL immediately if no items available

2. **Item Retrieval:**
   - Pops item from top of stack
   - Decrements stack top pointer
   - Releases pool mutex quickly to minimize contention

3. **Structure Initialization:**
   - Clears entire structure with memset for clean state
   - Sets socket descriptor
   - Records start time for performance measurement
   - Marks as UDP pooled for proper cleanup

**Performance Features:**
- **O(1) Allocation:** Constant time allocation from stack
- **Minimal Locking:** Brief mutex hold time
- **Zero Malloc:** No dynamic memory allocation during operation
- **Graceful Degradation:** Drops packets when pool exhausted

**Used By:** UDP request handling

**Dependencies:** Threading system, timing functions

### 3. Return UDP Connection Info to Pool (`return_udp_ci_to_pool`)
**Parameters:**
- Connection info pointer

**Returns:** None (static function)

**Purpose:** Returns used connection structure to pool after cleaning up dynamic allocations.

**Process:**
1. **Dynamic Memory Cleanup:**
   - Frees body buffer if allocated
   - Frees output buffer if allocated
   - Frees write buffer if allocated
   - Note: sockaddr is pre-allocated and not freed

2. **Pool Return:**
   - Acquires pool mutex for exclusive access
   - Checks for stack overflow condition
   - Pushes item back onto available stack
   - Increments stack top pointer

3. **Error Handling:**
   - Detects stack overflow conditions
   - Logs errors for debugging
   - Maintains pool integrity

**Used By:** UDP connection cleanup

**Dependencies:** Memory management, threading system

### 4. Initialize and Listen Sockets (`init_and_listen_sockets`)
**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Sets up complete network infrastructure including epoll instance, TCP/UDP listening sockets, cross-thread communication, and main event loop.

**Process:**
1. **Epoll Setup:**
   - Creates epoll instance with epoll_create1(0)
   - Sets up event structures for socket monitoring
   - Configures edge-triggered mode for optimal performance

2. **Cross-Thread Communication:**
   - Creates eventfd for worker thread to main thread signaling
   - Configures eventfd as non-blocking
   - Adds eventfd to epoll for immediate notification
   - Sets up modification queue for write-ready connections

3. **TCP Socket Initialization:**
   - Creates non-blocking TCP listening socket using init_tcp_socket
   - Configures socket options (SO_REUSEADDR)
   - Binds to configured port and starts listening
   - Adds to epoll for new connection monitoring

4. **UDP Socket Initialization:**
   - Creates non-blocking UDP socket using init_udp_socket
   - Binds to same port as TCP for protocol consistency
   - Adds to epoll for datagram monitoring

5. **Main Event Loop:**
   - Runs continuous epoll_wait loop until shutdown
   - Handles up to MAXEPOLLSIZE events per iteration
   - Uses RAIDA_EPOLL_TIMEOUT for responsiveness
   - Dispatches events to appropriate handlers

6. **Event Dispatch:**
   - **TCP Socket Events:** Calls handle_new_tcp_connection
   - **UDP Socket Events:** Calls handle_udp_request
   - **EventFD Events:** Processes write queue modifications
   - **Connection Events:** Calls handle_connection_event

7. **Cleanup:**
   - Closes all sockets during shutdown
   - Cleans up epoll and eventfd resources
   - Ensures graceful shutdown

**Performance Features:**
- **Edge-Triggered Epoll:** Minimal system calls and maximum throughput
- **Non-Blocking I/O:** Prevents blocking on slow clients
- **Event-Driven Architecture:** Scales to thousands of concurrent connections
- **Cross-Thread Optimization:** Efficient work distribution

**Used By:** Server main function, network initialization

**Dependencies:** Socket system calls, epoll interface, threading system

### 5. Handle New TCP Connection (`handle_new_tcp_connection`)
**Parameters:**
- TCP listening socket descriptor (integer)

**Returns:** None

**Purpose:** Accepts new TCP connections, configures them for non-blocking operation, and integrates them into the event monitoring system.

**Process:**
1. **Connection Acceptance Loop:**
   - Accepts multiple connections in single event notification
   - Uses while loop to handle all pending connections
   - Handles EAGAIN/EWOULDBLOCK for edge-triggered operation
   - Extracts client IP address for logging and security

2. **Socket Configuration:**
   - Sets socket to non-blocking mode using set_nonblocking
   - Enables TCP keep-alive with optimized parameters:
     - Keep-alive enabled (SO_KEEPALIVE)
     - Idle time: 60 seconds (TCP_KEEPIDLE)
     - Interval: 10 seconds (TCP_KEEPINTVL)
     - Count: 5 probes (TCP_KEEPCNT)

3. **Connection State Setup:**
   - Allocates connection information structure using alloc_ci
   - Initializes connection state to STATE_WANT_READ_HEADER
   - Sets up buffer management for header reading
   - Records client IP address

4. **Connection Tracking:**
   - Stores connection in global connections array
   - Uses file descriptor as array index
   - Enables connection lookup for event handling

5. **Epoll Integration:**
   - Adds new connection to epoll monitoring
   - Configures for EPOLLIN events initially
   - Uses edge-triggered mode (EPOLLET)
   - Sets up connection pointer for event dispatch

**Security Features:**
- **IP Address Logging:** All connections logged with source IP
- **Resource Limits:** Connection limits prevent resource exhaustion
- **Keep-Alive Management:** Automatic cleanup of dead connections
- **Error Handling:** Graceful handling of accept failures

**Used By:** Main event loop TCP event handling

**Dependencies:** TCP socket operations, connection management, epoll

### 6. Handle Connection Event (`handle_connection_event`)
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
   - Calls handle_read for EPOLLIN events
   - Manages header and body reading phases
   - Handles partial reads and connection state transitions

3. **Write Event Processing:**
   - Calls handle_write for EPOLLOUT events
   - Manages response transmission to clients
   - Handles partial writes and connection completion

**State Management:**
- **Event-Driven State Machine:** Connection state determines event handling
- **Atomic State Transitions:** State changes are atomic and consistent
- **Error Recovery:** Proper state cleanup on error conditions

**Used By:** Main event loop event dispatch

**Dependencies:** Read/write handlers, connection state management

### 7. Handle Read Operations (`handle_read`)
**Parameters:**
- Connection information pointer

**Returns:** None (static function)

**Purpose:** Manages non-blocking read operations for both request headers and bodies, handling partial reads and protocol validation.

**Process:**
1. **Read State Management:**
   - **Header Phase:** Reads fixed-size protocol header (REQUEST_HEADER_SIZE)
   - **Body Phase:** Reads variable-size request body
   - Handles transitions between read phases

2. **Non-Blocking Read Loop:**
   - Continues reading until EAGAIN/EWOULDBLOCK
   - Calculates target buffer and bytes to read
   - Uses recv with appropriate buffer offset
   - Handles connection closure by peer (bytes_read == 0)

3. **Header Completion Processing:**
   - Validates header when fully received using validate_header
   - Checks body size against MAX_BODY_SIZE limit
   - Allocates body buffer if body_size > 0
   - Transitions to body reading or processing state

4. **Body Completion Processing:**
   - Validates and decrypts body using validate_decrypt_body
   - Handles decryption and challenge validation
   - Transitions to STATE_PROCESSING

5. **Worker Thread Dispatch:**
   - Submits complete request to thread pool for processing
   - Removes connection from epoll monitoring during processing
   - Thread pool handles command execution

**Security Features:**
- **Size Validation:** Request sizes validated against maximum limits
- **Protocol Validation:** Headers validated before body allocation
- **Resource Protection:** Dynamic allocation only after validation
- **Buffer Overflow Prevention:** Careful buffer management

**Used By:** Connection event handler for read events

**Dependencies:** Protocol validation, thread pool, memory management

### 8. Handle Write Operations (`handle_write`)
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
   - Continues writing until all data sent or EAGAIN/EWOULDBLOCK
   - Calculates remaining bytes to write
   - Uses send with appropriate buffer offset
   - Tracks bytes written versus total response size

3. **Completion Handling:**
   - Detects when complete response has been sent
   - Records performance statistics using write_stat
   - Closes connection after successful response

**Performance Features:**
- **Partial Write Handling:** Efficiently manages large responses
- **Zero-Copy Potential:** Optimized for kernel zero-copy when available
- **Immediate Cleanup:** Resources freed as soon as response complete

**Used By:** Connection event handler for write events

**Dependencies:** Statistics recording, connection cleanup

### 9. Cross-Thread Communication

#### Push Modification Queue (`push_mod_queue`)
**Parameters:**
- Connection information pointer

**Returns:** None (static function)

**Purpose:** Thread-safe addition of connection to modification queue for write event registration.

**Process:**
1. **Queue Management:**
   - Acquires modification queue mutex
   - Adds connection to queue at tail position
   - Advances tail pointer with wraparound (modulo MOD_QUEUE_SIZE)
   - Detects queue overflow conditions

2. **Overflow Handling:**
   - Checks if tail would equal head (queue full)
   - Logs overflow error for debugging
   - Continues operation to prevent deadlock

3. **Thread Safety:**
   - Mutex protection ensures atomic queue operations
   - Brief critical section minimizes contention

**Used By:** arm_socket_for_write function

**Dependencies:** Threading synchronization

#### Pop Modification Queue (`pop_mod_queue`)
**Parameters:** None

**Returns:** Connection info pointer (NULL if queue empty, static function)

**Purpose:** Thread-safe removal of connection from modification queue.

**Process:**
1. **Queue Access:**
   - Acquires modification queue mutex
   - Checks if queue is empty (head == tail)
   - Returns NULL if no items available

2. **Item Removal:**
   - Retrieves connection from head position
   - Advances head pointer with wraparound
   - Returns connection to caller

3. **Thread Safety:**
   - Mutex protection ensures atomic operations
   - Handles concurrent access safely

**Used By:** process_write_queue function

**Dependencies:** Threading synchronization

#### Arm Socket for Write (`arm_socket_for_write`)
**Parameters:**
- Connection information pointer

**Returns:** None

**Purpose:** Thread-safe mechanism for worker threads to signal main I/O thread that a connection is ready for writing.

**Process:**
1. **Queue Addition:**
   - Pushes connection to modification queue
   - Handles queue overflow gracefully

2. **Event Signaling:**
   - Writes value 1 to eventfd to wake up main I/O thread
   - Provides immediate notification of write-ready connections
   - Handles signaling errors gracefully

**Thread Safety:**
- **Atomic Signaling:** Eventfd provides atomic cross-thread signaling
- **Queue Protection:** Modification queue operations are thread-safe
- **Error Handling:** Graceful handling of signaling failures

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
   - Continues until queue is empty

2. **Epoll Modification:**
   - For each connection in STATE_WANT_WRITE:
     - Modifies socket registration to listen for EPOLLOUT events
     - Changes from EPOLLIN to EPOLLOUT monitoring
     - Uses edge-triggered mode for efficiency

3. **Error Handling:**
   - Handles epoll modification errors
   - Closes connections that can't be modified
   - Maintains system stability

**Used By:** Main event loop when eventfd signaled

**Dependencies:** Epoll operations, connection state management

### 10. UDP Protocol Handling

#### Handle UDP Request (`handle_udp_request`)
**Parameters:**
- UDP socket descriptor (integer)

**Returns:** None

**Purpose:** Processes UDP datagrams including specialized handling for DDoS-resistant integrity protocol and regular client requests with object pool optimization.

**Process:**
1. **Datagram Reception Loop:**
   - Receives multiple datagrams per event notification
   - Allocates buffer up to udp_payload_threshold size
   - Extracts client address information
   - Handles socket errors and buffer management

2. **Statistics Update:**
   - Increments request counter for monitoring
   - Tracks UDP traffic statistics

3. **Integrity Protocol Fast Path:**
   - Detects integrity vote requests (command ID 7)
   - Calculates expected vote request size: 1 + (TOTAL_DENOMINATIONS * HASH_SIZE) + 16
   - Provides specialized fast processing for integrity votes
   - Bypasses normal command queue for performance

4. **Regular Request Processing with Object Pool:**
   - Gets connection structure from UDP object pool
   - Handles pool exhaustion by dropping packets gracefully
   - Copies client address to pre-allocated sockaddr structure
   - Records client IP for logging

5. **Protocol Validation:**
   - Validates minimum header size (REQUEST_HEADER_SIZE)
   - Calls validate_header for protocol compliance
   - Sends error responses for invalid requests

6. **Body Processing:**
   - Allocates body buffer for requests with body data
   - Copies body data from UDP packet
   - Calls validate_decrypt_body for protocol compliance

7. **Thread Pool Submission:**
   - Submits valid requests to worker thread pool
   - Provides immediate resource cleanup for UDP
   - Ensures efficient processing

**Specialized Features:**
- **Integrity Fast Path:** Optimized processing for network integrity operations
- **Object Pool Optimization:** Pre-allocated structures for high performance
- **Stateless Operation:** No persistent connection state for UDP
- **Immediate Processing:** Fast path operations processed without thread pool

**Used By:** Main event loop UDP event handling

**Dependencies:** Integrity system, protocol validation, thread pool, object pool

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
   - Uses get_merkle_root function for each denomination
   - Handles missing roots with zero padding for consistency

2. **Comparison Operation:**
   - Compares local roots with peer's submitted roots
   - Performs memcmp on complete root collection
   - Determines match/no-match vote result

3. **Response Generation:**
   - Creates minimal response (1 byte vote + 16 byte nonce echo)
   - Sets vote result (1 for match, 0 for no match)
   - Echoes peer's nonce for replay protection

4. **Immediate Response:**
   - Sends UDP response directly using sendto
   - No connection state or complex processing
   - Provides sub-millisecond response time

**Performance Optimization:**
- **Minimal Processing:** Optimized for sub-millisecond response
- **Cache Utilization:** Uses cached Merkle tree roots when available
- **Direct Response:** Bypasses all normal command processing
- **Zero Allocation:** No dynamic memory allocation

**Used By:** UDP request handler for integrity votes

**Dependencies:** Integrity system root access, network operations

### 11. Socket Configuration Functions

#### Initialize TCP Socket (`init_tcp_socket`)
**Parameters:** None

**Returns:** Socket descriptor (integer, -1 on failure)

**Purpose:** Creates and configures TCP listening socket with optimal settings for RAIDA server operation.

**Process:**
1. **Socket Creation:**
   - Creates TCP socket using socket(AF_INET, SOCK_STREAM, 0)
   - Handles socket creation failures

2. **Socket Options:**
   - Sets SO_REUSEADDR for rapid server restart capability
   - Validates socket option setting

3. **Non-Blocking Configuration:**
   - Sets socket to non-blocking mode using set_nonblocking
   - Ensures async operation compatibility

4. **Address Binding:**
   - Binds to configured port on all interfaces (INADDR_ANY)
   - Uses network byte order for port
   - Handles binding failures

5. **Listen Setup:**
   - Starts listening with SOMAXCONN backlog
   - Maximizes connection queue size
   - Validates listen operation

**Used By:** Network initialization

**Dependencies:** Configuration system, socket operations

#### Initialize UDP Socket (`init_udp_socket`)
**Parameters:** None

**Returns:** Socket descriptor (integer, -1 on failure)

**Purpose:** Creates and configures UDP socket for client communication and integrity protocol.

**Process:**
1. **Socket Creation:**
   - Creates UDP socket using socket(AF_INET, SOCK_DGRAM, 0)
   - Handles socket creation failures

2. **Non-Blocking Configuration:**
   - Sets socket to non-blocking mode using set_nonblocking
   - Ensures event-driven operation

3. **Address Binding:**
   - Binds to same port as TCP for protocol consistency
   - Binds to all interfaces for maximum accessibility
   - Uses network byte order for port

**Used By:** Network initialization

**Dependencies:** Configuration system, socket operations

### 12. Connection Management

#### Allocate Connection Info (`alloc_ci`)
**Parameters:**
- Socket descriptor (integer)

**Returns:** Connection info pointer (NULL on failure)

**Purpose:** Allocates and initializes connection information structure for new TCP connections.

**Process:**
1. **Memory Allocation:**
   - Allocates connection structure from heap
   - Handles allocation failures gracefully

2. **Structure Initialization:**
   - Clears entire structure with memset
   - Sets socket descriptor
   - Initializes state to STATE_WANT_READ_HEADER
   - Sets bytes_to_read to REQUEST_HEADER_SIZE

3. **Timing Setup:**
   - Records connection start time using gettimeofday
   - Enables performance measurement

**Used By:** TCP connection acceptance

**Dependencies:** Memory management, timing functions

#### Free Connection Info (`free_ci`)
**Parameters:**
- Connection info pointer

**Returns:** None

**Purpose:** Safely deallocates connection resources with differentiation between pooled and non-pooled structures.

**Process:**
1. **Structure Type Detection:**
   - Checks is_udp_pooled flag to determine structure type
   - Handles pooled UDP structures differently from TCP structures

2. **UDP Pooled Structure Handling:**
   - Returns structure to UDP object pool
   - Pool handles internal cleanup automatically

3. **TCP Structure Handling:**
   - Frees sockaddr structure if allocated
   - Frees body buffer if allocated
   - Frees output buffer if allocated
   - Frees write buffer if allocated
   - Frees main connection structure

**Used By:** Connection cleanup, error handling

**Dependencies:** Memory management, object pool

#### Close Connection (`close_connection`)
**Parameters:**
- Connection info pointer

**Returns:** None (static function)

**Purpose:** Cleanly closes connection and releases all associated resources.

**Process:**
1. **Validation:**
   - Checks connection pointer is not NULL
   - Returns immediately if NULL

2. **Epoll Cleanup:**
   - Removes socket from epoll monitoring using EPOLL_CTL_DEL
   - Handles epoll removal errors gracefully

3. **Socket Closure:**
   - Closes socket file descriptor
   - Releases socket resources

4. **Connection Tracking Cleanup:**
   - Removes from global connections array if valid index
   - Clears array entry to prevent stale references

5. **Resource Cleanup:**
   - Calls free_ci to release connection structure
   - Ensures no resource leaks

**Used By:** Error handling, normal connection completion

**Dependencies:** Epoll operations, resource management

### 13. Socket Utility Functions

#### Set Non-Blocking (`set_nonblocking`)
**Parameters:**
- File descriptor (integer)

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Configures socket for non-blocking operation essential for async I/O.

**Process:**
1. **Flag Retrieval:**
   - Gets current socket flags using fcntl(F_GETFL)
   - Handles flag retrieval errors
   - Validates current socket state

2. **Flag Modification:**
   - Adds O_NONBLOCK flag to existing flags using bitwise OR
   - Sets modified flags using fcntl(F_SETFL)
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
   - Gets current socket flags using fcntl(F_GETFL)
   - Validates flag retrieval operation

2. **Flag Modification:**
   - Removes O_NONBLOCK flag using bitwise AND with complement
   - Sets modified flags to enable blocking
   - Validates flag modification success

**Used By:** Specialized operations requiring blocking behavior

**Dependencies:** File control operations

## Performance Characteristics

### Scalability Features
- **Edge-Triggered Epoll:** Minimizes system calls for maximum throughput
- **Non-Blocking I/O:** Prevents blocking on slow clients
- **UDP Object Pool:** Eliminates malloc/free overhead for UDP requests
- **Connection Pooling:** Efficient reuse of connection structures
- **Zero-Copy Optimization:** Minimizes data copying where possible

### Memory Management
- **Dynamic Allocation:** Buffers allocated based on actual needs
- **Object Pool Optimization:** Pre-allocated UDP structures for performance
- **Resource Tracking:** All resources tracked for proper cleanup
- **Leak Prevention:** Systematic cleanup prevents memory leaks
- **Bounded Usage:** Connection limits prevent memory exhaustion

### Network Optimization
- **TCP Keep-Alive:** Automatic detection of dead connections
- **Nagle Algorithm Management:** Optimized for RAIDA protocol characteristics
- **Buffer Sizing:** Optimal buffer sizes for typical request patterns
- **Batch Processing:** Multiple events processed per epoll_wait call
- **Connection Reuse:** Efficient handling of persistent connections

## Security Considerations

### Connection Security
- **Rate Limiting Ready:** Infrastructure supports connection rate limiting
- **Resource Limits:** Maximum connections prevent resource exhaustion
- **IP Logging:** All connections logged with source IP for security analysis
- **Timeout Management:** Prevents resource holding by slow clients
- **Keep-Alive Protection:** Automatic cleanup of stale connections

### Protocol Security
- **Size Validation:** All request sizes validated before processing
- **Format Validation:** Protocol format validated before resource allocation
- **Error Handling:** Secure error handling prevents information leakage
- **Buffer Overflow Prevention:** Careful buffer management prevents overflows

### DDoS Protection
- **Integrity Fast Path:** Specialized handling prevents integrity protocol abuse
- **Resource Bounds:** All resource usage bounded to prevent exhaustion
- **Early Validation:** Invalid requests rejected before expensive processing
- **Pool Exhaustion Handling:** Graceful degradation when UDP pool exhausted

## Error Handling and Recovery

### Network Error Handling
- **Connection Failures:** Graceful handling of network connectivity issues
- **Socket Errors:** Proper handling of all socket error conditions
- **Resource Exhaustion:** Graceful degradation when resources limited
- **Pool Exhaustion:** Packet dropping when UDP pool exhausted

### Memory Error Handling
- **Allocation Failures:** Graceful handling of memory allocation failures
- **Buffer Overflows:** Prevention of buffer overflow conditions
- **Resource Leaks:** Systematic prevention of memory and socket leaks
- **Pool Management:** Proper object pool lifecycle management

### Protocol Error Handling
- **Invalid Requests:** Proper rejection of malformed requests
- **Size Violations:** Enforcement of protocol size limits
- **State Violations:** Handling of invalid connection state transitions
- **Timeout Handling:** Proper cleanup of timed-out operations

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
- **EventFD:** Cross-thread signaling mechanism

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
- **EventFD:** Atomic signaling between threads
- **Mutex Protection:** Critical sections protected by mutexes
- **Lock-Free Queues:** Circular buffer for cross-thread communication
- **Connection Isolation:** Each connection processed independently

### Concurrency Control
- **Non-Blocking Operations:** I/O thread never blocks on network operations
- **State Machines:** Connection state machines prevent race conditions
- **Resource Protection:** Shared resources protected by appropriate locking
- **Pool Synchronization:** Thread-safe object pool operations

## Administrative Interface

### Monitoring and Statistics
- **Connection Tracking:** Real-time visibility into active connections
- **Performance Metrics:** Detailed timing and throughput measurements
- **Error Reporting:** Comprehensive error logging and reporting
- **Resource Usage:** Memory and socket usage monitoring
- **Pool Utilization:** UDP object pool usage statistics

### Configuration Control
- **Port Configuration:** Configurable listening ports
- **Timeout Settings:** Adjustable timeout values for various operations
- **Connection Limits:** Configurable maximum connection counts
- **Buffer Sizes:** Tunable buffer sizes for performance optimization
- **Pool Size:** Configurable UDP object pool size

This network module provides the high-performance, scalable foundation for all RAIDA server communication, supporting thousands of concurrent connections while maintaining security, reliability, and optimal performance through advanced event-driven architecture, UDP object pool optimizations, and careful resource management.