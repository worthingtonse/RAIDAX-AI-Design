# Specification: RAIDAX Key Exchange Commands Implementation (cmd_key_exchange.c)

## 1. Module Purpose
This implementation file provides secure key exchange and cryptographic key management commands for the RAIDAX cryptocurrency system, part of the CloudCoinConsortium project. It implements secure session key generation, coin-based key encryption/decryption, RAIDA server key management, and chat system key storage with enhanced security features including secure random key generation and nonce-based encryption.

## 2. System Architecture Overview

### 2.1 Implementation Components
- **Secure Session Key Generation**: Cryptographically random session key creation using external random functions
- **Coin-Based Key Encryption**: Using coin authentication numbers as encryption keys with external database access
- **RAIDA Key Decryption**: Multi-server encrypted key distribution and decryption using external cryptographic functions
- **Chat Key Management**: Filesystem-based key storage and retrieval for communication systems using standard I/O functions
- **Encryption Coin Loading**: File-based encryption coin data loading with validation

### 2.2 Security Model
- **Cryptographic Randomness**: Calls external secure random number generation functions for session keys
- **Coin-Based Authentication**: Uses external database functions to access coin authentication numbers
- **CTR Mode Encryption**: Uses external cryptographic functions with nonce-based encryption
- **Filesystem Security**: Uses standard file system functions with appropriate permissions
- **Input Validation**: Comprehensive validation of all request parameters and data formats

### 2.3 External Function Dependencies
- **Database Functions**: Calls existing page management functions (get_page_by_sn_lock, unlock_page, add_page_to_dirty_queue)
- **Cryptographic Functions**: Uses external encryption function (crypt_ctr) and secure random generation (generate_random_bytes)
- **Utility Functions**: Calls data conversion functions (get_sn, get_mfs) and time calculation functions
- **File System Functions**: Uses standard system I/O functions (open, read, write, close)
- **Configuration Functions**: Accesses global configuration data through external configuration system
- **Memory Functions**: Uses standard memory allocation and manipulation functions

## 3. System Constants and Configuration

### 3.1 Request Size Constants
```
ENCRYPT_KEY_SIZE = 23 bytes            // 16 challenge + 1 denomination + 4 serial number + 2 EOF
DECRYPT_RAIDA_MIN_SIZE = 49 bytes      // 16 challenge + 1 denomination + 4 serial number + 26 key record + 2 EOF
POST_KEY_SIZE = 185 bytes              // 16 challenge + 16 identifier + 128 data + various fields + 2 EOF
GET_KEY_SIZE = 55 bytes                // 16 challenge + 16 identifier + 21 additional + 2 EOF
```

### 3.2 Data Structure Constants
```
SESSION_KEY_SIZE = 16 bytes            // Standard session key size
RAIDA_KEY_RECORD_SIZE = 26 bytes       // Per-coin record in RAIDA key operations
ENCRYPTION_COIN_FILE_SIZE = 440 bytes  // Standard encryption coin file size
ENCRYPTION_COIN_DATA_SIZE = 400 bytes  // Actual key data size (25 RAIDA × 16 bytes)
ENCRYPTION_COIN_OFFSET = 40 bytes      // Offset to key data in coin file
MAX_KEY_SEGMENT_SIZE = 127 bytes       // Maximum key segment length
CHAT_KEY_BUFFER_SIZE = 512 bytes       // Buffer size for chat key operations
```

### 3.3 File System Constants
```
COIN_FILE_PERMISSIONS = 0640           // Read/write owner, read group
KEY_FILE_PERMISSIONS = 0640            // Read/write owner, read group
COIN_FILE_PATH = "{config.cwd}/coins/{den:02x}.{sn}.bin"
KEY_FILE_PATH = "{config.cwd}/Keys/{16-byte-hex-identifier}"
```

## 4. Core Command Implementations

### 4.1 Encrypt Key Command Implementation
**Purpose**: Generate cryptographically secure session key and encrypt it for recipient using coin authentication number.

**Function Name**: cmd_encrypt_key

**Request Validation**:
1. **Fixed Size Check**: Verify request size equals exactly 23 bytes
2. **Parameter Extraction**: Extract recipient denomination and serial number from payload
3. **Coin Validation**: Verify denomination and serial number are valid

**Processing Implementation**:
1. **Recipient Key Retrieval**:
   - Call external database function get_page_by_sn_lock() to access recipient coin page
   - Copy recipient authentication number (16 bytes) from coin record using memory functions
   - Call external database function unlock_page() to release page lock
2. **Secure Key Generation**:
   - Call external utility function generate_random_bytes() for cryptographically random 16-byte session key
   - Verify successful random generation (return ERROR_INTERNAL on failure)
3. **Key Encryption**:
   - Allocate 16-byte output buffer using standard memory functions
   - XOR session key with recipient authentication number (placeholder for asymmetric encryption)
   - Store encrypted result in output buffer
4. **Response Preparation**:
   - Set output size to 16 bytes
   - Set command status to NO_ERROR

**Security Notes**:
- Uses cryptographically secure random generation instead of predictable keys
- XOR encryption serves as placeholder for proper asymmetric encryption
- Each session key is unique and unpredictable

### 4.2 Decrypt RAIDA Key Command Implementation
**Purpose**: Decrypt keys received from RAIDA servers and update coin authentication data.

**Function Name**: cmd_decrypt_raida_key

**Request Validation**:
1. **Minimum Size Check**: Verify request size at least 49 bytes
2. **Record Count Calculation**: Calculate number of key records as (body_size - 23) / 26
3. **Data Alignment Check**: Verify remaining data length is divisible by 26

**Processing Implementation**:
1. **Decryption Coin Loading**:
   - Extract decryption coin denomination and serial number from payload
   - Call internal function load_my_enc_coin() to load encryption coin data
   - Retrieve 400 bytes of RAIDA key data (25 servers × 16 bytes each)
2. **Key Record Processing Loop**:
   - For each 26-byte key record extract: split_id, RAIDA_address, denomination, serial_number, encrypted_key
   - Validate RAIDA address (must be 0-24)
   - Validate split_id (must be 0 or 1)
   - Select appropriate RAIDA key from loaded data (offset = RAIDA_address × 16)
   - Call external cryptographic function crypt_ctr() with RAIDA key and request_nonce for decryption
   - Validate decrypted key format (byte 15 must be 0xff)
   - Extract and verify decrypted coin denomination and serial number
   - Update coin database with decrypted key data
3. **Database Updates**:
   - Call external database function get_page_by_sn_lock() to access target coin page
   - Copy 8 bytes of decrypted key to appropriate offset (split_id × 8) using memory functions
   - Call external utility function get_mfs() to set current modification sequence
   - Call external database function add_page_to_dirty_queue() for persistence
   - Call external database function unlock_page() to release page lock
4. **Response Generation**:
   - Allocate response buffer (1 byte per key record) using standard memory functions
   - Set success bit (0x1) for successfully processed keys
   - Set failure bit (0x0) for failed key processing
   - Return STATUS_SUCCESS with bitmap response

### 4.3 Load Encryption Coin Implementation
**Purpose**: Internal utility function to load encryption coin data from filesystem.

**Function Name**: load_my_enc_coin

**Parameters**:
- denomination: 8-bit unsigned integer for coin denomination
- serial_number: 32-bit unsigned integer for coin serial number
- buffer: pointer to 400-byte output buffer for key data

**Processing Implementation**:
1. **File Path Construction**: Build path using configuration system and string formatting
2. **File Access**: Use standard file system functions to open file in read-only mode
3. **File Reading**: Use standard I/O functions to read exactly 440 bytes from file
4. **File Validation**:
   - Verify file size equals 440 bytes
   - Extract coin_id from bytes 2-3 using bit operations
   - Verify coin_id matches system configuration using external configuration access
5. **Data Extraction**: Use memory functions to copy 400 bytes from offset 40 to output buffer
6. **Resource Cleanup**: Use standard file system functions to close file descriptor

**Return Values**:
- 0: Success
- -1: File access error, size validation error, or coin_id mismatch

### 4.4 Post Key Command Implementation
**Purpose**: Store cryptographic key segments for chat system in filesystem.

**Function Name**: cmd_post_key

**Request Validation**:
1. **Fixed Size Check**: Verify request size equals exactly 185 bytes
2. **Parameter Extraction**: Extract denomination, serial number, key start position, and key length
3. **Key Segment Validation**: Verify key_start + key_length <= 127

**Processing Implementation**:
1. **File Path Generation**: Use configuration system and string formatting to create hex string path from 16-byte identifier
2. **Key Storage**:
   - Use standard file system functions to create key file with 0640 permissions
   - Use standard I/O functions to write denomination (1 byte)
   - Use standard I/O functions to write serial number (4 bytes)
   - Use standard I/O functions to write key segment data (key_length bytes from calculated offset)
3. **Write Verification**: Ensure all data written successfully using standard I/O error checking

**File Format**:
- Byte 0: Denomination
- Bytes 1-4: Serial number (4 bytes)
- Bytes 5+: Key segment data

### 4.5 Get Key Command Implementation
**Purpose**: Retrieve stored cryptographic keys for chat system from filesystem.

**Function Name**: cmd_get_key

**Request Validation**:
1. **Fixed Size Check**: Verify request size equals exactly 55 bytes
2. **Key Identifier Extraction**: Extract 16-byte identifier from request payload

**Processing Implementation**:
1. **File Path Generation**: Use configuration system and string formatting to create hex string path from identifier
2. **Key Retrieval**:
   - Use standard file system functions to open key file in read-only mode
   - Use standard I/O functions to read up to 512 bytes from file
   - Handle file access errors gracefully using standard error handling
3. **Response Preparation**:
   - Use standard memory functions to allocate output buffer matching file size
   - Use memory functions to copy file contents to output buffer
   - Set output size to actual bytes read
   - Return STATUS_SUCCESS

**Error Handling**:
- File not found: Return ERROR_FILESYSTEM
- Read errors: Return ERROR_FILESYSTEM
- Memory allocation failure: Return ERROR_MEMORY_ALLOC

### 4.6 Key Alert Command Implementation
**Purpose**: Handle key-related alerts (currently placeholder implementation).

**Function Name**: cmd_key_alert

**Processing Implementation**:
- Immediately return STATUS_SUCCESS
- No additional processing or validation
- Serves as placeholder for future key alert functionality

## 5. File System Integration

### 5.1 Encryption Coin Files
- **Location**: "{config.cwd}/coins/{denomination:02x}.{serial_number}.bin"
- **Size**: Exactly 440 bytes
- **Format**:
  - Bytes 0-1: Reserved
  - Bytes 2-3: Coin ID (16-bit value, bytes 2-3)
  - Bytes 4-39: Header/metadata
  - Bytes 40-439: RAIDA key data (25 servers × 16 bytes each)

### 5.2 Chat Key Files
- **Location**: "{config.cwd}/Keys/{16-byte-hex-identifier}"
- **Variable Size**: Up to 512 bytes
- **Format**:
  - Byte 0: Associated coin denomination
  - Bytes 1-4: Associated coin serial number
  - Bytes 5+: Key segment data

### 5.3 File Security
- **Permissions**: 0640 (read/write owner, read group only)
- **Error Handling**: Comprehensive error logging with system error details using external logging functions
- **Path Security**: Fixed directory structure prevents path traversal

## 6. Error Handling and Security Implementation

### 6.1 Request Validation
- **Fixed Size Requirements**: Exact byte count validation for all commands
- **Parameter Bounds**: Validate all numeric parameters within acceptable ranges
- **Data Alignment**: Verify record counts match expected data sizes
- **Format Validation**: Check required format markers and data structure

### 6.2 Cryptographic Security
- **Secure Random Generation**: Call external function generate_random_bytes() for session keys
- **Nonce-Based Encryption**: Use request_nonce with external crypt_ctr() function
- **Key Validation**: Verify decrypted key format and magic bytes
- **Authentication Verification**: Validate coin ownership and data integrity

### 6.3 File System Security
- **Access Control**: Use appropriate file permissions for created files
- **Path Validation**: Use fixed directory structure to prevent traversal
- **Size Validation**: Verify file sizes match expected values
- **Resource Management**: Proper file descriptor cleanup on all code paths

## 7. Integration Requirements

### 7.1 Database Integration
- **Page Management**: Call external functions get_page_by_sn_lock(), unlock_page() for coin access
- **Data Persistence**: Call external function add_page_to_dirty_queue() for modified pages
- **Record Access**: Calculate coin record offsets within pages using internal logic
- **Status Management**: Call external function get_mfs() for modification sequence updates

### 7.2 Cryptographic Integration
- **CTR Mode Encryption**: Call external function crypt_ctr() with nonce support for encryption/decryption
- **Secure Random Generation**: Call external function generate_random_bytes() for session key creation
- **Utility Functions**: Call external function get_sn() for serial number extraction
- **Data Conversion**: Use external utility functions for data format conversion

### 7.3 Configuration Integration
- **Directory Access**: Access external configuration system for config.cwd base directory paths
- **Coin ID Validation**: Access external configuration system for config.coin_id verification
- **File System Setup**: Use configuration data for proper directory structure

### 7.4 Logging Integration
- **Debug Logging**: Call external logging functions for operation tracking
- **Error Logging**: Call external logging functions for error reporting with context
- **Security Logging**: Log security-relevant events using external logging system

## 8. Performance Considerations

### 8.1 File I/O Efficiency
- **Single Operation I/O**: Complete file reads and writes in single system calls
- **Buffer Management**: Use appropriate buffer sizes for different operations
- **File Descriptor Management**: Minimize open file descriptor duration
- **Error Path Efficiency**: Quick cleanup on file operation failures

### 8.2 Memory Management
- **Dynamic Allocation**: Use standard memory functions for response buffer allocation
- **Buffer Sizing**: Allocate exact required sizes for operations
- **Resource Cleanup**: Proper memory cleanup on all code paths
- **Error Handling**: Memory cleanup on allocation failures

### 8.3 Database Access Optimization
- **Page Lock Duration**: Minimize database page lock time
- **Efficient Lookups**: Use optimized coin lookup by denomination and serial number
- **Batch Operations**: Process multiple key records efficiently
- **Resource Management**: Proper page unlocking and cleanup

This specification provides complete implementation guidance for secure key exchange operations while clearly distinguishing between external function calls and internal implementation, accurately reflecting the security features and file-based key management essential for cryptocurrency key exchange operations.