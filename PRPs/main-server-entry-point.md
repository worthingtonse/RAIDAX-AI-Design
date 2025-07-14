# RAIDA Server Main Entry Point (main.c)

## Module Purpose
This module serves as the main entry point for the RAIDA server application, orchestrating the initialization of all system components, configuring the runtime environment, and managing the server lifecycle. It coordinates database systems, network services, security features, and background processes to create a fully functional RAIDA network node.

## Core Functionality

### 1. Main Entry Function (`main`)
**Parameters:**
- Argument count (integer)
- Argument vector (array of strings)

**Returns:** Integer exit code (0 for success, 1 for failure)

**Purpose:** Primary entry point that initializes all system components and starts the server.

**Process:**
1. **Logging Initialization:**
   - Initializes logging subsystem first for error reporting
   - Configures log levels and output destinations
   - Ensures early error messages are captured

2. **Configuration Loading:**
   - Reads configuration file using executable path
   - Validates configuration parameters
   - Sets up server identification and network parameters

3. **Signal Handler Installation:**
   - Installs signal handlers for graceful shutdown
   - Configures handlers for SIGTERM, SIGINT, SIGHUP, SIGPIPE
   - Sets up special handlers for sync operations (SIGUSR1)

4. **Hardware Feature Detection:**
   - Detects Intel AES instruction support
   - Configures cryptographic optimizations
   - Enables hardware acceleration where available

5. **Database System Initialization:**
   - Initializes on-demand page cache database system
   - Starts background persistence threads
   - Validates database structure and accessibility

6. **Subsystem Initialization:**
   - Initializes ticket storage for healing operations
   - Sets up locker index for coin storage
   - Configures crossover index for external integration
   - Initializes statistics tracking system
   - Sets up IP hash table for rate limiting

7. **Integrity System Startup:**
   - Initializes Merkle tree integrity system
   - Starts background tree building threads
   - Enables network integrity verification

8. **Thread Pool Configuration:**
   - Configures thread pool size based on CPU cores or configuration
   - Initializes thread pool for request processing
   - Sets up worker threads for parallel processing

9. **Network Service Startup:**
   - Initializes and starts network socket listeners
   - Configures TCP and UDP service endpoints
   - Begins accepting client connections

**Dependencies:**
- All major system components
- Configuration system
- Logging system
- Network services

### 2. Signal Handler (`handle_signal`)
**Parameters:**
- Signal number (integer)
- Signal information structure pointer
- Signal context pointer

**Returns:** None

**Purpose:** Handles system signals for graceful shutdown and operational control.

**Process:**
1. **Signal Processing:**
   - Identifies signal type and source
   - Logs signal reception for debugging
   - Handles different signal types appropriately

2. **Shutdown Signals:**
   - SIGTERM, SIGINT, SIGHUP trigger graceful shutdown
   - Sets global shutdown flag (`is_finished`)
   - Allows all subsystems to clean up properly

3. **Operational Signals:**
   - SIGUSR1 triggers page synchronization
   - Sets sync flag (`need_sync`) for background processing
   - Enables manual synchronization operations

4. **Signal Safety:**
   - Unlocks log mutex to prevent deadlocks
   - Uses signal-safe operations only
   - Minimizes processing in signal context

**Signal Types:**
- **SIGTERM:** Graceful termination request
- **SIGINT:** Interrupt signal (Ctrl+C)
- **SIGHUP:** Hangup signal (configuration reload)
- **SIGPIPE:** Broken pipe signal (network disconnection)
- **SIGUSR1:** User-defined signal (sync request)

### 3. Signal Handler Installation (`install_signal_handlers`)
**Parameters:**
- None

**Returns:** Integer status code (0 for success, -1 for failure)

**Purpose:** Installs signal handlers for all relevant system signals.

**Process:**
1. **Signal Action Configuration:**
   - Configures signal action structure
   - Sets up signal handler function
   - Configures signal flags and masks

2. **Handler Installation:**
   - Installs handlers for all required signals
   - Validates successful installation
   - Ensures proper signal handling setup

3. **Error Handling:**
   - Checks for installation failures
   - Logs errors appropriately
   - Returns failure status on errors

**Signal Configuration:**
- **SA_SIGINFO:** Extended signal information
- **SA_RESTART:** Restart interrupted system calls
- **Signal Mask:** Controls signal blocking during handler execution

## Global State Management

### 1. Server State Variables
**Variables:**
- `is_finished`: Global shutdown flag for all subsystems
- `need_sync`: Flag indicating need for page synchronization
- `aes_hw`: Hardware AES support indicator
- `thpool`: Global thread pool for request processing

**Purpose:** Coordinate operations across all system components
**Thread Safety:** Read by multiple threads, written by signal handlers
**Lifecycle:** Initialized at startup, used throughout server operation

### 2. External References
**Variables:**
- `config`: Global configuration structure
- `log_mtx`: Logging system mutex for thread safety

**Purpose:** Access shared system resources and configuration
**Source:** Defined in other modules, referenced here
**Usage:** Configuration and logging coordination

## System Initialization Sequence

### 1. Early Initialization
1. **Logging System:** First priority for error reporting
2. **Configuration Loading:** Required for all other systems
3. **Signal Handlers:** Essential for proper shutdown handling
4. **Hardware Detection:** Enables optimizations early

### 2. Core System Initialization
1. **Database System:** Foundation for all coin operations
2. **Memory Pools:** Ticket storage and other pools
3. **Index Systems:** Locker and crossover indexes
4. **Statistics System:** Operation tracking and monitoring

### 3. Advanced Features
1. **Rate Limiting:** IP hash table for abuse prevention
2. **Integrity System:** Merkle tree verification
3. **Thread Pool:** Parallel request processing
4. **Network Services:** Client connection handling

### 4. Service Startup
1. **Network Listeners:** TCP and UDP service endpoints
2. **Background Threads:** Various maintenance operations
3. **Ready State:** Server ready to accept requests

## Configuration and Environment

### 1. Configuration Sources
- **Configuration File:** Primary source of server parameters
- **Command Line:** Executable path for configuration location
- **Environment:** Hardware detection and system parameters
- **Defaults:** Built-in defaults for missing parameters

### 2. Configuration Categories
- **Server Identity:** RAIDA number, coin ID, admin keys
- **Network Settings:** Ports, addresses, timeout values
- **Database Settings:** Paths, cache sizes, flush frequency
- **Security Settings:** Cryptographic parameters, rate limits
- **Performance Settings:** Thread counts, memory limits

### 3. Hardware Optimization
- **AES Instructions:** Intel AES-NI support detection
- **CPU Cores:** Thread pool sizing based on CPU count
- **Memory:** Cache sizing based on available memory
- **Storage:** Database configuration for storage type

## Error Handling and Recovery

### 1. Initialization Errors
- **Logging Failure:** Critical error, immediate exit
- **Configuration Errors:** Invalid config, immediate exit
- **Signal Handler Errors:** Cannot install handlers, immediate exit
- **Database Errors:** Cannot initialize database, immediate exit

### 2. Service Errors
- **Network Errors:** Cannot bind ports, immediate exit
- **Thread Pool Errors:** Cannot create threads, immediate exit
- **Memory Errors:** Cannot allocate memory, immediate exit
- **File System Errors:** Cannot access files, immediate exit

### 3. Recovery Mechanisms
- **Graceful Shutdown:** Proper cleanup on exit
- **Error Logging:** Detailed error reporting
- **Resource Cleanup:** Proper resource deallocation
- **Exit Codes:** Proper exit status reporting

## Performance and Scalability

### 1. Thread Pool Configuration
- **Dynamic Sizing:** Based on CPU cores or configuration
- **Worker Threads:** Parallel request processing
- **Queue Management:** Request queue for load balancing
- **Resource Limits:** Prevents resource exhaustion

### 2. Memory Management
- **Cache Sizing:** Database cache based on available memory
- **Pool Management:** Memory pools for frequent allocations
- **Garbage Collection:** Automatic cleanup of unused resources
- **Memory Monitoring:** Statistics and monitoring

### 3. Network Performance
- **Connection Pooling:** Efficient connection management
- **Asynchronous I/O:** Non-blocking network operations
- **Load Balancing:** Thread pool for request distribution
- **Rate Limiting:** Protection against abuse

## Security Features

### 1. Cryptographic Security
- **Hardware Acceleration:** AES-NI when available
- **Secure Random:** Cryptographically secure random numbers
- **Hash Functions:** SHA-256 for integrity verification
- **Key Management:** Secure key storage and handling

### 2. Network Security
- **Rate Limiting:** Protection against DDoS attacks
- **Input Validation:** All input validated before processing
- **Authentication:** Admin key validation for sensitive operations
- **Encryption:** Secure communication channels

### 3. System Security
- **Signal Handling:** Secure signal processing
- **Resource Limits:** Prevention of resource exhaustion
- **Access Control:** File and network access restrictions
- **Audit Logging:** Security event logging

## Dependencies and Integration

### Required Modules
- **Logging System:** Error reporting and debugging
- **Configuration System:** Server parameters and settings
- **Database System:** Coin data storage and access
- **Network System:** Client communication and services
- **Security System:** Cryptographic operations and validation
- **Threading System:** Parallel processing and background operations

### External Dependencies
- **System Libraries:** POSIX threads, networking, file I/O
- **Cryptographic Libraries:** OpenSSL or equivalent
- **Hardware Libraries:** CPU feature detection
- **Operating System:** Signal handling, process management

### Integration Points
- **Command Handlers:** All command processing modules
- **Background Services:** Various maintenance operations
- **Network Protocols:** Client and server communication
- **Storage Systems:** Database and file system integration

## Monitoring and Maintenance

### 1. Operational Monitoring
- **Statistics System:** Operation counts and performance metrics
- **Health Checks:** System health and status monitoring
- **Error Tracking:** Error rates and failure analysis
- **Resource Usage:** Memory, CPU, and network utilization

### 2. Maintenance Operations
- **Graceful Shutdown:** Proper server shutdown procedures
- **Configuration Reload:** Dynamic configuration updates
- **Log Rotation:** Log file management and rotation
- **Database Maintenance:** Background database operations

### 3. Debug and Troubleshooting
- **Detailed Logging:** Comprehensive operation logging
- **Error Reporting:** Detailed error information
- **Performance Profiling:** Operation timing and bottlenecks
- **State Inspection:** System state examination tools

This main entry module provides the foundation for the RAIDA server, orchestrating all system components and ensuring reliable, secure, and high-performance operation of the network node.