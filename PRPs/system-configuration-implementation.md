# Configuration Management Implementation (config.c)

## Module Purpose
This module implements comprehensive configuration file processing for the RAIDA server system using TOML format. It handles mandatory and optional server parameters, network address resolution, security key management, and provides secure configuration validation with enhanced security measures including mandatory authentication keys.

## Core Functionality

### 1. Primary Configuration Reader (`read_config`)
**Parameters:**
- Binary path string pointer (path to executable for determining config file location)

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Reads and parses the complete server configuration from TOML file, validates all parameters, resolves network addresses, and populates global configuration structure.

**Process:**
1. **File Location and Access:**
   - Extracts directory path from binary location
   - Constructs configuration file path using CONFIG_FILE_NAME constant
   - Opens TOML configuration file for reading
   - Initializes default values for optional parameters

2. **TOML Parsing and Validation:**
   - Parses TOML file using structured parser with error reporting
   - Validates presence of mandatory configuration sections
   - Extracts server configuration from [server] section
   - Handles parsing errors with detailed error messages

3. **Mandatory Parameter Processing:**
   - **RAIDA Server ID:** Unique identifier for this server instance
   - **Coin ID:** Identifier for managed coin type
   - **Network Port:** Listening port for client connections
   - **Proxy Key:** 32-character hexadecimal authentication key (mandatory security enhancement)
   - **Admin Key:** 32-character hexadecimal administrative key (mandatory security enhancement)

4. **Optional Parameter Processing:**
   - **Thread Count:** Worker thread pool size (defaults to system detection)
   - **Backup Frequency:** Database synchronization interval (defaults to DEFAULT_FLUSH_FREQ)
   - **Integrity Check Frequency:** Data validation interval (defaults to DEFAULT_INTEGRITY_FREQ)
   - **UDP Payload Threshold:** Maximum UDP packet size (defaults to DEFAULT_UDP_PAYLOAD_THRESHOLD)
   - **Proxy Address:** External proxy server address (defaults to DEFAULT_PROXY_ADDR)
   - **Proxy Port:** External proxy server port (defaults to DEFAULT_PROXY_PORT)
   - **BTC Confirmations:** Required blockchain confirmations (defaults to 2)

5. **Security Key Processing:**
   - Validates key length (exactly 32 hexadecimal characters)
   - Converts hexadecimal strings to binary format
   - Stores keys securely in configuration structure
   - Validates all characters are valid hexadecimal digits

6. **Network Address Resolution:**
   - Resolves proxy server hostname to IPv4 address
   - Processes RAIDA server array (25 servers total)
   - For each RAIDA server:
     - Parses host:port format
     - Resolves hostname to IPv4 address using DNS
     - Creates socket address structures for network operations
     - Stores resolved addresses and port numbers

7. **Configuration Validation:**
   - Verifies all mandatory parameters are present
   - Validates parameter ranges and formats
   - Ensures network addresses are resolvable
   - Confirms security key format compliance

8. **Memory Management:**
   - Allocates dynamic memory for resolved addresses
   - Manages TOML parser memory lifecycle
   - Handles cleanup on error conditions
   - Stores string references appropriately

**Dependencies:**
- TOML parsing library for configuration file processing
- DNS resolution utilities for address lookup
- Memory management for dynamic allocations
- Logging system for error reporting
- String processing utilities for key validation

### 2. Configuration Display (`dump_config`)
**Parameters:** None

**Returns:** None

**Purpose:** Outputs current configuration parameters to debug log for verification and troubleshooting.

**Process:**
1. Displays core server identification (RAIDA number, port)
2. Shows operational parameters (working directory, frequencies)
3. Reports network configuration details
4. Logs configuration validation completion

**Dependencies:**
- Logging system for structured output
- Global configuration structure access

## Configuration Structure and Parameters

### Mandatory Configuration Elements

#### Server Identification
- **raida_id:** Integer identifier (0-24) for this RAIDA server instance
- **coin_id:** 8-bit identifier for the managed coin type
- **port:** Network port number for client connections

#### Security Authentication (Enhanced Security)
- **proxy_key:** 16-byte binary key derived from 32-character hex string
- **admin_key:** 16-byte binary key derived from 32-character hex string
- Both keys are now mandatory (security enhancement from original implementation)

#### Network Configuration
- **raida_servers:** Array of 25 "host:port" strings for peer RAIDA servers

### Optional Configuration Elements

#### Performance Tuning
- **threads:** Worker thread pool size (defaults to system-appropriate value)
- **backup_freq:** Database flush frequency in seconds
- **integrity_freq:** Integrity check frequency in seconds  
- **udp_effective_payload:** Maximum UDP payload size in bytes

#### External Services
- **proxy_addr:** Hostname/IP of external proxy server
- **proxy_port:** Port number for external proxy service
- **btc_confirmations:** Required Bitcoin confirmation count

### Default Values and Constants
- `DEFAULT_FLUSH_FREQ`: Database synchronization frequency
- `DEFAULT_INTEGRITY_FREQ`: Data validation frequency  
- `DEFAULT_UDP_PAYLOAD_THRESHOLD`: UDP packet size limit
- `DEFAULT_PROXY_ADDR`: Default proxy server address
- `DEFAULT_PROXY_PORT`: Default proxy server port

## File Format and Structure

### File Location
- **Filename:** CONFIG_FILE_NAME constant (typically "config.toml")
- **Location:** Same directory as server executable
- **Access:** Read-only during startup phase

## Security Enhancements

### Mandatory Authentication Keys
- **Breaking Change:** Proxy and admin keys are now required (not optional)
- **Validation:** Strict format validation prevents weak key usage
- **Storage:** Binary conversion and secure memory handling
- **Error Handling:** Server refuses to start with invalid or missing keys

### Key Format Requirements
- **Length:** Exactly 32 hexadecimal characters
- **Character Set:** Valid hexadecimal digits (0-9, A-F, case insensitive)
- **Conversion:** Hex string converted to 16-byte binary format
- **Validation:** Each character pair validated during conversion

### Configuration Security
- **File Permissions:** Configuration file should have restricted access
- **Key Management:** No hardcoded fallback keys (security improvement)
- **Error Reporting:** Failed authentication logged for security audit

## Network Address Resolution

### DNS Resolution Process
- **Address Family:** IPv4 only (AF_INET) for simplicity
- **Socket Type:** Stream sockets (SOCK_STREAM) for TCP
- **Resolution:** Uses getaddrinfo() for robust address lookup
- **Validation:** Ensures all addresses resolve successfully before startup

### RAIDA Server Array
- **Size:** Exactly 25 servers (TOTAL_RAIDA_SERVERS constant)
- **Format:** Each entry as "hostname:port" string
- **Resolution:** Each hostname resolved to IPv4 address
- **Storage:** Resolved addresses stored as socket address structures

### Error Handling
- **DNS Failures:** Resolution failures prevent server startup
- **Invalid Formats:** Malformed host:port entries rejected
- **Network Issues:** Connection problems reported with context

**Error Categories**:
- **File Access Errors**: Configuration file reading and access problems
- **Parsing Errors**: TOML syntax and structure errors
- **Validation Errors**: Parameter validation and format errors
- **Network Errors**: DNS resolution and address conversion errors
- **Memory Errors**: Memory allocation and resource management errors

**Error Response Strategy**:
- **Immediate Termination**: Stop processing on critical errors
- **Descriptive Logging**: Provide detailed error messages for troubleshooting
- **Resource Cleanup**: Ensure proper cleanup on all error paths
- **Security Considerations**: Avoid sensitive information disclosure in error messages


## Error Handling and Validation

### Configuration File Errors
- **Missing File:** File not found in expected location
- **Parse Errors:** TOML syntax or structure problems
- **Missing Sections:** Required [server] section not present

### Parameter Validation Errors
- **Missing Mandatory:** Required parameters not specified
- **Invalid Ranges:** Numeric parameters outside valid ranges
- **Format Errors:** String parameters in incorrect format
- **Key Validation:** Authentication keys with invalid format

### Network Resolution Errors
- **DNS Failures:** Hostname resolution problems
- **Invalid Addresses:** Malformed network addresses
- **Connection Issues:** Network connectivity problems during resolution

### Recovery and Reporting
- **Error Context:** Detailed error messages with parameter names
- **Cleanup:** Proper resource cleanup on failure paths
- **Logging:** Comprehensive error reporting for troubleshooting

## Dependencies and Integration

### Required External Libraries
- **TOML Parser:** Library for TOML file format processing
- **Network APIs:** DNS resolution and socket address management
- **Memory Management:** Dynamic allocation for resolved addresses
- **String Processing:** Parsing and validation utilities

### System Dependencies
- **File System:** Configuration file access and reading
- **Network Stack:** DNS resolution and address validation
- **Memory Allocation:** Dynamic memory for configuration data

### Integration Points
- **Logging System:** Error reporting and debug output
- **Main Application:** Configuration data used throughout server
- **Network Layer:** Resolved addresses used for peer communication
- **Security Systems:** Authentication keys used for authorization

### Used By
- **Server Initialization:** Configuration loaded during startup
- **Network Operations:** Peer server addresses and ports
- **Security Operations:** Authentication and authorization systems
- **Administrative Tools:** Server identification and parameters

## Threading and Lifecycle

### Initialization Phase
- **Single Thread:** Configuration loaded during single-threaded startup
- **Blocking Operation:** Server waits for complete configuration loading
- **Error Termination:** Invalid configuration prevents server startup

### Runtime Access
- **Read-Only:** Configuration treated as immutable after loading
- **Thread-Safe:** Multiple threads can safely read configuration data
- **No Modifications:** Configuration not changed during operation

### Memory Management
- **Static Allocation:** Main configuration structure statically allocated
- **Dynamic Elements:** Resolved addresses allocated dynamically
- **Lifetime:** Configuration persists for entire server lifetime
**Memory Operations**:
- **String Duplication**: Proper duplication of configuration strings
- **Address Allocation**: Dynamic allocation for socket address structures
- **TOML Memory**: Proper cleanup of TOML parser allocated memory
- **Error Path Cleanup**: Ensure cleanup occurs on all error exit paths
**Resource Management**:
- **File Handles**: Proper closing of configuration file handles
- **Network Resources**: Cleanup of address resolution resources
- **Dynamic Allocation**: Track and cleanup all dynamically allocated memory
- **Resource Verification**: Verify successful resource allocation before use

## 6. Configuration Display and Debugging

### 6.1 Configuration Dump Implementation
**Purpose**: Provide debugging output for configuration verification.

**Display Information**:
- **RAIDA Identification**: Display RAIDA number and coin identifier
- **Network Configuration**: Show port and network settings
- **Performance Settings**: Display frequency and threshold parameters
- **Working Directory**: Show resolved working directory path

**Debug Output**:
- **Structured Display**: Organize configuration display for readability
- **Parameter Verification**: Allow verification of loaded configuration
- **Troubleshooting Aid**: Assist in configuration troubleshooting
- **Security Consideration**: Avoid displaying sensitive key material

This module provides the foundation for secure, validated server configuration management, ensuring all required parameters are present and properly formatted before server operation begins.




