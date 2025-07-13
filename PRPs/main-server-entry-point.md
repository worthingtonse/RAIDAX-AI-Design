# Main Server Entry Point Implementation (main)

## Module Purpose
This module implements the primary server initialization and lifecycle management for the RAIDA network server. It orchestrates the startup sequence for all subsystems, configures signal handling for graceful shutdown, detects hardware capabilities, and manages the main server lifecycle with optimized database initialization. The module has been updated to support the new on-demand page cache system and enhanced performance optimizations.

## Core Functionality

### 1. Signal Handling System (`handle_signal`)
**Parameters:**
- Signal number (integer identifying the received signal)
- Signal information structure pointer (system signal metadata)
- Context pointer (signal context, unused)

**Returns:** None

**Purpose:** Handles system signals for graceful server shutdown and operational control.

**Process:**
1. **Mutex Safety:** Unlocks log mutex in case signal interrupted logging operation
2. **Signal Processing:**
   - **SIGUSR1:** Sets need_sync flag for manual page synchronization
   - **Termination Signals (SIGTERM, SIGINT, SIGHUP):** Sets is_finished flag for graceful shutdown
   - **SIGPIPE:** Handles broken pipe conditions from network operations
3. **Shutdown Coordination:** Logs termination request and initiates shutdown sequence

**Signal Handling Features:**
- **Graceful Shutdown:** Allows all subsystems to complete current operations
- **Manual Sync:** Provides administrative control over database synchronization
- **Interrupt Safety:** Handles signals safely even during critical operations

### 2. Signal Handler Installation (`install_signal_handlers`)
**Parameters:** None

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Configures system signal handlers for proper server lifecycle management.

**Process:**
1. **Handler Configuration:**
   - Sets up signal action structure with SA_SIGINFO and SA_RESTART flags
   - Configures signal mask to prevent signal interference
   - Associates handle_signal function with all relevant signals

2. **Signal Registration:**
   - **SIGTERM:** Graceful shutdown from process manager
   - **SIGINT:** Interactive shutdown (Ctrl-C)
   - **SIGHUP:** Configuration reload or shutdown
   - **SIGPIPE:** Broken network connection handling
   - **SIGUSR1:** Manual database synchronization trigger

3. **Validation:** Ensures all signal handlers installed successfully

**Dependencies:**
- System signal handling APIs
- Error handling for signal installation failures

### 3. Main Server Entry Point (`main`)
**Parameters:**
- Argument count (integer number of command line arguments)
- Argument array (array of string pointers containing command line arguments)

**Returns:** Integer exit code (0 for success, non-zero for error)

**Purpose:** Primary server initialization and main execution loop with optimized startup sequence.

**Process:**

#### Phase 1: Core System Initialization
1. **Logging System Startup:**
   - Initializes logging subsystem before any other operations
   - Enables debug output and error reporting throughout startup
   - Logs server build timestamp and version information

2. **Configuration Loading:**
   - Reads configuration file using executable path
   - Validates all mandatory and optional parameters
   - Handles configuration errors with appropriate error messages

3. **Signal Handler Installation:**
   - Sets up signal handling for graceful operation and shutdown
   - Configures handlers for all relevant system signals

#### Phase 2: Hardware and Database Initialization
4. **Hardware Capability Detection:**
   - Checks for Intel AES instruction support
   - Logs hardware acceleration availability
   - Optimizes cryptographic operations based on capabilities

5. **REVOLUTIONARY: On-Demand Database Initialization:**
   - Calls init_db() which now initializes the on-demand page cache
   - **NO LONGER** calls load_pages() - this is the core optimization
   - Pages now loaded on-demand when accessed rather than at startup
   - Dramatically reduces startup time and memory usage

#### Phase 3: Subsystem Initialization
6. **Memory Pool Initialization:**
   - Initializes ticket storage with fine-grained locking
   - Sets up memory pools for request processing

7. **Optimized Index Systems:**
   - Initializes locker indexing with incremental update support
   - Initializes crossover indexing with optimized background processing
   - Both systems now use performance-optimized algorithms

8. **Statistics and Monitoring:**
   - Initializes statistics collection system
   - Sets up IP hash table for rate limiting and monitoring

#### Phase 4: Background Thread Launch
9. **: Optimized Persistence Thread:**
   - Launches persistence_and_eviction_thread for database management
   - Handles dirty page synchronization and cache eviction
   

10. **Integrity Checking Thread:**
    - Starts background integrity verification (currently disabled)
    - Provides framework for data consistency monitoring

#### Phase 5: Network and Threading Setup
11. **Thread Pool Configuration:**
    - Determines optimal thread pool size:
      - Uses configured thread count if specified
      - Otherwise uses system CPU count or default minimum
    - Initializes thread pool for request processing

12. **Network Layer Startup:**
    - Calls init_and_listen_sockets() which blocks in main event loop
    - Handles both TCP and UDP connections
    - Processes client requests until shutdown signal received

#### Phase 6: Shutdown and Cleanup
13. **Graceful Termination:**
    - Returns from network event loop when is_finished flag set
    - All subsystems clean up automatically
    - Logs successful program completion

**Dependencies:**
- Configuration management system
- Logging subsystem
- Database layer with on-demand caching
- Network layer with event-driven processing
- Thread pool management
- All server subsystems

## Global Variables and State Management

### Server State Variables
- **is_finished:** Integer flag controlling main server loop termination
- **need_sync:** Integer flag for manual database synchronization (legacy usage)
- **aes_hw:** Integer flag indicating Intel AES instruction availability

### External References
- **config:** Global configuration structure from configuration module
- **thpool:** Thread pool handle for request processing
- **log_mtx:** Logging mutex for thread-safe log operations

## Hardware Optimization

### CPU Feature Detection
**Purpose:** Optimizes cryptographic operations based on available hardware features

#### AES Instruction Support
- **Detection:** Calls check_for_aes_instructions() to detect Intel AES-NI
- **Optimization:** Enables hardware-accelerated AES operations when available
- **Fallback:** Uses software AES implementation on unsupported hardware
- **Logging:** Reports hardware acceleration status for monitoring

## Performance Optimizations

### Memory Usage Optimization

#### : On-Demand Approach
- **Startup:** Seconds - no page preloading required
- **Memory:** Megabytes - only accessed pages cached
- **Scalability:** Independent of database size

### Thread Pool Optimization
- **Dynamic Sizing:** Adapts to system capabilities
- **CPU Detection:** Uses get_nprocs() for optimal thread count
- **Configuration Override:** Allows manual thread count specification
- **Performance Scaling:** Balances concurrency with resource usage

### Subsystem Optimization
- **Locker Indexing:** Incremental updates replace expensive full rebuilds
- **Crossover Processing:** Optimized background thread with dynamic sleep
- **Cache Management:** Intelligent page caching with LRU eviction

- `init_locker_index() → int`: Initializes locker index structures.
- `init_crossover_index() → int`: Initializes transaction ledger.
- `init_stats() → int`: Sets up usage/statistics collection.
- `init_ht() → int`: Initializes rate-limiting hash table.


## Error Handling and Recovery

### Initialization Error Handling
- **Early Exit:** Critical errors cause immediate program termination
- **Error Reporting:** Detailed error messages for troubleshooting
- **Resource Cleanup:** Proper cleanup even during initialization failures
- **Exit Codes:** Non-zero exit codes indicate specific failure types

### Runtime Error Handling
- **Signal Safety:** Signal handlers are interrupt-safe
- **Graceful Shutdown:** All subsystems notified of shutdown request
- **Resource Cleanup:** Automatic cleanup during normal shutdown
- **Error Isolation:** Subsystem failures don't crash entire server

### Recovery Mechanisms
- **Restart Capability:** Server can be restarted safely after shutdown
- **Configuration Reload:** SIGHUP can trigger configuration reload (framework)
- **Manual Synchronization:** SIGUSR1 provides manual database sync
- **Health Monitoring:** Echo command provides health check capability

## Dependencies and Integration

### Required Modules
- **Configuration System:** Server parameters, network settings, security keys
- **Logging System:** Debug output, error reporting, operational logging
- **Database Layer:** On-demand page cache, persistence management
- **Network Layer:** TCP/UDP event handling, connection management
- **Threading System:** Thread pool, background processing
- **Cryptographic System:** Hardware acceleration detection, AES operations

### System Dependencies
- **Signal Handling:** POSIX signal management
- **CPU Detection:** System processor count and feature detection
- **Memory Management:** Dynamic allocation for subsystems
- **File System:** Configuration file access, database storage

### Integration Points
- **Configuration Loading:** Reads config.toml from executable directory
- **Network Binding:** Listens on configured port for client connections
- **Database Access:** Provides page access through on-demand caching
- **Request Processing:** Handles client requests through thread pool

## Security Considerations

### Secure Initialization
- **Configuration Validation:** All parameters validated before use
- **Key Loading:** Cryptographic keys loaded securely
- **Permission Checking:** File system permissions validated
- **Error Handling:** No sensitive information leaked in error messages

### Runtime Security
- **Signal Handling:** Secure signal processing prevents exploitation
- **Resource Limits:** Thread pool and memory usage controlled
- **Network Security:** Proper connection handling prevents attacks
- **Access Control:** Administrative functions require authentication

### Operational Security
- **Logging Security:** No sensitive data logged inadvertently
- **Shutdown Security:** Graceful shutdown prevents data corruption
- **Error Reporting:** Security-conscious error message handling
- **Monitoring:** Health check capabilities for security monitoring

## Monitoring and Observability

### Server Health Monitoring
- **Startup Sequence:** Each initialization phase logged
- **Hardware Detection:** CPU features and capabilities logged
- **Performance Metrics:** Thread pool size and configuration logged
- **Error Tracking:** All initialization errors logged with detail

### Operational Monitoring
- **Signal Processing:** Signal reception and handling logged
- **Shutdown Process:** Graceful shutdown progress logged
- **Resource Usage:** Memory and thread usage monitoring
- **Performance Tracking:** Database and network performance metrics

### Debug Information
- **Build Information:** Server build timestamp logged at startup
- **Configuration Summary:** Key configuration parameters logged
- **Hardware Summary:** CPU and acceleration features logged
- **Network Status:** Listening ports and addresses logged

## Performance Characteristics

### Startup Performance
- **Dramatic Improvement:** Startup time reduced from minutes to seconds
- **Memory Efficiency:** Memory usage reduced by orders of magnitude
- **Scalability:** Startup time independent of database size
- **Resource Usage:** Minimal resource consumption during initialization

### Runtime Performance
- **Thread Pool Scaling:** Optimal concurrency for system capabilities
- **Hardware Acceleration:** AES operations optimized for available hardware
- **Cache Efficiency:** On-demand page loading optimizes memory usage
- **Background Processing:** Optimized background threads minimize overhead

### Shutdown Performance
- **Graceful Termination:** Quick, clean shutdown process
- **Resource Cleanup:** Automatic cleanup of all allocated resources
- **Data Safety:** All dirty data synchronized before shutdown
- **Restart Capability:** Fast restart after shutdown

## Future Enhancements

### Configuration Management
- **Hot Reload:** Runtime configuration reload without restart
- **Validation:** Enhanced configuration validation and error reporting
- **Monitoring:** Configuration change tracking and audit
- **Security:** Enhanced configuration file security

### Performance Monitoring
- **Metrics API:** Real-time performance metrics exposure
- **Health Checks:** Comprehensive health monitoring
- **Alerting:** Automated alerting for performance issues
- **Profiling:** Built-in performance profiling capabilities

### Operational Features
- **Rolling Updates:** Zero-downtime server updates
- **Load Balancing:** Multi-instance coordination
- **High Availability:** Clustering and failover support
- **Backup Integration:** Automated backup and recovery

This main server implementation provides the foundation for efficient, scalable, and secure RAIDA server operation with revolutionary performance improvements through on-demand page caching and optimized subsystem initialization.