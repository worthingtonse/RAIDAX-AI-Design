# Configuration Management System (config.c)

## Module Purpose
This module implements comprehensive configuration management for RAIDA servers, parsing TOML configuration files, validating network settings, resolving RAIDA server addresses, and managing security keys. It provides centralized configuration with mandatory security enforcement, network topology management, and feature flags for controlled system deployment.

## Constants and Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `CONFIG_FILE_NAME` | "config.toml" | Name of the main configuration file |
| `DEFAULT_PROXY_PORT` | 50000 | Default port for proxy server connections |
| `DEFAULT_PROXY_ADDR` | "swap.cloudcoin.org" | Default proxy server address |
| `DEFAULT_FLUSH_FREQ` | Variable | Default frequency for database flush operations (seconds) |
| `DEFAULT_INTEGRITY_FREQ` | Variable | Default frequency for integrity checking (seconds) |
| `DEFAULT_UDP_PAYLOAD_THRESHOLD` | Variable | Default UDP payload size threshold (bytes) |
| `TOTAL_RAIDA_SERVERS` | 25 | Total number of RAIDA servers in the network |


## Core Functionality

### 1. Read Configuration (`read_config`)
**Parameters:**
- Binary path string containing the path to the executable

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Parses the TOML configuration file, validates all settings, resolves network addresses, and populates the global configuration structure with validated settings.

**Process:**
1. **File Location and Access:**
   - Determines configuration file path relative to binary location
   - Opens and reads TOML configuration file
   - Handles file access errors gracefully

2. **TOML Parsing:**
   - Parses TOML file using external TOML library
   - Validates TOML syntax and structure
   - Reports parsing errors with specific line information

3. **Mandatory Field Validation:**
   - **raida_id:** Validates server identifier within network range
   - **coin_id:** Validates coin type identifier
   - **port:** Validates network port number
   - **proxy_key:** Validates 32-character hexadecimal key format
   - **admin_key:** Validates 32-character hexadecimal key format

4. **Optional Field Processing:**
   - **threads:** Worker thread count (defaults to auto-detection)
   - **proxy_addr:** Proxy server address (defaults to DEFAULT_PROXY_ADDR)
   - **proxy_port:** Proxy server port (defaults to DEFAULT_PROXY_PORT)
   - **backup_freq:** Database flush frequency
   - **integrity_freq:** Integrity checking frequency
   - **synchronization_enabled:** Integrity system master switch
   - **udp_effective_payload:** UDP protocol threshold

5. **Network Address Resolution:**
   - Resolves proxy server hostname to IP address
   - Handles DNS resolution failures
   - Converts hostname to IPv4 address for networking

6. **RAIDA Network Topology:**
   - Parses array of 25 RAIDA server addresses
   - Splits host:port pairs for each server
   - Resolves each hostname to IP address
   - Creates socket address structures for network communication
   - Validates complete network topology

7. **Security Key Processing:**
   - Converts hexadecimal key strings to binary format
   - Validates key format and length
   - Stores keys securely in configuration structure

**Security Features:**
- Mandatory security key validation prevents startup with default/missing keys
- DNS resolution validation prevents configuration with invalid addresses
- Complete network topology validation ensures connectivity capability

**Used By:** Server initialization, startup validation

**Dependencies:** TOML parsing library, DNS resolution system

### 2. Dump Configuration (`dump_config`)
**Parameters:** None

**Returns:** None

**Purpose:** Outputs current configuration settings to debug log for verification and troubleshooting.

**Process:**
1. **Core Settings Display:**
   - RAIDA server identifier
   - Listening port number
   - Working directory path

2. **System Tuning Display:**
   - Database flush frequency
   - Integrity checking frequency
   - Synchronization system status (enabled/disabled)
   - UDP payload threshold

**Security Features:**
- Does not display sensitive security keys
- Safe for inclusion in log files
- Provides verification without exposing secrets

**Used By:** Debug logging, configuration verification

## Data Structures and Formats

### Configuration File Format (TOML)
```toml
[server]
raida_id = 0
coin_id = 1
port = 25000
threads = 8
proxy_key = "0123456789abcdef0123456789abcdef"
admin_key = "fedcba9876543210fedcba9876543210"
proxy_addr = "proxy.example.com"
proxy_port = 50000
backup_freq = 300
integrity_freq = 3600
synchronization_enabled = true
udp_effective_payload = 1024
btc_confirmations = 6
raida_servers = [
    "raida0.example.com:25000",
    "raida1.example.com:25000",
    # ... 23 more entries
]
```

## Security Considerations

### Mandatory Security Keys
- **Removed Hardcoded Keys:** All default keys removed from source code
- **Configuration Required:** Server will not start without valid keys in configuration
- **Format Validation:** Keys must be exactly 32 hexadecimal characters
- **Binary Conversion:** Keys stored in binary format for efficient use

### Network Security
- **Address Validation:** All network addresses resolved and validated at startup
- **DNS Security:** DNS resolution performed once at startup to prevent runtime attacks
- **Complete Topology:** Full network topology required for proper operation

### Configuration File Security
- **Sensitive Data:** Configuration file contains sensitive keys
- **File Permissions:** Should be protected with appropriate file system permissions
- **Backup Security:** Configuration backups must protect sensitive keys

## Error Handling and Validation

### File System Errors
- **File Not Found:** Clear error message when configuration file missing
- **Permission Denied:** Proper error handling for file access issues
- **Directory Resolution:** Graceful handling of binary path resolution errors

### Network Resolution Errors
- **DNS Failures:** Clear error messages for hostname resolution failures
- **IPv4 Requirement:** Validation that resolved addresses are IPv4
- **Network Validation:** Complete validation of all 25 RAIDA server addresses

### Configuration Validation Errors
- **Missing Required Fields:** Clear identification of missing mandatory settings
- **Format Validation:** Detailed validation of key formats and network addresses
- **Range Validation:** Validation of numeric fields within acceptable ranges

## Performance Characteristics

### Startup Optimization
- **Single Parse:** Configuration parsed once at startup
- **Pre-Resolution:** Network addresses resolved at startup for runtime efficiency
- **Memory Efficiency:** Configuration stored in efficient binary format

### Runtime Access
- **Global Structure:** Configuration accessible globally without runtime parsing
- **Binary Keys:** Security keys stored in binary format for efficient cryptographic use
- **Cached Addresses:** Network addresses cached for efficient connection establishment

## Dependencies and Integration

### Required External Libraries
- **TOML Parser:** External TOML parsing library for configuration file processing
- **DNS Resolution:** System DNS resolution capabilities
- **Network Stack:** Socket address structure support

### Used By
- **Server Initialization:** Primary configuration source for all server components
- **Network Layer:** RAIDA server addresses and communication settings
- **Security Systems:** Authentication keys for admin and proxy operations
- **Database Layer:** Flush frequency and working directory settings
- **Integrity System:** Master switch and frequency settings

### Cross-File Dependencies
- **All Modules:** Configuration provides global settings for entire system
- **Network Components:** Server addresses and communication parameters
- **Security Components:** Authentication keys and security settings
- **Database Components:** Working directory and persistence settings

## Feature Flags and Deployment Control

### Integrity System Control
- **Master Switch:** `synchronization_enabled` controls entire integrity system
- **Gradual Rollout:** Allows deployment with integrity system disabled
- **Runtime Safety:** System operates safely with integrity features disabled

### Performance Tuning
- **Frequency Controls:** Configurable timing for various background operations
- **Threshold Controls:** Configurable limits for protocol switching and resource usage
- **Thread Controls:** Configurable worker thread allocation

## Backward Compatibility

### Configuration Evolution
- **Optional Fields:** New configuration fields default to safe values
- **Version Tolerance:** System operates with older configuration files
- **Migration Support:** Supports gradual configuration file updates

### Default Values
- **Safe Defaults:** All optional fields have safe default values
- **Performance Defaults:** Default values chosen for optimal performance
- **Security Defaults:** Security features default to safest configuration

## Threading and Concurrency

### Read-Only After Initialization
- **Initialization Phase:** Configuration modified only during startup
- **Runtime Phase:** Configuration treated as read-only for thread safety
- **Global Access:** Safe concurrent access after initialization complete

### Memory Management
- **Static Allocation:** Configuration uses static/global allocation
- **No Runtime Allocation:** No dynamic memory allocation during runtime access
- **Resource Safety:** No cleanup required for configuration data

This configuration management module provides the foundation for secure and reliable RAIDA server operation, ensuring proper network topology, security key management, and system tuning while supporting controlled feature deployment and backward compatibility.