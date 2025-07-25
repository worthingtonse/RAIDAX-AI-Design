# Key Exchange Command Handlers (cmd_key_exchange)

## Module Purpose
This module implements key exchange and cryptographic communication commands for secure inter-RAIDA communication and client key management. It provides encryption key generation, RAIDA-to-RAIDA decryption, chat system key management, and secure key distribution with full integration to the on-demand page cache system and free pages bitmap.

## Core Functionality

### 1. Encrypt Key Command (`cmd_encrypt_key`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Generates a new session key and encrypts it for a recipient using their coin's authentication number as the encryption key.

**Process:**
1. **Request Validation:**
   - Validates exact request size (23 bytes)
   - Extracts recipient coin denomination and serial number
   - Ensures request format compliance

2. **Recipient Coin Loading:**
   - Uses get_page_by_sn_lock to load recipient's coin page
   - Extracts 16-byte authentication number from coin record
   - Unlocks page immediately after AN extraction
   - Validates coin exists and is accessible

3. **Session Key Generation:**
   - Generates cryptographically secure 16-byte random session key
   - Uses generate_random_bytes for strong randomness
   - Ensures key unpredictability and cryptographic strength

4. **Key Encryption Process:**
   - **Simplified Encryption:** XORs session key with recipient's authentication number
   - **Security Model:** Recipient can decrypt using their known AN
   - **Efficiency:** Fast XOR operation suitable for real-time use
   - Returns encrypted key in 16-byte response

**Security Features:**
- **Recipient-Specific:** Each encrypted key is specific to recipient coin
- **Strong Randomness:** Cryptographically secure random key generation
- **Forward Security:** New session key for each exchange
- **Simple Protocol:** Efficient XOR-based encryption

**Used By:** Secure communication setup, session establishment, key distribution

**Dependencies:** Database layer, cryptographic random generation

### 2. Decrypt RAIDA Key Command (`cmd_decrypt_raida_key`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Decrypts messages from other RAIDA servers using encryption coins and reconstructs split authentication numbers.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (49 bytes)
   - Calculates coin count: (body_size - 23) / 26
   - Extracts base denomination and serial number

2. **Encryption Coin Loading:**
   - Loads local encryption coin using load_my_enc_coin
   - **File-Based Storage:** Reads from coins/[den].[sn].bin file
   - **Validation:** Verifies coin ID matches configuration
   - **AES Key Material:** Loads 400 bytes of AES key material (25 × 16 bytes)

3. **Message Decryption Process:**
   - For each encrypted message:
     - Extracts split ID, denomination authority, target coin info, and encrypted data
     - **Parameter Validation:** Verifies split ID (0 or 1) and authority (0-24)
     - **Decryption:** Uses crypt_ctr with appropriate AES key and nonce
     - **Integrity Check:** Verifies decrypted data ends with 0xff marker

4. **Authentication Number Reconstruction:**
   - **Split Recovery:** Combines split parts to reconstruct full authentication numbers
   - **Target Validation:** Verifies decrypted denomination and serial number match
   - **Coin Update:** Updates target coin with reconstructed authentication number fragment
   - **Metadata Update:** Sets MFS to current timestamp and marks page dirty

5. **Bitmap Integration:**
   - **NEW:** Updates free pages bitmap for each successfully reconstructed coin
   - **Consistency:** Maintains synchronization between coin data and bitmap
   - **Real-Time Updates:** Immediate bitmap updates for instant availability

**Cryptographic Features:**
- **Split Secret Sharing:** Supports authentication number splitting across multiple parts
- **AES Encryption:** Uses AES counter mode for secure communication
- **Nonce-Based Security:** Request nonce provides replay protection
- **Integrity Verification:** Built-in integrity checking for decrypted data

**Used By:** Inter-RAIDA secure communication, distributed key recovery, consensus operations

**Dependencies:** Database layer, AES encryption, file system access

### 3. Load My Encryption Coin (`load_my_enc_coin`)
**Parameters:**
- Denomination (8-bit integer)
- Serial number (32-bit integer)
- Output buffer (400 bytes)

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Loads encryption coin from local file system for cryptographic operations.

**Process:**
1. **File Path Construction:**
   - Builds path: [cwd]/coins/[den].[sn].bin
   - Uses denomination and serial number for unique identification
   - Follows standard file naming convention

2. **File Access:**
   - Opens coin file in read-only mode
   - Validates file exists and is accessible
   - Handles file access errors gracefully

3. **Data Loading and Validation:**
   - Reads exactly 440 bytes from coin file
   - **Header Validation:** Extracts and validates coin ID from bytes 2-3
   - **Configuration Check:** Ensures coin ID matches system configuration
   - **Key Material Extraction:** Copies 400 bytes of AES key material (offset 40)

4. **Error Handling:**
   - **File Errors:** Handles missing files and access permission issues
   - **Size Validation:** Ensures file contains expected amount of data
   - **Configuration Mismatch:** Detects and reports coin ID mismatches

**File Format:**
- **Total Size:** 440 bytes
- **Header:** Bytes 0-39 (metadata and coin ID)
- **Key Material:** Bytes 40-439 (25 × 16-byte AES keys)
- **Coin ID Location:** Bytes 2-3 (big-endian 16-bit value)

**Used By:** Decrypt RAIDA key operations, secure communication setup

**Dependencies:** File system access, configuration system

### 4. Post Key Command (`cmd_post_key`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status)

**Purpose:** Stores cryptographic keys for the chat system with secure file-based storage.

**Process:**
1. **Request Validation:**
   - Validates exact request size (185 bytes)
   - Extracts key storage parameters and key length information
   - Validates key length constraints

2. **Key Length Validation:**
   - Extracts key start position (ks) and key length (kl)
   - **Safety Check:** Ensures ks + kl ≤ 127 to prevent buffer overflow
   - **Parameter Validation:** Validates all length parameters are reasonable

3. **File Path Generation:**
   - **Unique Naming:** Uses first 16 bytes of request as hex filename
   - **Directory Structure:** Stores in [cwd]/Keys/ directory
   - **Collision Resistance:** 16-byte hex names provide collision resistance

4. **Key Storage Process:**
   - Creates file with appropriate permissions (0640)
   - **Metadata Storage:** Writes denomination (1 byte) and serial number (4 bytes)
   - **Key Data:** Writes actual key data (kl bytes from offset 37+ks)
   - **Atomic Write:** Ensures complete key storage or failure

**Security Features:**
- **Access Control:** Files created with restricted permissions
- **Length Validation:** Prevents buffer overflow attacks
- **Unique Storage:** Collision-resistant naming scheme
- **Metadata Association:** Keys associated with specific coins

**Used By:** Chat system, secure communication, key distribution

**Dependencies:** File system access, directory management

### 5. Get Key Command (`cmd_get_key`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Retrieves cryptographic keys from the chat system storage with complete key data.

**Process:**
1. **Request Validation:**
   - Validates exact request size (55 bytes)
   - Extracts key identifier from first 16 bytes
   - Ensures request format compliance

2. **File Path Resolution:**
   - **Key Lookup:** Uses 16-byte identifier to construct filename
   - **Directory Access:** Accesses [cwd]/Keys/ directory
   - **File Location:** Locates specific key file

3. **Key Retrieval:**
   - Opens key file in read-only mode
   - **Complete Read:** Reads entire file contents (up to 512 bytes)
   - **Dynamic Sizing:** Handles variable-length key files
   - **Error Handling:** Manages missing files and read errors

4. **Response Construction:**
   - **Memory Allocation:** Allocates exact response buffer size
   - **Data Copy:** Copies complete key file contents to response
   - **Size Setting:** Sets exact response size based on file size

**Performance Features:**
- **Efficient Lookup:** Direct file access based on identifier
- **Dynamic Response:** Response size matches actual key data
- **Memory Efficiency:** Allocates only necessary memory
- **Error Recovery:** Graceful handling of missing keys

**Used By:** Chat system, key retrieval, secure communication

**Dependencies:** File system access, memory management

### 6. Key Alert Command (`cmd_key_alert`)
**Parameters:**
- Connection info structure containing request body  

**Returns:** None (sets connection status)

**Purpose:** Handles key alert notifications (currently a no-operation placeholder for future functionality).

**Process:**
1. **Acknowledgment:**
   - Logs receipt of key alert
   - Returns success status
   - **Future Extension Point:** Placeholder for alert handling logic

2. **Design Pattern:**
   - **Extensibility:** Ready for future alert processing requirements
   - **Protocol Compliance:** Maintains protocol compatibility
   - **Status Reporting:** Provides proper status response

**Used By:** Key management system, alert processing, protocol compliance

**Dependencies:** Logging system

## Chat System Integration

### Key Storage Architecture
- **File-Based Storage:** Persistent key storage in file system
- **Unique Naming:** Collision-resistant 16-byte hex identifiers
- **Secure Permissions:** Restricted file permissions for security
- **Metadata Association:** Keys linked to specific coins and metadata

### Key Lifecycle Management
- **Key Creation:** Secure storage with validation and error handling
- **Key Retrieval:** Efficient lookup and complete data retrieval
- **Key Validation:** Comprehensive validation during storage and retrieval
- **Key Security:** Access control and secure file permissions

## Cryptographic Security

### Encryption Methods
- **XOR Encryption:** Simple XOR-based encryption for session keys
- **AES Counter Mode:** Secure AES-CTR encryption for inter-RAIDA communication
- **Random Key Generation:** Cryptographically secure random key generation
- **Nonce-Based Security:** Request nonces prevent replay attacks

### Key Management Security
- **Encryption Coins:** Dedicated coins for cryptographic operations
- **Split Secret Sharing:** Authentication numbers split across multiple parts
- **File-Based Keys:** Secure file storage with access control
- **Configuration Validation:** Coin ID validation ensures system integrity

### Security Boundaries
- **Recipient-Specific:** Keys encrypted for specific recipients
- **Session-Based:** Unique keys for each communication session
- **Access Control:** File permissions restrict key access
- **Integrity Checking:** Built-in integrity verification

## Performance Optimizations

### Database Integration
- **On-Demand Loading:** Coins loaded only when needed for cryptographic operations
- **Efficient Caching:** Benefits from database page cache for frequently used coins
- **Immediate Unlocking:** Pages unlocked quickly after data extraction
- **Minimal I/O:** Optimized for minimal disk access

### Memory Management
- **Dynamic Allocation:** Memory allocated based on actual key sizes
- **Efficient Copying:** Optimized data copying and buffer management
- **Resource Cleanup:** Proper cleanup prevents memory leaks
- **Buffer Validation:** Comprehensive buffer overflow prevention

### File System Optimization
- **Direct Access:** Direct file access for encryption coins
- **Efficient Naming:** Hash-based naming for fast lookup
- **Minimal I/O:** Reads only necessary data from files
- **Error Caching:** Avoids repeated access to missing files

## Error Handling and Recovery

### Cryptographic Error Handling
- **Key Generation Failures:** Graceful handling of random generation failures
- **Decryption Failures:** Proper handling of corrupted or invalid encrypted data
- **Validation Failures:** Comprehensive validation with clear error reporting
- **Integrity Failures:** Detection and handling of integrity check failures

### File System Error Handling
- **Missing Files:** Graceful handling of missing encryption coins and keys
- **Access Errors:** Proper handling of permission and access errors
- **I/O Errors:** Recovery from disk I/O failures
- **Storage Errors:** Handling of storage failures during key saving

### System Recovery
- **State Consistency:** Operations maintain consistent system state
- **Error Reporting:** Clear error codes and messages for debugging
- **Resource Cleanup:** Proper cleanup on all error paths
- **Graceful Degradation:** System continues operating despite key system failures

## Dependencies and Integration

### Required Modules
- **Database Layer:** Coin data access for encryption operations
- **Cryptographic Functions:** Random number generation and AES encryption
- **File System:** Encryption coin and key file access
- **Configuration System:** System configuration and coin ID validation
- **Network Layer:** Inter-RAIDA communication support

### Used By
- **Secure Communication:** Primary interface for cryptographic operations
- **Chat System:** Key management for secure messaging
- **Inter-RAIDA Operations:** Secure communication between RAIDA servers
- **Authentication Systems:** Key-based authentication mechanisms

### Cross-File Dependencies
- **Database Module:** Page access and coin data retrieval
- **Utilities Module:** Cryptographic functions and random generation
- **Configuration Module:** System settings and validation parameters
- **Network Module:** Communication protocols and data handling
- **File System:** Key storage and encryption coin access

This key exchange command module provides comprehensive cryptographic communication capabilities with secure key management, inter-RAIDA communication protocols, chat system integration, and robust error handling, enabling secure distributed operations across the RAIDA network.