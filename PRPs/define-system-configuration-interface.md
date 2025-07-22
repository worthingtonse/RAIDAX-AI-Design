# Configuration Header Definitions (config.h)

## Module Purpose
This header file defines the global configuration structure, default values, and constants for the RAIDA server system. It provides centralized configuration management with mandatory security enforcement, network topology definitions, and feature flags for controlled deployment of system capabilities.

## Constants and Default Values

### Configuration File
| Constant | Value | Description |
|----------|-------|-------------|
| `CONFIG_FILE_NAME` | "config.toml" | Name of the main configuration file located in binary directory |

### Default Network Settings
| Constant | Value | Description |
|----------|-------|-------------|
| `DEFAULT_PROXY_PORT` | 50000 | Default port for proxy server connections |
| `DEFAULT_PROXY_ADDR` | "swap.cloudcoin.org" | Default proxy server hostname |
| `DEFAULT_FLUSH_FREQ` | Variable | Default database backup frequency in seconds |
| `DEFAULT_INTEGRITY_FREQ` | Variable | Default integrity checking frequency in seconds |
| `DEFAULT_UDP_PAYLOAD_THRESHOLD` | Variable | Default UDP payload size threshold for protocol switching |

### Network Topology
| Constant | Value | Description |
|----------|-------|-------------|
| `TOTAL_RAIDA_SERVERS` | 25 | Total number of RAIDA servers in the network topology |

## Security Key Management

### Administrative Key Requirements
- **Mandatory Configuration:** Server will not start without valid admin_key in configuration
- **Format:** 32-character hexadecimal string converted to 16-byte binary
- **Usage:** Required for all executive and administrative operations
- **Storage:** Stored in binary format for efficient cryptographic operations

### Proxy Key Requirements
- **Mandatory Configuration:** Server will not start without valid proxy_key in configuration
- **Format:** 32-character hexadecimal string converted to 16-byte binary
- **Usage:** Required for proxy server authentication and communication
- **Storage:** Stored in binary format for network protocol operations

### Security Enforcement
- **No Default Keys:** All hardcoded default keys removed from source code
- **Configuration Validation:** Keys validated for correct format during startup
- **Binary Conversion:** Hexadecimal strings converted to binary during configuration loading

## Network Topology Management

### RAIDA Server Arrays
- **Hostnames:** Array of hostname/IP address strings for all 25 RAIDA servers
- **Ports:** Array of port numbers corresponding to each RAIDA server
- **Socket Addresses:** Pre-resolved binary socket addresses for efficient connection

### Address Resolution
- **Startup Resolution:** All hostnames resolved to IP addresses during server startup
- **IPv4 Requirement:** All addresses must resolve to valid IPv4 addresses
- **Connection Preparation:** Socket address structures prepared for immediate use

### Network Validation
- **Complete Topology:** All 25 RAIDA server addresses must be configured
- **DNS Validation:** All hostnames must resolve successfully
- **Port Validation:** All port numbers must be valid and accessible

## Feature Control Flags

### Integrity System Control
- **Master Switch:** `synchronization_enabled` controls entire Merkle tree integrity system
- **Deployment Control:** Allows deployment with integrity features disabled
- **Runtime Safety:** System operates safely with integrity features disabled
- **Gradual Rollout:** Enables controlled activation across network

### Performance Tuning Parameters
- **Flush Frequency:** Controls database persistence timing
- **Integrity Frequency:** Controls integrity checking intervals
- **Thread Count:** Controls worker thread pool size
- **UDP Threshold:** Controls UDP vs TCP protocol selection

## Default Value Strategy

### Safe Defaults
- **Performance Defaults:** Default values chosen for optimal performance
- **Security Defaults:** Security features default to safest configuration
- **Compatibility Defaults:** Default values maintain backward compatibility

### Auto-Detection
- **Thread Count:** Zero value enables automatic thread count detection
- **Hardware Adaptation:** System adapts to available hardware resources
- **Performance Optimization:** Automatic optimization based on system capabilities

## Configuration File Structure

### TOML Format Requirements
```toml
[server]
# Mandatory fields
raida_id = 0
coin_id = 1
port = 25000
proxy_key = "32-character-hex-string"
admin_key = "32-character-hex-string"

# Optional fields with defaults
threads = 8
proxy_addr = "proxy.example.com"
proxy_port = 50000
backup_freq = 300
integrity_freq = 3600
synchronization_enabled = true
udp_effective_payload = 1024
btc_confirmations = 6

# Network topology (25 entries required)
raida_servers = [
    "raida0.example.com:25000",
    "raida1.example.com:25000",
    # ... 23 more entries
]
```

### Validation Requirements
- **Required Fields:** Server startup fails if mandatory fields missing
- **Format Validation:** All fields validated for correct type and format
- **Range Validation:** Numeric fields validated for acceptable ranges
- **Key Validation:** Security keys validated for correct hexadecimal format

## Global Configuration Access

### Global Variable Declaration
- **Single Instance:** One global configuration structure per server process
- **Read-Only Runtime:** Configuration treated as read-only after initialization
- **Thread-Safe Access:** Safe for concurrent reading from multiple threads

### Initialization Lifecycle
- **Startup Phase:** Configuration loaded and validated during server startup
- **Runtime Phase:** Configuration accessed but never modified
- **Shutdown Phase:** Configuration memory cleaned up during shutdown

## Cross-Module Integration

### Configuration Dependencies
- **All Modules:** Configuration provides settings for entire system
- **Network Components:** Server addresses and communication parameters
- **Security Systems:** Authentication keys and security settings
- **Database Layer:** Working directory and persistence timing
- **Integrity System:** Master enable/disable switch and timing

### External Dependencies
- **TOML Parser:** External library for configuration file parsing
- **Network System:** Socket address structures and network types
- **File System:** Path handling and directory operations

## Deployment and Migration Support

### Version Compatibility
- **Backward Compatibility:** New configuration fields have safe defaults
- **Forward Compatibility:** System tolerates unknown configuration fields
- **Migration Support:** Supports gradual configuration updates

### Deployment Control
- **Feature Flags:** Enable controlled rollout of new features
- **Configuration Validation:** Prevents deployment with invalid configuration
- **Safety Checks:** Multiple validation layers prevent misconfiguration

## Security Considerations

### Configuration File Security
- **Sensitive Data:** Configuration file contains sensitive authentication keys
- **File Permissions:** Should be protected with appropriate file system permissions
- **Access Control:** Only authorized personnel should have access to configuration
- **Backup Security:** Configuration backups must protect sensitive information

### Runtime Security
- **Memory Protection:** Configuration keys stored securely in memory
- **No Key Logging:** Security keys never logged or displayed
- **Secure Validation:** Key validation performed securely without exposure

### Network Security
- **Address Validation:** All network addresses validated before use
- **Connection Security:** Pre-resolved addresses prevent DNS-based attacks
- **Topology Validation:** Complete network topology required for security

## Error Handling and Validation

### Configuration Errors
- **Missing Fields:** Clear error messages for missing mandatory fields
- **Format Errors:** Detailed validation errors for incorrect formats
- **Network Errors:** DNS resolution errors clearly reported
- **Key Errors:** Security key validation errors with guidance

### Validation Layers
- **Syntax Validation:** TOML syntax validation with error reporting
- **Semantic Validation:** Field value validation with range checking
- **Network Validation:** DNS resolution and connectivity validation
- **Security Validation:** Key format and security parameter validation

## Performance and Resource Management

### Startup Performance
- **Single Parse:** Configuration parsed once during startup
- **Pre-Resolution:** Network addresses resolved during startup for runtime efficiency
- **Memory Efficiency:** Configuration stored in optimized format

### Runtime Efficiency
- **No Re-parsing:** Configuration never re-parsed during runtime
- **Direct Access:** All configuration values accessible without indirection
- **Cache-Friendly:** Configuration structure organized for efficient access

This configuration header provides the essential structure and constants for centralized, secure configuration management across the entire RAIDA system, enabling controlled deployment, secure operations, and efficient runtime access to all system settings.