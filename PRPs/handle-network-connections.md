<!-- 
#  Specification:  Network Layer Implementation

## 1. Module Purpose
This module implements a high-performance, event-driven network layer for handling both TCP and UDP connections in the CloudCoin RAIDA system. It uses an epoll-based event loop with a thread pool for request processing, supporting thousands of concurrent connections while maintaining low latency and high throughput.

## 2. System Architecture Overview

### 2.1 Core Components
- **Event-Driven I/O**: Single-threaded epoll event loop for maximum efficiency
- **Dual Protocol Support**: Simultaneous TCP and UDP handling on the same port
- **Thread Pool Integration**: Worker threads for CPU-intensive request processing
- **Cross-Thread Communication**: EventFD-based signaling between I/O and worker threads
- **Connection State Management**: Finite state machine for TCP connection lifecycle

### 2.2 Performance Characteristics
- **Non-Blocking I/O**: All socket operations are non-blocking for scalability
- **Edge-Triggered Events**: Epoll configured for edge-triggered mode for efficiency
- **Connection Tracking**: Global array for managing active connections by file descriptor

## 3. System Constants and Configuration

### 3.1 Network Configuration
```
RAIDA_EPOLL_TIMEOUT = 10000 milliseconds
MAXEPOLLSIZE = 10000 events
MAX_FDS = 65535 file descriptors
MAX_BODY_SIZE = 65536 bytes
SOCKET_TIMEOUT = 2 seconds
MOD_QUEUE_SIZE = 1024 connections
```

### 3.2 TCP Keep-Alive Settings
```
keepalive = 1 (enabled)
keep_idle = 60 seconds
keep_interval = 10 seconds  
keep_count = 5 probes
```

## 4. Data Structures

### 4.1 Connection Information Structure
Each connection must track the following state information:
- Socket file descriptor
- Client IP address (15 character string plus null terminator)
- Connection start timestamp
- Current connection state (enumerated value)
- Read buffer for request headers (fixed size: REQUEST_HEADER_SIZE)
- Bytes read and bytes expected for current read operation
- Dynamically allocated body buffer and body size
- Write buffer, output buffer, and write progress tracking
- Socket address structure (for UDP connections only)
- Protocol header fields: session_id, command, version, raida_id, reserved

### 4.2 Connection State Machine
Four distinct states manage the TCP connection lifecycle:
- STATE_WANT_READ_HEADER: Waiting to read complete request header
- STATE_WANT_READ_BODY: Waiting to read complete request body  
- STATE_PROCESSING: Request being processed by worker thread
- STATE_WANT_WRITE: Ready to write response data

### 4.3 Global Management Structures
- Connection array indexed by file descriptor (size MAX_FDS)
- Circular modification queue for cross-thread write signaling
- Queue head and tail indices with mutex protection
- Global epoll file descriptor and eventfd for cross-thread signaling

## 5. Core Network Operations

### 5.1 System Initialization
The main initialization function must:
- Create epoll instance for event management
- Create eventfd for cross-thread communication with non-blocking flag
- Initialize TCP listening socket with stream protocol
- Initialize UDP socket with datagram protocol
- Add all file descriptors to epoll with edge-triggered input events
- Enter main event loop processing until shutdown signal

### 5.2 TCP Socket Configuration
TCP socket setup requires:
- Create socket with IPv4 address family and stream type
- Enable SO_REUSEADDR socket option for address reuse
- Configure non-blocking mode using file control flags
- Bind to any address (INADDR_ANY) on configured port
- Start listening with maximum connection backlog (SOMAXCONN)

### 5.3 UDP Socket Configuration  
UDP socket setup requires:
- Create socket with IPv4 address family and datagram type
- Configure non-blocking mode using file control flags
- Bind to any address (INADDR_ANY) on configured port

## 6. Event Loop Processing

### 6.1 Main Event Loop
The central event loop must:
- Wait for events using epoll with configured timeout
- Handle interrupted system calls by continuing loop
- Process each returned event based on file descriptor type
- Route TCP listening socket events to new connection handler
- Route UDP socket events to datagram request handler
- Route eventfd events to write queue processor
- Route client connection events to connection event handler

### 6.2 TCP Connection Acceptance
New TCP connection handling requires:
- Accept all pending connections in loop until EAGAIN/EWOULDBLOCK
- Configure each accepted socket for non-blocking operation
- Apply TCP keep-alive settings with specified parameters
- Extract and store client IP address from connection info
- Allocate and initialize connection information structure
- Add client socket to epoll for input events with edge triggering

### 6.3 Connection Event Processing
Connection event routing must:
- Check for error events (EPOLLERR, EPOLLHUP) and close connection
- Process read events when EPOLLIN flag is set
- Process write events when EPOLLOUT flag is set

## 7. TCP Data Transfer Management

### 7.1 Read Event Handling
Read processing implements a state machine:
- For STATE_WANT_READ_HEADER: Read into header buffer until complete
- For STATE_WANT_READ_BODY: Read into body buffer until complete
- Handle EAGAIN/EWOULDBLOCK by returning to wait for more data
- Handle connection close (zero bytes read) by cleaning up connection
- Validate header when complete and transition to body read if needed
- Validate and decrypt body when complete
- Submit complete requests to thread pool for processing
- Handle oversized body requests by sending error response

### 7.2 Write Event Handling
Write processing must:
- Only process connections in STATE_WANT_WRITE
- Send data from write buffer starting at current write position
- Handle EAGAIN/EWOULDBLOCK by waiting for next write event
- Update bytes written counter after each successful send
- Close connection when all response data has been sent

## 8. UDP Request Processing

### 8.1 Datagram Handling
UDP request processing requires:
- Read all pending datagrams in loop until EAGAIN/EWOULDBLOCK
- Allocate temporary buffer using configured UDP payload threshold
- Extract client address information from recvfrom operation
- Validate minimum packet size against REQUEST_HEADER_SIZE
- Create temporary connection information for request processing
- Validate header and extract body if present
- Process request immediately without state machine (synchronous)
- Send error responses for malformed or oversized requests

## 9. Cross-Thread Communication

### 9.1 Write Queue Management
Thread-safe queue operations:
- Push operation adds connection to tail of circular queue
- Pop operation removes connection from head of circular queue
- Mutex protection around all queue modifications
- Queue overflow detection when tail catches up to head

### 9.2 Write Signaling Mechanism
Worker threads signal main I/O thread by:
- Adding connection to modification queue
- Writing 64-bit value to eventfd to wake epoll loop
- Main thread processes eventfd events by clearing value and processing queue

### 9.3 Write Event Arming
Queue processing updates epoll registration:
- Extract all queued connections requiring write events
- Modify epoll registration to listen for EPOLLOUT events
- Use edge-triggered mode for write event notification

## 10. Socket Configuration Utilities

### 10.1 Blocking Mode Control
Non-blocking configuration:
- Get current file descriptor flags
- Set O_NONBLOCK flag while preserving other flags

Blocking configuration:
- Get current file descriptor flags  
- Clear O_NONBLOCK flag while preserving other flags

## 11. Connection Lifecycle Management

### 11.1 Connection Allocation
Connection structure initialization:
- Allocate memory for connection information structure
- Initialize all fields to zero
- Set socket file descriptor
- Set initial state to STATE_WANT_READ_HEADER
- Set bytes to read to REQUEST_HEADER_SIZE
- Record current timestamp as start time

### 11.2 Connection Cleanup
Resource deallocation sequence:
- Free socket address structure if allocated (UDP connections)
- Free body buffer if allocated
- Free output buffer if allocated  
- Free write buffer if allocated
- Free main connection structure

### 11.3 Connection Termination
Connection closure process:
- Remove socket from epoll monitoring
- Close socket file descriptor
- Clear connection array entry
- Free all associated resources

## 12. Error Handling and Recovery

### 12.1 Network Error Management
- Socket creation failures: Log error and return failure code
- Binding failures: Log error with system error details
- Accept failures: Distinguish between EAGAIN and actual errors
- Read/write failures: Handle EAGAIN separately from connection errors
- Connection drops: Clean up resources and continue serving others

### 12.2 Resource Error Management
- Memory allocation failures: Send error response and clean up
- File descriptor exhaustion: Log error and reject new connections
- Queue overflow: Log error and continue processing
- Protocol validation failures: Send appropriate error codes

### 12.3 Security Error Handling
- Oversized body requests: Reject with error response
- Invalid header validation: Send error and close connection
- Buffer overflow attempts: Enforce strict bounds checking
- Malformed packets: Reject with minimal processing

## 13. Performance Optimization Strategies

### 13.1 I/O Efficiency
- Edge-triggered epoll events minimize system calls
- Non-blocking operations prevent thread blocking
- Batch event processing reduces context switching overhead
- Direct buffer operations minimize memory copying

### 13.2 Memory Management
- Fixed-size header buffers avoid repeated allocation
- Dynamic body buffers allocated only when needed
- Connection structures reused through allocation pool
- Resource cleanup on all exit paths prevents leaks

### 13.3 Thread Coordination
- Single I/O thread eliminates lock contention in event loop
- Worker threads operate on isolated connection structures
- EventFD provides efficient cross-thread signaling
- Modification queue batches write event updates

## 14. Integration Requirements

### 14.1 External Function Dependencies
- validate_header: Validates request header and extracts fields
- validate_decrypt_body: Validates and decrypts request body
- run_command: Processes complete request in worker thread
- send_command_error: Sends error response to client
- thpool_add_work: Submits work to thread pool
- thpool_num_threads_working: Returns active worker thread count

### 14.2 Configuration Dependencies
- config.port: Network port for listening sockets
- config.udp_payload_threshold: Maximum UDP packet size
- is_finished: Global shutdown signal flag

### 14.3 Protocol Constants
- REQUEST_HEADER_SIZE: Size of protocol request header
- Error codes: NO_ERROR, ERROR_INVALID_PACKET_LENGTH, ERROR_MEMORY_ALLOC

## 15. Security and Resource Protection

### 15.1 Input Validation
- Request header validation before processing
- Body size limits enforced against MAX_BODY_SIZE
- Packet size validation for UDP requests
- Buffer bounds checking on all operations

### 15.2 Resource Limits
- Maximum concurrent connections limited by MAX_FDS
- Memory usage limited by body size restrictions
- Connection timeouts prevent resource exhaustion
- Queue sizes prevent unbounded memory growth

### 15.3 Attack Mitigation
- Malformed request rejection with minimal processing
- Buffer overflow prevention through size validation
- Connection flooding protection through limits
- Memory exhaustion protection through size limits

This specification provides complete implementation guidance for a high-performance, event-driven network layer while remaining language-agnostic and focusing only on the actual functionality present in the source code. -->



# Network Layer Implementation (net)

## Module Purpose
This module implements a high-performance, event-driven network layer for the RAIDA server supporting both TCP and UDP protocols. It provides non-blocking I/O operations, connection management, cross-thread communication, and efficient request processing using epoll-based event notification. The system handles thousands of concurrent connections while maintaining low latency and high throughput.

## Core Architecture

### Event-Driven Design
- **Main I/O Thread:** Single thread handling all network events using epoll (Single thread handling all network events using a platform-level event-notification mechanism (e.g., epoll on Linux, kqueue on BSD/macOS, IOCP on Windows))
- **Worker Thread Pool:** Multiple threads processing requests concurrently
- **Cross-Thread Communication:** EventFD-based signaling for write completion
- **Non-Blocking Operations:** All socket operations use non-blocking mode

### Connection State Management
- **State Machine:** Tracks connection progress through read/write phases
- **Buffer Management:** Dynamic allocation for variable-sized requests/responses
- **Resource Tracking:** Per-connection resource management and cleanup


### Each active client connection is tracked using a dedicated Connection State object, which encapsulates all relevant information for request handling, buffering, and lifecycle management.

### This structure supports both TCP and UDP clients, and its fields are used to manage read/write phases, socket identity, and connection-specific data.

Fields:
socket_id (Integer):
Unique identifier for the connection’s socket (used to perform read/write operations).

state (Enum or String):
Current state of the connection in its lifecycle. Expected values:

"READ_HEADER" – Reading the fixed-length header.

"READ_BODY" – Reading the optional body payload.

"PROCESSING" – Request is being processed by a worker thread.

"WRITE" – Response is ready to be sent back.

header_buffer (Byte array):
Fixed-length buffer to hold the incoming request header. Size defined by protocol (e.g. 20–32 bytes).

body_buffer (Byte array or null):
Optional dynamically allocated buffer to hold the request body. Only present if the protocol indicates a body is expected.

bytes_read_total (Integer):
Total number of bytes received so far (across header and/or body).

bytes_expected_total (Integer):
Total number of bytes expected to be received (determined after reading the header).

bytes_written_total (Integer):
Number of bytes of the response already sent to the client.

bytes_to_write_total (Integer):
Total number of bytes that must be written to complete the response.

response_buffer (Byte array):
Holds the full response (header + body) ready to be sent to the client.

client_ip_address (String):
Textual IP address of the remote client, used for logging or access control.

udp_client_address (Object or null):
For UDP requests, this field holds the client’s socket address for response routing.

created_timestamp (Timestamp):
The time at which this connection was established. Used for timeout cleanup logic.


### network config

| Parameter                 | Value         | Description                                                                                         |
| ------------------------- | ------------- | --------------------------------------------------------------------------------------------------- |
| Network event timeout     | 10 000 ms     | Maximum wait time for the event-notification mechanism (e.g., epoll, kqueue, select, IOCP, asyncio) |
| Max simultaneous events   | 10 000 events | Maximum number of concurrent file-descriptor events supported                                       |
| Max open file descriptors | 65 535        | Upper limit for file-descriptor handles                                                             |
| Max request body size     | 65 536 bytes  | Largest allowed request payload                                                                     |
| Socket inactivity timeout | 2 seconds     | Max idle time for inactive sockets                                                                  |
| Modification queue size   | 1 024 conns   | Capacity for sockets waiting to be re-armed for writing                                             |


### TCP Keep‑Alive Policy
Enabled (keepalive = 1)

Initial delay: 60 s of inactivity before probes begin

Probe interval: every 10 s once inactivity threshold reached

Max probes: 5 before the socket is declared dead

## Core Functionality

### 1. Main Network Entry Point (`init_and_listen_sockets`)
**Parameters:** None

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Initializes network infrastructure and runs main event loop until server shutdown.

**Process:**

#### Infrastructure Setup
1. **Epoll Initialization:**
   - Creates epoll instance for event notification (Initializes the event-notification system (e.g., epoll, kqueue, or similar depending on platform))
   - Sets up event queue for handling multiple concurrent connections (Prepares an event queue for handling multiple concurrent connections using edge-triggered or level-triggered notifications)

2. **EventFD Creation:**
   - Creates eventfd for cross-thread signaling between worker threads and I/O thread
   - Configures non-blocking mode for efficient signaling
   - Adds eventfd to epoll for monitoring write completion signals

3. **Socket Initialization:**
   - Creates and configures TCP listening socket
   - Creates and configures UDP socket for connectionless operations
   - Adds both sockets to epoll for event monitoring

#### Main Event Loop
4. **Event Processing:**
   - Runs continuous loop until server shutdown signal
   - Waits for events using epoll_wait with configurable timeout (Waits for incoming socket events using the system’s event polling mechanism (e.g., epoll_wait on Linux, select/poll on legacy systems))
   - Processes multiple events per iteration for efficiency (Supports batching of multiple socket events per loop iteration for efficiency)

5. **Event Dispatching:**
   - **TCP Socket Events:** New connection acceptance
   - **UDP Socket Events:** Connectionless request processing
   - **EventFD Events:** Write completion notifications from worker threads
   - **Connection Events:** Read/write operations on established connections

6. **Shutdown Handling:**
   - Closes all listening sockets gracefully
   - Cleans up epoll instance and eventfd
   - Returns control to main server shutdown sequence

**Dependencies:**
- Socket initialization functions for TCP and UDP
- Connection handling functions for established connections
- Cross-thread communication system for write operations

### 2. TCP Socket Management (`init_tcp_socket`)
**Parameters:** None

**Returns:** Integer socket file descriptor (negative on error)

**Purpose:** Creates and configures TCP listening socket for client connections.

**Process:**
1. **Socket Creation:**
   - Creates TCP socket using SOCK_STREAM type
   - Enables SO_REUSEADDR for quick server restart capability

2. **Non-Blocking Configuration:**
   - Sets socket to non-blocking mode for event-driven operation
   - Prevents blocking on accept operations

3. **Binding and Listening:**
   - Binds to configured port on all interfaces (INADDR_ANY)
   - Sets listen backlog to SOMAXCONN for maximum connection queue

4. **Error Handling:**
   - Validates each configuration step
   - Cleans up on failure with appropriate error reporting

**Security Features:**
- Configurable listening address (currently all interfaces)
- Maximum connection backlog to prevent resource exhaustion
- Proper error handling prevents information leakage

### 3. UDP Socket Management (`init_udp_socket`)
**Parameters:** None

**Returns:** Integer socket file descriptor (negative on error)

**Purpose:** Creates and configures UDP socket for connectionless request processing.

**Process:**
1. **Socket Creation:**
   - Creates UDP socket using SOCK_DGRAM type
   - Configures for connectionless operation

2. **Non-Blocking Configuration:**
   - Sets non-blocking mode for event-driven processing
   - Prevents blocking on receive operations

3. **Binding:**
   - Binds to same port as TCP socket
   - Enables dual-protocol support on single port

**Performance Features:**
- Single socket handles all UDP traffic
- Non-blocking operation prevents thread starvation
- Efficient event notification through epoll

### 4. TCP Connection Handling (`handle_new_tcp_connection`)
**Parameters:**
- TCP listening socket file descriptor (integer)

**Returns:** None

**Purpose:** Accepts and configures new TCP connections with proper initialization.

**Process:**
1. **Connection Acceptance Loop:**
   - Accepts multiple connections per event notification
   - Handles EAGAIN/EWOULDBLOCK for non-blocking operation
   - Continues until no more pending connections

2. **Connection Configuration:**
   - Sets accepted socket to non-blocking mode
   - Configures TCP keep-alive parameters:
     - keep-alive enabled with 60-second idle timeout
     - 10-second interval between keep-alive probes
     - 5 probe count before connection declared dead

3. **Connection Information Setup:**
   - Allocates connection information structure
   - Extracts and stores client IP address
   - Initializes connection state for header reading

4. **Epoll Registration:**
   - Adds new connection to epoll for read event monitoring (Registers the new connection with the event dispatcher to receive read event notifications)
   - Configures edge-triggered mode for efficiency (Uses edge-triggered or equivalent optimized notification mode (e.g., EPOLLET in epoll))
   - Handles registration failures with connection cleanup

**Security Features:**
- TCP keep-alive prevents resource exhaustion from idle connections
- Client IP address logging for audit and security monitoring
- Proper resource cleanup on configuration failures

### 5. Connection Event Processing (`handle_connection_event`)
**Parameters:**
- Connection information structure pointer
- Event mask (32-bit unsigned integer indicating event types)

**Returns:** None

**Purpose:** Routes connection events to appropriate read/write handlers based on event type.

**Process:**
1. **Error Event Handling:**
   - Detects EPOLLERR, EPOLLHUP, and invalid event combinations
   - Closes connections with error conditions
   - Logs error details for troubleshooting

2. **Read Event Processing:**
   - Calls read handler for EPOLLIN events
   - Handles partial reads and connection state transitions
   - Manages buffer allocation for request data

3. **Write Event Processing:**
   - Calls write handler for EPOLLOUT events
   - Handles partial writes and response transmission
   - Manages connection cleanup after response completion

**Thread Safety:** Called only from main I/O thread, no synchronization required

### 6. Read Operation Handling (`handle_read`)
**Parameters:**
- Connection information structure pointer

**Returns:** None (static function, modifies connection state)

**Purpose:** Handles reading request data from TCP connections with state management.

**Process:**

#### Header Reading Phase
1. **Header Buffer Management:**
   - Reads into fixed-size header buffer
   - Tracks bytes read vs. bytes expected (REQUEST_HEADER_SIZE)
   - Handles partial reads across multiple events

2. **Header Validation:**
   - Validates complete header using protocol validation
   - Extracts body size from header fields
   - Validates body size against MAX_BODY_SIZE limit

3. **State Transition:**
   - Transitions to body reading if body size > 0
   - Submits to thread pool if no body required
   - Allocates body buffer for expected body size

#### Body Reading Phase
4. **Body Buffer Management:**
   - Reads into dynamically allocated body buffer
   - Tracks body bytes read vs. expected body size
   - Handles partial body reads across events

5. **Body Validation:**
   - Validates and decrypts complete body using protocol functions
   - Handles encryption and integrity verification
   - Prepares request for worker thread processing

6. **Request Submission:**
   - Submits complete request to thread pool for processing
   - Changes connection state to processing
   - Transfers control to worker thread

**Error Handling:**
- Network errors close connection with logging
- Protocol errors send error response before closing
- Resource allocation failures handled gracefully

### 7. Write Operation Handling (`handle_write`)
**Parameters:**
- Connection information structure pointer

**Returns:** None (static function, modifies connection state)

**Purpose:** Handles writing response data to TCP connections.

**Process:**
1. **State Validation:**
   - Ensures connection is in write state
   - Validates write buffer and size information

2. **Data Transmission:**
   - Sends response data using non-blocking send()
   - Handles partial writes by updating buffer position
   - Continues until all data transmitted or error occurs

3. **Completion Handling:**
   - Closes connection when all data transmitted
   - Handles write errors with connection cleanup
   - Updates statistics for completed requests

**Performance Features:**
- Efficient partial write handling
- Non-blocking operation prevents thread blocking
- Automatic connection cleanup after response

### 8. UDP Request Processing (`handle_udp_request`)
**Parameters:**
- UDP socket file descriptor (integer)

**Returns:** None

**Purpose:** Processes UDP datagrams with immediate response capability.

**Process:**
1. **Datagram Reception Loop:**
   - Receives multiple datagrams per event notification
   - Allocates buffer for each datagram up to threshold size
   - Extracts client address for response routing

2. **Request Validation:**
   - Validates minimum packet size requirements
   - Performs header validation using protocol functions
   - Validates body size and extracts body data

3. **Body Processing:**
   - Allocates and populates body buffer if present
   - Validates and decrypts body using protocol functions
   - Handles encryption and integrity verification

4. **Immediate Processing:**
   - Processes UDP requests immediately (no thread pool delay)
   - Calls run_command directly for request execution
   - Sends response immediately using sendto()

**Performance Characteristics:**
- Lower latency than TCP (no connection establishment)
- Immediate processing without thread pool overhead
- Efficient for small, stateless requests

### 9. Cross-Thread Communication (`arm_socket_for_write`)
**Parameters:**
- Connection information structure pointer

**Returns:** None

**Purpose:** Signals main I/O thread that worker thread has completed request processing.

**Process:**
1. **Queue Management:**
   - Adds connection to modification queue using thread-safe operations
   - Uses circular buffer with head/tail pointers
   - Handles queue overflow conditions

2. **I/O Thread Signaling:**
   - Writes to eventfd to wake up main I/O thread
   - Triggers immediate processing of write queue
   - Enables efficient cross-thread communication

3. **Write Queue Processing:**
   - Main I/O thread processes queued connections
   - Modifies epoll registration to monitor EPOLLOUT events
   - Enables write operation handling for completed requests

**Thread Safety:**
- Mutex-protected queue operations
- Atomic eventfd signaling
- Lock-free I/O thread processing

### 10. Socket Configuration Utilities

#### `set_nonblocking`
**Parameters:**
- File descriptor (integer socket)

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Configures socket for non-blocking operation using fcntl.

#### `set_blocking`
**Parameters:**
- File descriptor (integer socket)

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Reverts socket to blocking mode (used for external connections).

## Connection Management

### 11. Connection Information Management

#### `alloc_ci`
**Parameters:**
- Socket file descriptor (integer)

**Returns:** Pointer to connection information structure (NULL on failure)

**Purpose:** Allocates and initializes connection information structure.

**Initialization:**
- Clears entire structure to zero
- Sets socket file descriptor
- Sets initial state to header reading
- Records connection start timestamp
- Sets initial bytes to read to header size

#### `free_ci`
**Parameters:**
- Connection information structure pointer

**Returns:** None

**Purpose:** Frees all resources associated with connection.

**Cleanup Process:**
- Frees socket address structure (UDP connections)
- Frees body buffer (if allocated)
- Frees output buffer (response data)
- Frees write buffer (formatted response)
- Frees main connection structure
- Handles NULL pointers gracefully

### 12. Connection Cleanup (`close_connection`)
**Parameters:**
- Connection information structure pointer

**Returns:** None (static function)

**Purpose:** Closes TCP connection and cleans up all associated resources.

**Process:**
1. **Epoll Cleanup:**
   - Removes connection from epoll monitoring
   - Prevents further event notifications

2. **Socket Cleanup:**
   - Closes socket file descriptor
   - Releases system socket resources

3. **Tracking Cleanup:**
   - Removes connection from global tracking array
   - Prevents access to freed connection

4. **Resource Cleanup:**
   - Calls free_ci to release all allocated memory
   - Ensures no memory leaks

## Data Structures and State Management

### Connection State Machine
- **STATE_WANT_READ_HEADER:** Waiting for complete request header
- **STATE_WANT_READ_BODY:** Waiting for complete request body
- **STATE_PROCESSING:** Request being processed by worker thread
- **STATE_WANT_WRITE:** Response ready for transmission

### Global Data Structures
- **connections:** Array tracking connection information by file descriptor
- **mod_queue:** Circular buffer for cross-thread write notifications
- **mod_queue_mutex:** Mutex protecting modification queue operations

### Performance Optimizations
- **Edge-Triggered Epoll:** Reduces event notification overhead (Edge-Triggered Notification: Uses event polling strategies that reduce spurious wakeups and improve efficiency (e.g., EPOLLET, EV_CLEAR in kqueue))
- **Connection Pooling:** Reuses connection structures when possible
- **Buffer Management:** Efficient allocation and cleanup strategies
- **Event Batching:** Processes multiple events per epoll_wait call

## Security Considerations

### Network Security
- **Connection Limits:** Maximum file descriptor limits prevent resource exhaustion
- **Request Size Limits:** MAX_BODY_SIZE prevents memory exhaustion attacks
- **Address Validation:** Client IP addresses logged for security monitoring

### Resource Protection
- **Memory Management:** Careful allocation and cleanup prevents memory leaks
- **File Descriptor Management:** Proper cleanup prevents descriptor exhaustion
- **Buffer Overflow Protection:** Size validation prevents buffer overflows
- **Error Handling:** Secure error responses prevent information leakage

### Access Control
- **Protocol Validation:** All requests validated before processing
- **Connection Tracking:** Global tracking enables monitoring and control
- **Rate Limiting:** Foundation for implementing rate limiting controls
- **Audit Logging:** Connection events logged for security analysis

## Dependencies and Integration

### Required Modules
- **Protocol Layer:** Request/response validation, encryption/decryption
- **Thread Pool:** Worker thread management for request processing
- **Configuration System:** Network parameters, port configuration
- **Statistics System:** Request counting, performance monitoring
- **Logging System:** Debug output, error reporting, security logging

### System Dependencies
- **Epoll System:** Linux-specific event notification mechanism
- **EventFD System:** Linux-specific cross-thread signaling
- **Socket API:** POSIX socket operations and configuration
- **Threading API:** POSIX thread synchronization primitives

###  Constants Required
| Constant              | Purpose                                               |
|-----------------------|--------------------------------------------------------|
| `MAX_BODY_SIZE`       | Maximum allowed size of request body (prevents abuse) |
| `HEADER_SIZE`         | Size in bytes of request header                       |
| `MAXEPOLLSIZE`        | Max events handled in a single epoll_wait call        |
| `RAIDA_EPOLL_TIMEOUT` | Timeout (ms) for epoll_wait calls                     |
| `LISTEN_PORT`         | Port number for both TCP and UDP listening sockets    |
| `MOD_QUEUE_SIZE`      | Size of the circular queue for write notifications    |


### Used By
- **Main Server:** Primary network interface for client communication
- **Command Processors:** Request routing and response handling
- **Administrative Tools:** Health monitoring and connection management

## Performance Characteristics

### Scalability
- **Concurrent Connections:** Handles thousands of simultaneous connections
- **Event Efficiency:** Epoll scales efficiently with connection count
- **Memory Usage:** O(n) memory usage for n active connections
- **CPU Efficiency:** Single I/O thread minimizes context switching

### Latency Optimization
- **Non-Blocking Operations:** No blocking delays in I/O operations
- **Edge-Triggered Events:** Minimal event notification overhead
- **Direct UDP Processing:** Lower latency for connectionless requests
- **Efficient State Management:** Minimal overhead per connection

### Throughput Optimization
- **Batch Processing:** Multiple events processed per system call
- **Efficient Memory Management:** Optimized allocation patterns
- **Zero-Copy Operations:** Minimal data copying in network stack
- **Thread Pool Integration:** Optimal CPU utilization for request processing

This network layer provides the foundation for high-performance, scalable client communication in the RAIDA system, supporting both reliable TCP connections and efficient UDP communications while maintaining security and resource efficiency.