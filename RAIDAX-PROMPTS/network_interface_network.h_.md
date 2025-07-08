Language-Agnostic Specification: Network Interface
1. Module Purpose
This module defines the public interface for the RAIDAX server's high-performance, non-blocking network layer. It specifies the required functions and constants for initializing and running the server's network event loop.

2. Core Constants
EVENT_LOOP_TIMEOUT: An integer representing the timeout for the I/O event multiplexer in milliseconds (e.g., 10000).

MAX_CONNECTIONS: The maximum number of concurrent connections the server is configured to handle (e.g., 65535).

MAX_BODY_SIZE: A constant defining the maximum allowed size for a request body (e.g., 65536 bytes). This is a critical security measure.

3. Required Functions
The network layer implementation must expose the following functionalities:

start_server_event_loop(): The main entry point for the network layer. It initializes the server sockets, sets up the asynchronous I/O event multiplexer, and starts the main event loop. This function should block indefinitely.

initialize_tcp_listener(): Creates, binds, and listens on the primary TCP socket, configuring it for non-blocking operation. Returns a handle to the listening socket.

initialize_udp_socket(): Creates and binds the primary UDP socket, configuring it for non-blocking operation. Returns a handle to the UDP socket.

arm_socket_for_write(connection_object): A thread-safe function that allows a worker thread to signal the main I/O thread that a connection is ready to be written to.

4. Architectural Notes
The interface is designed to be abstract and can be implemented using any modern asynchronous I/O framework (e.g., epoll, kqueue, IOCP, libuv, asyncio, Netty).

The design separates the network I/O logic (this module) from the protocol-handling logic.

////////////////////////

#  Specification: CloudCoin Network Header Interface

## 1. Module Purpose
This specification defines the public interface for the CloudCoin network layer. It establishes the constants, data structure requirements, and function signatures needed to implement a high-performance, event-driven network system supporting both TCP and UDP protocols.

## 2. System Constants and Configuration

### 2.1 Network Performance Parameters
```
RAIDA_EPOLL_TIMEOUT = 10000        // Timeout in milliseconds
MAXEPOLLSIZE = 10000               // Maximum events per wait operation
MAX_FDS = 65535                    // Maximum file descriptors
SOCKET_TIMEOUT = 2                 // Socket timeout in seconds
```

### 2.2 Security and Resource Limits
```
MAX_BODY_SIZE = 65536              // Maximum request body size in bytes
```

## 3. Core Data Structures

### 3.1 Connection Information Requirements
The connection information structure must track the complete state of a network connection and include these logical components:

**Socket and Network Information:**
- Socket file descriptor (integer)
- Client IP address (16-character array including null terminator)
- Socket address structure pointer (for UDP connections, dynamically allocated)
- Connection start timestamp (platform-specific time structure)

**Connection State Management:**
- Current connection state (enumerated type)
- Read buffer for headers (fixed-size byte array of REQUEST_HEADER_SIZE)
- Bytes read in current operation (integer)
- Bytes expected for current operation (integer)

**Dynamic Buffer Management:**
- Body buffer pointer (dynamically allocated byte array)
- Body size (integer)
- Write buffer pointer (dynamically allocated byte array)
- Bytes written in current operation (integer)
- Total bytes to write (integer)
- Output data pointer (processed response data)

**Protocol Header Fields:**
- Session identifier (32-bit unsigned integer)
- Command type (8-bit unsigned integer)
- Protocol version (8-bit unsigned integer)
- RAIDA identifier (8-bit unsigned integer)
- Reserved field (8-bit unsigned integer)

### 3.2 Connection State Enumeration
The system must support these connection states:

| State Name              | Description                              |
|------------------------|------------------------------------------|
| STATE_WANT_READ_HEADER | Waiting to read complete request header  |
| STATE_WANT_READ_BODY   | Waiting to read complete request body    |
| STATE_PROCESSING       | Request being processed by worker thread |
| STATE_WANT_WRITE       | Ready to write response data             |

## 4. Public Function Interface

### 4.1 Main Network Functions

#### 4.1.1 init_and_listen_sockets()
```
Function: init_and_listen_sockets
Parameters: none
Returns: integer (0 = success, negative = error)
Purpose: Initialize network layer and run main event loop
```

#### 4.1.2 init_tcp_socket()
```
Function: init_tcp_socket
Parameters: none
Returns: integer (socket descriptor or negative on error)
Purpose: Create and configure TCP listening socket
```

#### 4.1.3 init_udp_socket()
```
Function: init_udp_socket
Parameters: none
Returns: integer (socket descriptor or negative on error)
Purpose: Create and configure UDP socket
```

### 4.2 Socket Configuration Functions

#### 4.2.1 set_nonblocking(file_descriptor)
```
Function: set_nonblocking
Parameters: fd (integer file descriptor)
Returns: integer (0 = success, negative = error)
Purpose: Configure socket for non-blocking operation
```

#### 4.2.2 set_blocking(file_descriptor)
```
Function: set_blocking
Parameters: fd (integer file descriptor)
Returns: integer (0 = success, negative = error)
Purpose: Configure socket for blocking operation
```

### 4.3 Connection Management Functions

#### 4.3.1 alloc_ci(socket)
```
Function: alloc_ci
Parameters: sk (integer socket file descriptor)
Returns: pointer to connection_info (NULL on allocation failure)
Purpose: Allocate and initialize connection information structure
```

#### 4.3.2 free_ci(conn_info)
```
Function: free_ci
Parameters: ci (pointer to connection_info)
Returns: void
Purpose: Free all resources associated with connection information
```

### 4.4 Event Handling Functions

#### 4.4.1 handle_new_tcp_connection(tcp_socket)
```
Function: handle_new_tcp_connection
Parameters: tsocket (integer TCP listening socket)
Returns: void
Purpose: Accept and configure new TCP connections
```

#### 4.4.2 handle_udp_request(udp_socket)
```
Function: handle_udp_request
Parameters: usocket (integer UDP socket)
Returns: void
Purpose: Process incoming UDP datagrams
```

#### 4.4.3 handle_connection_event(conn_info, events)
```
Function: handle_connection_event
Parameters:
    ci (pointer to connection_info)
    events (32-bit unsigned integer event mask)
Returns: void
Purpose: Route connection events to appropriate handlers
```

### 4.5 Cross-Thread Communication

#### 4.5.1 arm_socket_for_write(conn_info)
```
Function: arm_socket_for_write
Parameters: ci (pointer to connection_info)
Returns: void
Purpose: Signal main I/O thread that socket is ready for writing
Thread Context: Called by worker threads after processing requests
```

## 5. Function Signature Corrections

Note: The header file contains function signatures that differ from the implementation:

### 5.1 Signature Discrepancies
- handle_new_tcp_connection: Header declares extra epoll_fd parameter not used in implementation
- handle_udp_request: Header declares extra epoll_fd parameter not used in implementation  
- handle_connection_event: Header declares extra epoll_fd parameter not used in implementation

### 5.2 Implementation Requirement
Implementations should follow the actual function signatures used in the implementation rather than the header declarations for these three functions.

## 6. Memory Management Requirements

### 6.1 Allocation Strategy
- Connection information structures: Dynamically allocated per connection
- Body buffers: Dynamically allocated when request body is present
- Write buffers: Dynamically allocated for response data
- Socket address structures: Dynamically allocated for UDP connections only

### 6.2 Deallocation Requirements
The free_ci function must handle cleanup of:
- Socket address structure (if not NULL)
- Body buffer (if not NULL)
- Output buffer (if not NULL)
- Write buffer (if not NULL)
- Main connection structure
- Must handle NULL input parameter gracefully

## 7. Initialization Requirements

### 7.1 Connection Structure Initialization
When allocating connection information, implementations must:
- Initialize all fields to zero
- Set socket descriptor to provided value
- Set initial state to STATE_WANT_READ_HEADER
- Set bytes_to_read to REQUEST_HEADER_SIZE
- Record current timestamp as start time

### 7.2 Socket Configuration Requirements
TCP sockets must be configured with:
- SO_REUSEADDR option enabled
- Non-blocking mode enabled
- TCP keep-alive enabled with specific parameters:
  - keepalive = 1
  - keep_idle = 60 seconds
  - keep_interval = 10 seconds
  - keep_count = 5 probes

UDP sockets must be configured with:
- Non-blocking mode enabled

## 8. Error Handling Standards

### 8.1 Return Code Conventions
- Success operations return 0
- Error conditions return negative values
- Pointer-returning functions return NULL on error
- All system errors should be logged with context

### 8.2 Resource Cleanup
- All allocated resources must be freed on error paths
- Connection cleanup must remove from all tracking structures
- Socket file descriptors must be closed on connection termination
- Error responses should be sent before closing connections when possible

## 9. Threading and Concurrency

### 9.1 Thread Safety Requirements
- Main network functions execute in single I/O thread
- Connection structures accessed by single thread at a time during processing
- Cross-thread communication uses thread-safe signaling mechanism
- Worker threads call arm_socket_for_write to signal completion

### 9.2 Synchronization Points
- Write queue operations require mutex protection
- EventFD signaling provides cross-thread communication
- Connection state transitions managed by single thread

## 10. Integration Dependencies

### 10.1 External Constants Required
- REQUEST_HEADER_SIZE: Size of protocol request header
- NO_ERROR: Success status code from protocol layer
- ERROR_INVALID_PACKET_LENGTH: Protocol error for size validation
- ERROR_MEMORY_ALLOC: Protocol error for allocation failures

### 10.2 External Function Dependencies
- validate_header: Protocol header validation
- validate_decrypt_body: Protocol body validation and decryption
- run_command: Request processing function
- send_command_error: Error response transmission
- Thread pool functions for work submission
- Logging functions for debug and error output
- Statistics functions for request counting

### 10.3 Configuration Access Required
- config.port: Network port number for binding
- config.udp_payload_threshold: Maximum UDP packet size
- is_finished: Global shutdown signal

## 11. Platform Abstraction

### 11.1 Required Abstractions
Implementations must provide platform-appropriate equivalents for:
- Event notification mechanism (epoll on Linux)
- Cross-thread signaling (eventfd on Linux)
- Socket API (POSIX sockets)
- Threading primitives (POSIX threads)
- File descriptor management
- Non-blocking I/O configuration

### 11.2 Type System Requirements
- Integer types: Standard signed and unsigned integers
- Pointer types: For dynamic memory management
- Network address types: Platform socket address structures
- Time types: Platform timestamp structures
- Thread synchronization types: Platform mutex primitives

## 12. Performance Considerations

### 12.1 Scalability Requirements
- Support for thousands of concurrent connections
- Non-blocking operations throughout
- Efficient event notification mechanism
- Minimal memory allocation during steady-state operation

### 12.2 Resource Management
- Connection limits enforced by MAX_FDS constant
- Body size limits enforced by MAX_BODY_SIZE constant
- Timeout mechanisms for connection cleanup
- Efficient cleanup of terminated connections

## 13. Security Interface

### 13.1 Input Validation
- All header fields must be validated before use
- Body size must not exceed MAX_BODY_SIZE
- Buffer operations must include bounds checking
- Malformed requests must be rejected gracefully

### 13.2 Resource Protection
- Connection limits prevent resource exhaustion
- Memory limits prevent excessive allocation
- Timeout mechanisms prevent connection hoarding
- Error responses prevent information leakage

This specification provides the complete interface definition needed to implement a high-performance network layer while remaining language-agnostic and accurately reflecting the actual implementation requirements.