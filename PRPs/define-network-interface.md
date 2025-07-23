# Network Header Definitions (net.h)

## Module Purpose
This header file defines the network interface constants, function declarations, and configuration parameters for the RAIDA network infrastructure. It provides the interface for high-performance asynchronous I/O operations, UDP performance optimizations, and cross-thread communication mechanisms.

## Network Configuration Constants

### Timeout and Performance Settings
| Constant | Value | Description |
|----------|-------|-------------|
| `RAIDA_EPOLL_TIMEOUT` | 10000 | Timeout for epoll_wait operations in milliseconds |
| `MAXEPOLLSIZE` | 10000 | Maximum number of events that epoll can handle in its backlog |
| `MAX_FDS` | 65535 | Maximum file descriptors to handle for connection tracking |
| `MAX_BODY_SIZE` | 65536 | Maximum allowed body size for requests (64KB) to prevent memory exhaustion attacks |
| `SOCKET_TIMEOUT` | 2 | Default socket read timeout in seconds |

### Performance Optimization Constants
| Constant | Value | Description |
|----------|-------|-------------|
| `UDP_CI_POOL_SIZE` | 4096 | Number of pre-allocated connection info structures for UDP performance optimization |

## Function Interface Specifications

### 1. Core Network Initialization
**Function Name:** Initialize and Listen Sockets

**Purpose:** Sets up complete network infrastructure including epoll instance, TCP/UDP listening sockets, and main event loop

**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Description:** Primary network initialization function that establishes the complete networking infrastructure for the RAIDA server, including event-driven I/O setup and cross-thread communication mechanisms.

### 2. Socket Initialization Functions

#### TCP Socket Initialization
**Function Name:** Initialize TCP Socket

**Purpose:** Creates and configures TCP listening socket with optimal settings

**Parameters:** None

**Returns:** Socket descriptor (integer, -1 on failure)

**Description:** Creates TCP listening socket with non-blocking mode, SO_REUSEADDR option, and proper binding configuration for RAIDA server operation.

#### UDP Socket Initialization
**Function Name:** Initialize UDP Socket

**Purpose:** Creates and configures UDP socket for client communication and integrity protocol

**Parameters:** None

**Returns:** Socket descriptor (integer, -1 on failure)

**Description:** Creates UDP socket for datagram communication with non-blocking mode and proper binding for both client requests and integrity protocol operations.

#### UDP Connection Info Pool Initialization
**Function Name:** Initialize UDP Connection Info Pool

**Purpose:** Initializes object pool for UDP connection structures to optimize performance under high traffic

**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Description:** Sets up pre-allocated pool of connection info structures specifically for UDP operations, eliminating malloc/free overhead during high-traffic periods and improving overall network performance.

### 3. Socket Configuration Functions

#### Set Non-Blocking Mode
**Function Name:** Set Non-Blocking

**Purpose:** Configures socket for non-blocking operation essential for async I/O

**Parameters:**
- File descriptor (integer)

**Returns:** Integer (0 for success, -1 for failure)

**Description:** Modifies socket flags to enable non-blocking mode using fcntl operations, essential for event-driven network architecture.

#### Set Blocking Mode
**Function Name:** Set Blocking

**Purpose:** Converts socket back to blocking mode when needed for specific operations

**Parameters:**
- File descriptor (integer)

**Returns:** Integer (0 for success, -1 for failure)

**Description:** Modifies socket flags to disable non-blocking mode, used for specialized operations that require blocking behavior.

### 4. Connection Management Functions

#### Allocate Connection Info
**Function Name:** Allocate Connection Info

**Purpose:** Allocates and initializes connection information structure for new connections

**Parameters:**
- Socket descriptor (integer)

**Returns:** Connection info pointer (NULL on failure)

**Description:** Creates and initializes connection information structure with proper state management, buffer allocation, and timing setup for new TCP connections.

#### Free Connection Info
**Function Name:** Free Connection Info

**Purpose:** Safely deallocates connection resources with differentiation between pooled and non-pooled structures

**Parameters:**
- Connection info pointer

**Returns:** None

**Description:** Properly frees connection resources, handling both regular TCP connections and UDP pooled structures differently to prevent resource leaks and maintain object pool integrity.

### 5. Connection Event Handlers

#### Handle New TCP Connection
**Function Name:** Handle New TCP Connection

**Purpose:** Accepts new TCP connections and integrates them into the event monitoring system

**Parameters:**
- TCP listening socket descriptor (integer)

**Returns:** None

**Description:** Processes new incoming TCP connections, configures them for non-blocking operation with keep-alive settings, and adds them to the epoll monitoring system for event-driven processing.

#### Handle UDP Request
**Function Name:** Handle UDP Request

**Purpose:** Processes UDP datagrams including specialized handling for integrity protocol with object pool optimization

**Parameters:**
- UDP socket descriptor (integer)

**Returns:** None

**Description:** Handles incoming UDP datagrams with optimized processing for integrity vote requests and regular client requests, utilizing object pool for performance and providing immediate response for integrity operations.

#### Handle Connection Event
**Function Name:** Handle Connection Event

**Purpose:** Dispatches connection events to appropriate read/write handlers based on event type

**Parameters:**
- Connection info pointer
- Event flags (32-bit integer)

**Returns:** None

**Description:** Central event dispatcher that routes epoll events to appropriate handlers based on event type (read, write, error) and current connection state.

### 6. Cross-Thread Communication

#### Arm Socket for Write
**Function Name:** Arm Socket for Write

**Purpose:** Thread-safe mechanism for worker threads to signal main I/O thread that connection is ready for writing

**Parameters:**
- Connection info pointer

**Returns:** None

**Description:** Provides cross-thread communication mechanism using eventfd and modification queue to notify the main I/O thread when worker threads have prepared responses for transmission.

## Network Architecture Design

### Event-Driven Model
- **Epoll-Based:** Uses Linux epoll for efficient event monitoring
- **Edge-Triggered:** Configured for edge-triggered mode for maximum performance
- **Non-Blocking I/O:** All socket operations are non-blocking to prevent thread blocking
- **Cross-Thread Communication:** EventFD-based signaling between I/O and worker threads

### Performance Optimizations
- **UDP Object Pool:** Pre-allocated structures eliminate allocation overhead
- **Connection Pooling:** Efficient reuse of connection structures
- **Batch Processing:** Multiple events processed per epoll_wait call
- **Zero-Copy Design:** Minimizes data copying where possible

### Scalability Features
- **High Connection Capacity:** Supports thousands of concurrent connections
- **Memory Bounded:** All memory usage is bounded and controlled
- **Thread-Safe Design:** Safe concurrent access from multiple threads
- **Resource Management:** Automatic cleanup and resource tracking

## Security Considerations

### Connection Security
- **IP Address Logging:** All connections logged with source IP for security analysis
- **Resource Limits:** Maximum connections and request sizes prevent exhaustion attacks
- **Timeout Management:** Configurable timeouts prevent resource holding
- **Keep-Alive Protection:** TCP keep-alive prevents stale connection accumulation

### Protocol Security
- **Size Validation:** All request sizes validated before processing
- **Format Validation:** Protocol headers validated before resource allocation
- **DDoS Protection:** UDP object pool provides graceful degradation under attack
- **Rate Limiting Ready:** Infrastructure supports connection and request rate limiting

### Memory Security
- **Bounded Allocations:** All dynamic allocations have maximum size limits
- **Pool Management:** Object pools prevent memory exhaustion
- **Buffer Overflow Prevention:** Careful buffer management prevents overflows
- **Resource Tracking:** All resources tracked for proper cleanup

## Integration Requirements

### Required System Dependencies
- **Linux Epoll:** Requires Linux epoll interface for event monitoring
- **POSIX Threading:** Requires POSIX threading for synchronization
- **Socket API:** Standard socket API for TCP/UDP operations
- **EventFD:** Linux eventfd for cross-thread signaling

### Module Dependencies
- **Protocol Module:** For request/response processing and validation
- **Thread Pool:** For worker thread management and request processing
- **Configuration Module:** For network settings and operational parameters
- **Statistics Module:** For performance monitoring and metrics collection

### Integration Points
- **Protocol Layer:** Provides network transport for all protocol operations
- **Command Processing:** Network interface for all command handlers
- **Integrity System:** Network communication for healing and consensus operations
- **Administrative Interface:** Network access for management operations

## Performance Characteristics

### Throughput Optimization
- **Event-Driven Architecture:** Scales to thousands of concurrent connections
- **Non-Blocking Operations:** Prevents blocking on slow clients
- **Batch Processing:** Efficient handling of multiple simultaneous events
- **Hardware Utilization:** Optimized for modern multi-core systems

### Latency Optimization
- **Object Pool:** Eliminates allocation latency for UDP operations
- **Direct Processing:** Integrity votes processed immediately without queuing
- **Minimal Copying:** Reduces data copying overhead
- **Efficient Buffering:** Optimal buffer sizes for typical request patterns

### Memory Efficiency
- **Bounded Usage:** All memory usage is bounded and predictable
- **Pool Management:** Object pools reduce memory fragmentation
- **Automatic Cleanup:** Systematic resource cleanup prevents leaks
- **Efficient Structures:** Compact data structures minimize memory overhead

## Error Handling and Recovery

### Network Error Handling
- **Connection Failures:** Graceful handling of network connectivity issues
- **Socket Errors:** Proper handling of all socket error conditions
- **Timeout Management:** Appropriate timeouts prevent resource starvation
- **Resource Exhaustion:** Graceful degradation when limits reached

### Memory Error Handling
- **Allocation Failures:** Graceful handling of memory allocation failures
- **Pool Exhaustion:** Safe handling when object pools are exhausted
- **Buffer Management:** Prevention of buffer overflow and underflow conditions
- **Resource Recovery:** Systematic recovery from resource shortage conditions

### Protocol Error Handling
- **Invalid Requests:** Proper rejection and cleanup of malformed requests
- **Size Violations:** Enforcement of protocol size limits with appropriate responses
- **State Violations:** Handling of invalid connection state transitions
- **Security Violations:** Detection and response to potential security threats

## Administrative and Monitoring Interface

### Performance Monitoring
- **Connection Statistics:** Real-time monitoring of active connections
- **Throughput Metrics:** Request and response throughput measurements
- **Error Statistics:** Comprehensive error tracking and reporting
- **Resource Utilization:** Memory and socket usage monitoring

### Configuration Control
- **Dynamic Parameters:** Some network parameters adjustable at runtime
- **Resource Limits:** Configurable limits for connections and resources
- **Timeout Settings:** Adjustable timeout values for various operations
- **Performance Tuning:** Parameters for optimizing performance characteristics

### Debugging Support
- **Connection Tracking:** Detailed tracking of connection lifecycle
- **Event Logging:** Comprehensive logging of network events
- **Performance Profiling:** Timing information for performance analysis
- **State Monitoring:** Visibility into connection and system state

This network header provides the complete interface specification for high-performance, scalable network operations in the RAIDA system, supporting both TCP and UDP protocols with advanced performance optimizations, security features, and comprehensive error handling capabilities.