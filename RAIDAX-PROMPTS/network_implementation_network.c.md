
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

This specification provides complete implementation guidance for a high-performance, event-driven network layer while remaining language-agnostic and focusing only on the actual functionality present in the source code.