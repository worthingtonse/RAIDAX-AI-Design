# Specification: Configuration Header Interface (config.h)

## 1. Module Purpose
This header file defines the configuration structure and constants for the RAIDAX cryptocurrency system, part of the CloudCoinConsortium project. It provides the interface for system-wide configuration management including network settings, security keys, operational parameters, and inter-RAIDA communication.

## 2. Configuration Structure Interface

### 2.1 Core System Configuration
**RAIDA Identification**: Integer identifier for this RAIDA server instance
**Network Configuration**: Port number for incoming connections
**Performance Settings**: Flush frequency and integrity check intervals
**Communication Thresholds**: UDP payload size limits for protocol optimization

### 2.2 Security Configuration Interface
**Admin Key**: 16-byte administrative key for executive command authentication
**Proxy Key**: 16-byte proxy key for secure proxy communication
**Key Management**: No hardcoded default keys - must be configured externally

### 2.3 File System Configuration
**Working Directory**: String pointer to current working directory path
**Configuration File**: Standard configuration file name definition

### 2.4 Network Topology Configuration
**RAIDA Servers**: Array of server addresses for inter-RAIDA communication
**Server Ports**: Array of port numbers corresponding to RAIDA servers
**Socket Addresses**: Binary converted socket address structures
**Proxy Settings**: Proxy server address and port configuration

### 2.5 Operational Configuration
**Coin Management**: Coin identifier for this RAIDA server instance
**Threading**: Number of worker threads in thread pool
**Blockchain Settings**: Bitcoin confirmation requirements

## 3. Security Model

### 3.1 Key Management Interface
- **No Hardcoded Keys**: All cryptographic keys must be externally configured
- **16-Byte Keys**: Standard key size for admin and proxy authentication
- **Configuration-Based**: Keys loaded from configuration file only

### 3.2 Configuration Safety
- **Thread-Safe Reading**: Structure readable from multiple threads
- **Write-Once**: Configuration written only during program initialization
- **Global Access**: Single global configuration instance

## 4. Integration Requirements

### 4.1 Protocol Dependencies
- **Protocol Constants**: Requires protocol header for RAIDA server count
- **System Limits**: Uses standard system limits for path definitions
- **Type Definitions**: Standard integer and character type support

### 4.2 Configuration File Interface
- **File Name Definition**: Header defines external configuration file name constant
- **Standard Location**: Configuration file located with program binary
- **External Loading**: Configuration data loaded from external file into structure

This specification provides the interface definition for RAIDAX system configuration while emphasizing the security improvements requiring external key configuration and the thread-safe global configuration access pattern.