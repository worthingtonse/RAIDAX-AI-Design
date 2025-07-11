# Configuration Header Definitions (config.h)

## Module Purpose
This header file defines the configuration data structures, constants, and interface for the RAIDA server configuration system. It establishes the global configuration structure that holds all server parameters, network settings, security keys, and operational parameters. The module includes enhanced security measures with mandatory authentication keys and comprehensive network configuration management.

## Configuration Structure Definition

### Primary Configuration Structure (`config_s`)
**Purpose:** Central repository for all server configuration parameters, designed for thread-safe read access after initialization.

#### Server Identity Parameters
- **raida_no:** Integer identifier for this RAIDA server (0-24 range)
- **port:** Network port number for client connections
- **coin_id:** 8-bit identifier for managed coin type
- **cwd:** String pointer to current working directory path

#### Security Authentication Keys (Enhanced Security)
- **admin_key:** 16-byte binary administrative authentication key
- **proxy_key:** 16-byte binary proxy service authentication key
- Both keys are mandatory (security enhancement) - no default values provided

#### Performance and Operational Parameters
- **flush_freq:** Database synchronization frequency in seconds
- **integrity_freq:** Data integrity check frequency in seconds
- **udp_payload_threshold:** Maximum UDP packet size in bytes
- **threads:** Worker thread pool size (8-bit value)

#### Network Configuration Arrays
- **raida_servers:** Array of 25 string pointers to peer server hostnames
- **raida_servers_ports:** Array of 25 16-bit port numbers for peer servers
- **raida_addrs:** Array of 25 socket address structure pointers (resolved addresses)

#### External Service Configuration
- **proxy_addr:** String pointer to external proxy server address
- **proxy_port:** Integer port number for proxy service
- **btc_confirmations:** 8-bit count of required Bitcoin confirmations

## Constants and Default Values

### File System Constants
- **CONFIG_FILE_NAME:** Name of configuration file (typically "config.toml")
- **File Location:** Configuration file must be in same directory as executable

### Default Operational Parameters
- **DEFAULT_PROXY_PORT:** Default port for external proxy service (50000)
- **DEFAULT_PROXY_ADDR:** Default proxy server address ("swap.cloudcoin.org")

### Removed Hardcoded Security Elements
- **Security Enhancement:** Removed DEFAULT_ADMIN_KEY and DEFAULT_PROXY_KEY macros
- **Mandatory Configuration:** Admin and proxy keys must be specified in configuration file
- **No Fallbacks:** Server will not start with missing or invalid authentication keys

## Function Interface Declarations

### Configuration Processing Functions

#### `read_config`
**Parameters:** Character pointer to binary path (executable location)
**Returns:** Integer status code (0 for success, negative for error)
**Purpose:** Primary configuration loading and parsing function

#### `dump_config`
**Parameters:** None
**Returns:** None  
**Purpose:** Debug output function for configuration verification

## Security Model and Enhancements

### Authentication Key Management
- **Mandatory Keys:** Both admin_key and proxy_key are required in configuration
- **Format Requirements:** Keys must be exactly 32 hexadecimal characters
- **Binary Storage:** Keys converted from hex strings to 16-byte binary format
- **No Defaults:** Eliminates security risk from hardcoded keys

### Key Usage Patterns
- **Admin Key:** Used for administrative command authentication
- **Proxy Key:** Used for external proxy service authentication
- **Validation:** Keys validated during configuration loading phase

### Security Improvements
- **Eliminated Hardcoded Keys:** Previous versions had hardcoded fallback keys
- **Mandatory Specification:** Forces explicit key configuration
- **Format Validation:** Ensures keys meet cryptographic standards

## Network Configuration Model

### RAIDA Peer Network
- **Server Count:** Exactly 25 peer RAIDA servers (TOTAL_RAIDA_SERVERS)
- **Address Resolution:** Hostnames resolved to IPv4 addresses during startup
- **Storage Format:** Parallel arrays for hostnames, ports, and resolved addresses
- **Access Pattern:** Read-only access after initialization phase

### External Service Integration
- **Proxy Configuration:** External proxy server for blockchain operations
- **DNS Resolution:** All hostnames resolved to IP addresses during startup
- **Connection Management:** Resolved addresses used for network operations

## Data Structure Memory Management

### Static vs Dynamic Allocation
- **Main Structure:** Static global allocation for primary configuration
- **String Storage:** TOML library manages string memory lifecycle
- **Resolved Addresses:** Dynamic allocation for socket address structures
- **Array Management:** Fixed-size arrays for peer server information

### Lifecycle Management
- **Initialization:** Configuration loaded once during server startup
- **Runtime:** Read-only access throughout server operation
- **Cleanup:** Dynamic memory freed during server shutdown
- **Thread Safety:** No modifications after initialization phase

## Configuration File Format Requirements

### TOML Structure Expectations
- **Root Section:** [server] section containing all parameters
- **Mandatory Fields:** raida_id, coin_id, port, proxy_key, admin_key, raida_servers
- **Optional Fields:** All other parameters with sensible defaults
- **Array Format:** raida_servers as array of "host:port" strings

### Validation Requirements
- **Type Checking:** All parameters must match expected data types
- **Range Validation:** Numeric parameters within valid ranges
- **Format Validation:** String parameters in expected formats
- **Completeness:** All mandatory parameters must be present

### Error Handling Requirements
- **Parse Errors:** Invalid TOML syntax must be reported clearly
- **Missing Parameters:** Absent mandatory fields cause startup failure
- **Invalid Values:** Out-of-range or malformed values prevent startup
- **Network Errors:** DNS resolution failures block server initialization

## Integration Dependencies

### Required External Definitions
- **Protocol Constants:** TOTAL_RAIDA_SERVERS from protocol definitions
- **System Types:** Socket address structures from network headers
- **Standard Types:** Integer and string types from system headers
- **File System:** Path manipulation and file access capabilities

### Provided to Other Modules
- **Global Configuration:** config structure accessible throughout application
- **Server Identity:** RAIDA number and coin ID for protocol operations
- **Network Parameters:** Peer addresses and ports for communication
- **Security Keys:** Authentication keys for secure operations

### Module Dependencies
- **Configuration Implementation:** Functions defined in config.c
- **TOML Parser:** External library for configuration file processing
- **Network Stack:** DNS resolution and socket address management
- **Logging System:** Error reporting and debug output

## Thread Safety and Access Patterns

### Initialization Phase
- **Single-Threaded:** Configuration loaded before thread pool startup
- **Blocking:** Server initialization waits for complete configuration
- **Validation:** All parameters validated before proceeding

### Runtime Phase
- **Read-Only:** Configuration treated as immutable after loading
- **Multi-Threaded:** Safe concurrent read access from all threads
- **No Locking:** No synchronization needed for read operations

### Error Recovery
- **Startup Failure:** Invalid configuration prevents server startup
- **No Runtime Changes:** Configuration cannot be modified during operation
- **Restart Required:** Configuration changes require server restart

## Usage Patterns and Examples

### Global Access Pattern
```
// Access configuration throughout application
extern struct config_s config;
int server_port = config.port;
char *proxy_address = config.proxy_addr;
```

### Security Key Usage
```
// Authentication using stored keys
if (memcmp(provided_key, config.admin_key, 16) == 0) {
    // Grant administrative access
}
```

### Network Configuration Access
```
// Peer server communication
for (int i = 0; i < TOTAL_RAIDA_SERVERS; i++) {
    connect_to_peer(config.raida_addrs[i], config.raida_servers_ports[i]);
}
```

## Security Considerations

### Key Storage Security
- **Binary Format:** Keys stored as binary data, not strings
- **Memory Protection:** No key copying beyond necessary operations
- **Access Control:** Keys only accessible through configuration structure

### Configuration File Security
- **File Permissions:** Configuration file should have restricted access
- **Key Visibility:** Hexadecimal keys visible in configuration file
- **Secure Storage:** Configuration file should be protected on filesystem

### Runtime Security
- **Immutable Configuration:** No runtime modification prevents tampering
- **Validation Enforcement:** Invalid configurations prevent operation
- **Audit Trail:** Configuration loading logged for security monitoring

## Performance Characteristics

### Initialization Performance
- **One-Time Cost:** Configuration loaded once during startup
- **DNS Resolution:** Network overhead during address resolution
- **Memory Allocation:** Minimal dynamic allocation for addresses

### Runtime Performance
- **Zero Overhead:** No runtime configuration processing
- **Cache-Friendly:** Configuration data locality for frequent access
- **No Synchronization:** No locking overhead for read operations

## Compatibility and Versioning

### Configuration Version Management
- **Format Stability:** TOML format provides structured extensibility
- **Backward Compatibility:** Optional parameters maintain compatibility
- **Default Values:** Sensible defaults for new optional parameters

### Migration Support
- **Parameter Addition:** New optional parameters added with defaults
- **Deprecation:** Old parameters marked deprecated before removal
- **Validation:** Format validation ensures compatibility

This header provides the complete interface for secure, validated configuration management, establishing the foundation for all server operations while maintaining security, performance, and reliability requirements.