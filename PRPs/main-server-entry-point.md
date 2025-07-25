# Main Entry Point and Server Initialization (main)

## Module Purpose
This module serves as the primary entry point for the RAIDAX server, orchestrating the initialization of all system components including the optimized database layer, integrity system, network infrastructure, and various subsystems. It provides comprehensive system startup, signal handling, and resource management with full optimization for modern hardware.

## Global State Management

### System Control Variables
| Variable | Type | Description |
|----------|------|-------------|
| `is_finished` | Integer | Global flag controlling main event loops across all modules |
| `need_sync` | Integer | Legacy synchronization flag (may be deprecated) |
| `aes_hw` | Integer | Hardware AES instruction support detection flag |
| `thpool` | Thread Pool | Main thread pool for request processing |

### External Dependencies
| Dependency | Source | Description |
|------------|--------|-------------|
| `log_mtx` | log.c | Logging system mutex for thread-safe logging |
| `config` | config.c | Global configuration structure |

## Core Functionality

### 1. Signal Handler (`handle_signal`)
**Parameters:**
- Signal number (integer)
- Signal info structure pointer
- Context pointer (unused)

**Returns:** None

**Purpose:** Handles system signals for graceful shutdown and operational control with proper resource cleanup.

**Process:**
1. **Logging Safety:**
   - Unlocks log mutex in case signal interrupted logging operation
   - Prevents deadlock in logging system during shutdown
   - Ensures logging remains functional during signal handling

2. **Signal Processing:**
   - **SIGUSR1:** Sets need_sync flag for manual synchronization trigger
   - **SIGTERM/SIGINT/SIGHUP/SIGPIPE:** Initiates graceful shutdown
   - Logs signal receipt for debugging and audit purposes

3. **Graceful Shutdown:**
   - Sets is_finished flag to signal all subsystems to shutdown
   - Allows running operations to complete before termination
   - Ensures clean resource cleanup across all modules

**Signal Safety:**
- **Async-Signal-Safe:** Uses only signal-safe operations
- **Resource Protection:** Prevents resource corruption during signal handling
- **Clean Shutdown:** Coordinates shutdown across all system components

**Used By:** Operating system signal delivery, system administration

**Dependencies:** Logging system, global state management

### 2. Install Signal Handlers (`install_signal_handlers`)
**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Configures comprehensive signal handling for robust server operation and administrative control.

**Process:**
1. **Signal Action Configuration:**
   - Configures SA_SIGINFO for detailed signal information
   - Sets SA_RESTART for automatic system call restart
   - Clears signal mask for proper signal delivery

2. **Signal Handler Registration:**
   - **SIGTERM:** Graceful termination signal from system
   - **SIGINT:** User interrupt signal (Ctrl-C)
   - **SIGHUP:** Hangup signal for configuration reload
   - **SIGPIPE:** Broken pipe signal for network error handling
   - **SIGUSR1:** User-defined signal for manual synchronization

3. **Error Handling:**
   - Validates each signal handler installation
   - Returns failure if any handler installation fails
   - Logs successful installation for verification

**Operational Benefits:**
- **Graceful Shutdown:** Allows proper cleanup on termination
- **Administrative Control:** Provides administrative signal interface
- **Network Resilience:** Handles network-related signals properly
- **Development Support:** Enables debugging and development workflows

**Used By:** Server initialization, system administration

**Dependencies:** POSIX signal handling

### 3. Main Function (`main`)
**Parameters:**
- Argument count (integer)
- Argument vector (string array)

**Returns:** Integer exit code

**Purpose:** Primary server initialization function that orchestrates startup of all system components in correct order with comprehensive error handling.

**Process:**

#### Phase 1: Core System Initialization
1. **Logging System:**
   - **First Priority:** Initializes logging before any other operations
   - Enables error reporting for subsequent initialization steps
   - Critical for debugging initialization failures

2. **Build Information:**
   - Logs server start with build timestamp
   - Provides version tracking and deployment verification
   - Essential for support and debugging

3. **Configuration Loading:**
   - Reads configuration from file system
   - Validates all configuration parameters
   - Critical for all subsequent initialization

4. **Signal Handler Installation:**
   - Installs comprehensive signal handling
   - Enables graceful shutdown and administrative control
   - Essential for production operation

#### Phase 2: Hardware and Security Initialization
1. **Hardware AES Detection:**
   - **Optimization Check:** Detects Intel AES-NI instruction support
   - **Performance Enhancement:** Enables hardware acceleration when available
   - **Security Optimization:** Uses fastest available encryption

#### Phase 3: Database and Storage Systems
1. **Database Initialization:**
   - **OPTIMIZATION:** Initializes on-demand page cache system
   - **Background Threads:** Starts persistence and eviction threads
   - **Performance Critical:** Foundation for all coin operations
   - **Memory Efficiency:** Dramatically reduces memory usage

#### Phase 4: Memory and Resource Management
1. **Ticket Storage:**
   - Initializes ticket memory pool for healing operations
   - Enables distributed authentication across RAIDA network
   - Critical for healing and consensus operations

2. **Index Systems:**
   - **Locker Index:** Initializes coin locker indexing system
   - **Crossover Index:** Initializes crossover operation indexing
   - **Performance Critical:** Enables fast coin lookup operations

3. **Statistics System:**
   - Initializes performance and operational metrics
   - Enables monitoring and performance analysis
   - Critical for production monitoring

4. **IP Hash Table:**
   - Initializes rate limiting and abuse prevention
   - Protects system from denial-of-service attacks
   - Essential for production security

#### Phase 5: Advanced Systems
1. **Integrity System:**
   - **NEW FEATURE:** Initializes Merkle Tree integrity system
   - **Background Processing:** Starts integrity verification thread
   - **Network Healing:** Enables distributed healing capabilities
   - **Data Integrity:** Ensures data consistency across RAIDA network

#### Phase 6: Threading and Network Infrastructure
1. **Thread Pool Configuration:**
   - **Intelligent Sizing:** Uses configured threads or auto-detects CPU cores
   - **Performance Optimization:** Defaults to THPOOL_SIZE with CPU detection
   - **Scalability:** Adapts to available hardware resources
   - **Load Distribution:** Enables concurrent request processing

2. **Network Infrastructure:**
   - **Event-Driven I/O:** Initializes epoll-based network handling
   - **Protocol Support:** Enables TCP and UDP protocol handling
   - **Performance Critical:** Handles all client communication
   - **Blocking Operation:** Main execution blocks here serving requests

#### Error Handling Strategy
- **Early Termination:** Any initialization failure terminates server
- **Clear Error Messages:** Descriptive error messages for each failure
- **Resource Cleanup:** Proper cleanup on initialization failures
- **Exit Codes:** Consistent exit codes for monitoring systems

**System Architecture Benefits:**
- **Modular Initialization:** Clean separation of initialization phases
- **Dependency Management:** Proper initialization order respect dependencies
- **Resource Optimization:** Efficient resource allocation and management
- **Error Resilience:** Comprehensive error handling throughout initialization

**Performance Optimizations:**
- **Hardware Detection:** Automatic optimization for available hardware
- **Dynamic Sizing:** Adaptive resource allocation based on system capabilities
- **Cache Systems:** Multiple levels of caching for optimal performance
- **Thread Management:** Intelligent thread pool sizing and management

**Used By:** System startup, process management, service managers

**Dependencies:** All system modules and subsystems

## Initialization Sequence and Dependencies

### Critical Path Dependencies
1. **Logging → Configuration → Signal Handlers**
2. **Database → Indexes → Statistics**
3. **Network → Thread Pool → Main Loop**

### Parallel Initialization Opportunities
- **Background Threads:** Database persistence, integrity checking
- **Independent Systems:** Statistics, rate limiting, ticket management
- **Optimization Systems:** Hardware detection, performance tuning

### Error Recovery Points
- **Early Failures:** Logging and configuration errors are fatal
- **Resource Failures:** Memory allocation failures are fatal
- **Network Failures:** Network initialization failures are fatal
- **Graceful Degradation:** Some subsystems can operate with reduced functionality

## Hardware Optimization Integration

### AES Hardware Acceleration
- **Detection:** Automatic detection of Intel AES-NI instructions
- **Optimization:** Enables hardware-accelerated encryption when available
- **Fallback:** Graceful fallback to software encryption
- **Performance:** Significant performance improvement for cryptographic operations

### CPU Core Detection
- **Dynamic Scaling:** Thread pool size adapts to available CPU cores
- **Performance:** Optimal utilization of available processing power
- **Resource Management:** Prevents over-subscription of CPU resources
- **Scalability:** Automatically scales with hardware upgrades

### Memory Optimization
- **On-Demand Loading:** Database pages loaded only when needed
- **Cache Management:** Intelligent cache sizing and eviction
- **Memory Bounds:** All memory usage is bounded and predictable
- **Resource Monitoring:** Memory usage tracking and optimization

## Production Deployment Features

### Service Integration
- **Signal Handling:** Compatible with systemd and other service managers
- **Process Management:** Proper daemon behavior and resource management
- **Logging Integration:** Structured logging for system log integration
- **Configuration Management:** File-based configuration with validation

### Monitoring and Observability
- **Build Tracking:** Build timestamp logging for deployment verification
- **System Information:** Hardware capability detection and logging
- **Performance Metrics:** Comprehensive metrics collection and reporting
- **Health Monitoring:** System health indicators and status reporting

### Security Hardening
- **Signal Security:** Secure signal handling prevents signal-based attacks
- **Resource Limits:** All resource usage is bounded and controlled
- **Error Handling:** Secure error handling prevents information leakage
- **Access Control:** Administrative functions require proper authentication

## Development and Debugging Support

### Debug Information
- **Build Timestamps:** Precise build identification for debugging
- **Initialization Logging:** Detailed logging of initialization process
- **Component Status:** Status reporting for each initialized component
- **Error Diagnostics:** Comprehensive error reporting and diagnostics

### Development Workflow
- **Signal-Based Control:** SIGUSR1 for manual synchronization triggers
- **Graceful Shutdown:** Clean shutdown for development and testing
- **Resource Cleanup:** Proper cleanup enables rapid restart cycles
- **Configuration Flexibility:** File-based configuration enables easy testing

## Error Handling and Recovery

### Initialization Failure Handling
- **Early Detection:** Failures detected before system becomes operational
- **Clear Messaging:** Descriptive error messages for each failure type
- **Resource Cleanup:** Proper cleanup prevents resource leaks
- **Exit Code Standards:** Standard exit codes for monitoring integration

### Runtime Error Handling
- **Signal-Based Shutdown:** Graceful shutdown on system signals
- **Resource Monitoring:** Continuous monitoring of resource usage
- **Error Recovery:** Automatic recovery from transient failures
- **Audit Logging:** Comprehensive logging for post-incident analysis

### System Recovery
- **Clean Restart:** System designed for clean restart after failures
- **State Recovery:** Persistent state recovered from disk storage
- **Configuration Reload:** Configuration can be reloaded without restart
- **Component Resilience:** Individual component failures don't crash system

## Dependencies and Integration

### System Dependencies
- **Operating System:** Linux with epoll support, POSIX signals, threading
- **Hardware:** x86/x64 processors with optional AES-NI instruction support
- **File System:** POSIX-compliant file system for data storage
- **Network:** TCP/UDP network stack for client and inter-RAIDA communication
- **Memory:** Sufficient RAM for page cache and thread pools

### Module Dependencies
- **Configuration System:** System settings and operational parameters
- **Logging System:** Thread-safe logging infrastructure
- **Database Layer:** On-demand page cache and persistence
- **Network Layer:** Event-driven I/O and protocol handling
- **Thread Pool:** Concurrent request processing
- **Security Systems:** Cryptographic functions and authentication

### Integration Points
- **Service Managers:** Compatible with systemd, init systems
- **Monitoring Systems:** Metrics export and health checking
- **Administrative Tools:** Configuration management and system control
- **Client Applications:** Network API for all client operations
- **RAIDA Network:** Inter-server communication and consensus

## Performance Characteristics

### Startup Performance
- **Fast Initialization:** Optimized initialization sequence
- **Parallel Loading:** Concurrent initialization where possible
- **Resource Pre-allocation:** Pre-allocates critical resources
- **Cache Warming:** Intelligent cache warming strategies

### Runtime Performance
- **Event-Driven Architecture:** Non-blocking I/O for maximum throughput
- **Thread Pool Management:** Optimal thread utilization
- **Memory Efficiency:** Bounded memory usage with intelligent caching
- **Hardware Acceleration:** Automatic use of available hardware features

### Scalability Features
- **CPU Scaling:** Automatic adaptation to available CPU cores
- **Memory Scaling:** Dynamic memory allocation based on workload
- **Network Scaling:** High-concurrency network handling
- **Load Distribution:** Even distribution of processing load

## Security Considerations

### Process Security
- **Signal Handling:** Secure signal processing prevents signal-based attacks
- **Resource Limits:** All resource usage bounded to prevent exhaustion
- **Error Handling:** Secure error handling prevents information disclosure
- **Memory Protection:** Proper memory management prevents buffer overflows

### Network Security
- **Protocol Validation:** All network protocols strictly validated
- **Rate Limiting:** Built-in protection against denial-of-service attacks
- **Authentication:** Strong authentication for administrative operations
- **Encryption:** Hardware-accelerated encryption when available

### Data Security
- **Access Control:** Strict access controls on all data operations
- **Audit Logging:** Comprehensive audit trail for security monitoring
- **Data Integrity:** Cryptographic integrity verification
- **Secure Storage:** Secure storage of sensitive configuration data

## Operational Considerations

### System Administration
- **Configuration Management:** File-based configuration with validation
- **Process Control:** Standard signal-based process control
- **Log Management:** Structured logging compatible with log management systems
- **Health Monitoring:** Built-in health checks and status reporting

### Maintenance Operations
- **Graceful Shutdown:** Clean shutdown preserves system state
- **Configuration Reload:** Dynamic configuration updates
- **Resource Monitoring:** Real-time resource usage monitoring
- **Performance Tuning:** Runtime performance optimization

### Backup and Recovery
- **State Persistence:** Critical state persisted to disk storage
- **Clean Restart:** System designed for clean restart after failures
- **Data Recovery:** Robust data recovery mechanisms
- **Disaster Recovery:** Support for disaster recovery procedures

## Monitoring and Diagnostics

### System Metrics
- **Performance Metrics:** CPU, memory, network, and I/O statistics
- **Operational Metrics:** Request rates, response times, error rates
- **Resource Metrics:** Thread pool utilization, cache hit rates
- **System Health:** Overall system health and component status

### Diagnostic Information
- **Build Information:** Version, build time, and configuration details
- **Hardware Information:** CPU features, memory, and system capabilities
- **Component Status:** Detailed status of all system components
- **Error Diagnostics:** Comprehensive error tracking and analysis

### Alerting Integration
- **Status Reporting:** Machine-readable status information
- **Error Reporting:** Structured error information for alerting systems
- **Performance Thresholds:** Configurable performance thresholds
- **Health Checks:** Built-in health check endpoints

## Development and Testing Support

### Development Features
- **Debug Logging:** Comprehensive debug information
- **Signal Control:** Development-friendly signal handling
- **Configuration Flexibility:** Easy configuration for testing
- **Resource Monitoring:** Real-time resource usage visibility

### Testing Integration
- **Clean Startup/Shutdown:** Reliable startup and shutdown for testing
- **Configuration Validation:** Comprehensive configuration validation
- **Error Simulation:** Support for error injection and testing
- **Performance Testing:** Built-in performance measurement tools

### Debugging Support
- **Detailed Logging:** Comprehensive logging for debugging
- **Component Isolation:** Individual component status and control
- **Error Tracking:** Detailed error tracking and reporting
- **State Inspection:** Runtime state inspection capabilities

## Future Evolution and Extensibility

### Architectural Flexibility
- **Modular Design:** Clean separation between system components
- **Plugin Architecture:** Support for future plugin systems
- **Protocol Evolution:** Support for protocol version evolution
- **Feature Flags:** Built-in support for feature flag systems

### Performance Evolution
- **Hardware Adaptation:** Automatic adaptation to new hardware features
- **Algorithm Updates:** Support for updated cryptographic algorithms
- **Optimization Opportunities:** Continuous performance optimization
- **Scalability Improvements:** Support for increased scale requirements

### Integration Evolution
- **Service Integration:** Enhanced integration with modern service architectures
- **Monitoring Evolution:** Support for evolved monitoring and observability
- **Security Evolution:** Adaptation to evolving security requirements
- **Operational Evolution:** Support for evolved operational practices

This main entry point module provides comprehensive server initialization and management with advanced features including hardware optimization, security hardening, performance monitoring, and production-ready operational capabilities, serving as the foundation for a robust, scalable, and maintainable RAIDA server system.