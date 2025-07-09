#  Specification: Key Exchange Commands Implementation

## 1. Module Purpose
This module implements secure key exchange and cryptographic key management commands for the CloudCoin RAIDA system. It provides cryptographically secure key generation, coin-based key encryption/decryption, RAIDA server key management, and chat system key storage with enhanced security features including secure random key generation and nonce-based encryption.

## 2. System Architecture Overview

### 2.1 Core Components
- **Secure Key Generation**: Cryptographically random session key creation
- **Coin-Based Encryption**: Using coin ANs as public keys for key exchange
- **RAIDA Key Management**: Multi-server encrypted key distribution and decryption
- **Chat Key Storage**: Filesystem-based key storage for communication systems
- **Enhanced Encryption**: Nonce-based CTR mode encryption for improved security

### 2.2 Security Model
- **Cryptographic Randomness**: Secure random number generation for session keys
- **Coin-Based Authentication**: Coin ANs serve as cryptographic identifiers
- **Multi-Server Distribution**: Keys distributed across multiple RAIDA servers
- **Filesystem Security**: Secure key file storage with appropriate permissions

## 3. System Constants and Configuration

### 3.1 Key Management Constants
```
SESSION_KEY_SIZE = 16 bytes            // Standard session key size
RAIDA_KEY_RECORD_SIZE = 26 bytes       // Per-coin record in RAIDA key operations
ENCRYPTION_COIN_FILE_SIZE = 440 bytes  // Standard encryption coin file size
ENCRYPTION_COIN_DATA_SIZE = 400 bytes  // Actual key data size (25 RAIDA * 16 bytes)
ENCRYPTION_COIN_OFFSET = 40 bytes      // Offset to key data in coin file
MAX_KEY_SEGMENT_SIZE = 127 bytes       // Maximum key segment length
CHAT_KEY_BUFFER_SIZE = 512 bytes       // Buffer size for chat key operations
```

### 3.2 Request Format Constants
```
ENCRYPT_KEY_SIZE = 23 bytes            // 16 challenge + 5 coin + 2 EOF
POST_KEY_SIZE = 185 bytes              // 16 challenge + 165 key data + 4 control + 2 EOF
GET_KEY_SIZE = 55 bytes                // 16 challenge + 37 identifier + 2 EOF
DECRYPT_KEY_MIN_SIZE = 49 bytes        // 16 challenge + 5 coin + 26 key record + 2 EOF
```

### 3.3 File System Constants
```
COIN_FILE_PERMISSIONS = 0640           // Read/write owner, read group
KEY_FILE_PERMISSIONS = 0640            // Read/write owner, read group
COIN_FILE_PATH = "{config.cwd}/coins/{den:02x}.{sn}.bin"
KEY_FILE_PATH = "{config.cwd}/Keys/{16-byte-hex-identifier}"
```

## 4. Core Command Implementations

### 4.1. cmd_encrypt_key
**Purpose**: Generates cryptographically secure session key and encrypts it for recipient.

**Request Format**:
```
[16 bytes Challenge Header]
[1 byte Recipient Denomination]
[4 bytes Recipient Serial Number]
[2 bytes EOF trailer]
Total size: exactly 23 bytes
```

**Enhanced Security Processing**:
1. **Validation**: Verify request size equals 23 bytes exactly
2. **Recipient Key Retrieval**:
   - Extract recipient denomination and serial number
   - Lock database page containing recipient coin
   - Copy recipient's AN (16 bytes) from coin record
   - Unlock database page
3. **Secure Key Generation**:
   - **Generate cryptographically random 16-byte session key** using generate_random_bytes()
   - Verify successful random generation (return ERROR_INTERNAL on failure)
4. **Key Encryption**:
   - Allocate 16-byte output buffer
   - **Security Note**: XOR session key with recipient AN (placeholder for asymmetric encryption)
   - In production systems, use proper asymmetric encryption (RSA, ECC, etc.)
5. **Response Preparation**:
   - Set output size to 16 bytes
   - Set command status to NO_ERROR

**Security Improvements**:
- Replaced predictable key generation with cryptographically secure random generation
- Each key generation produces unique, unpredictable session keys
- Foundation for proper asymmetric encryption implementation

### 4.2. cmd_decrypt_raida_key
**Purpose**: Decrypts keys received from RAIDA servers and updates coin data.

**Request Format**:
```
[16 bytes Challenge Header]
[1 byte Decryption Coin Denomination]
[4 bytes Decryption Coin Serial Number]
[26 bytes per key record: 2 coin + split + RAIDA + shard + DN + 4 SN + 16 key]
[2 bytes EOF trailer]
Minimum size: 49 bytes (16 + 5 + 26 + 2)
```

**Processing Logic**:
1. **Request Validation**:
   - Verify minimum size of 49 bytes
   - Calculate number of key records: (body_size - 23) / 26
   - Verify data length is divisible by 26

2. **Decryption Coin Loading**:
   - Extract decryption coin denomination and serial number
   - Load encryption coin data using load_my_enc_coin()
   - Retrieve 400 bytes of RAIDA key data (25 servers × 16 bytes each)

3. **Key Record Processing**:
   - For each 26-byte key record:
     - Extract split_id, RAIDA_address, denomination, serial_number, encrypted_key
     - Validate RAIDA address (must be 0-24)
     - Validate split_id (must be 0 or 1)
     - Select appropriate RAIDA key from loaded data (offset = RAIDA_address × 16)
     - **Use request_nonce for decryption** (enhanced security)
     - Decrypt 16-byte key using crypt_ctr with RAIDA key and request_nonce
     - Validate decrypted key format (byte 15 must be 0xff)
     - Extract and verify decrypted coin denomination and serial number
     - Update coin database with decrypted key data

4. **Database Updates**:
   - Lock database page for target coin
   - Copy 8 bytes of decrypted key to appropriate offset (split_id × 8)
   - Set MFS byte to current modification sequence
   - Add page to dirty queue for persistence
   - Unlock database page

5. **Response Generation**:
   - Allocate response buffer (1 byte per key record)
   - Set success bit (0x1) for successfully processed keys
   - Set failure bit (0x0) for failed key processing
   - Return STATUS_SUCCESS with bitmap response

**Enhanced Security Features**:
- Uses request_nonce for CTR mode decryption
- Comprehensive validation of all key record components
- Secure handling of multi-server key distribution

### 4.3. load_my_enc_coin(denomination, serial_number, buffer)
**Purpose**: Utility function to load encryption coin data from filesystem.

**Parameters**:
- denomination: 8-bit signed integer for coin denomination
- serial_number: 32-bit unsigned integer for coin serial number
- buffer: pointer to 400-byte output buffer for key data

**Processing Logic**:
1. **File Path Construction**: Build path as "{config.cwd}/coins/{den:02x}.{sn}.bin"
2. **File Access**: Open file in read-only mode
3. **File Reading**: Read exactly 440 bytes from file
4. **Validation**:
   - Verify file size equals 440 bytes
   - Extract coin_id from bytes 2-3
   - Verify coin_id matches system configuration
5. **Data Extraction**: Copy 400 bytes from offset 40 to output buffer
6. **Cleanup**: Close file descriptor

**Return Values**:
- 0: Success
- -1: File access error, size validation error, or coin_id mismatch

### 4.4. cmd_post_key
**Purpose**: Stores cryptographic key segments for chat system.

**Request Format**:
```
[16 bytes Challenge Header]
[16 bytes Key Identifier]
[128 bytes Key Data]
[1 byte Denomination]
[4 bytes Serial Number]
[1 byte Key Start Position]
[1 byte Key Length]
[2 bytes EOF trailer]
Total size: exactly 185 bytes
```

**Processing Logic**:
1. **Request Validation**: Verify request size equals 185 bytes exactly
2. **Parameter Extraction**:
   - Key identifier (16 bytes from offset 0)
   - Denomination and serial number (5 bytes from offset 32)
   - Key start position and length (2 bytes from offset 165-166)
3. **Key Segment Validation**: Verify key_start + key_length <= 127
4. **File Path Generation**: Create hex string path from 16-byte identifier
5. **Key Storage**:
   - Create key file with 0640 permissions
   - Write denomination (1 byte)
   - Write serial number (4 bytes)
   - Write key segment data (key_length bytes from offset 37 + key_start)
6. **Verification**: Ensure all data written successfully

**File Format**:
- Byte 0: Denomination
- Bytes 1-4: Serial number
- Bytes 5+: Key segment data

### 4.5. cmd_get_key
**Purpose**: Retrieves stored cryptographic keys for chat system.

**Request Format**:
```
[16 bytes Challenge Header]
[16 bytes Key Identifier]
[21 bytes Additional Data]
[2 bytes EOF trailer]
Total size: exactly 55 bytes
```

**Processing Logic**:
1. **Request Validation**: Verify request size equals 55 bytes exactly
2. **Key Identifier Extraction**: Get 16-byte identifier from request
3. **File Path Generation**: Create hex string path from identifier
4. **Key Retrieval**:
   - Open key file in read-only mode
   - Read up to 512 bytes from file
   - Handle file access errors gracefully
5. **Response Preparation**:
   - Allocate output buffer matching file size
   - Copy file contents to output buffer
   - Set output size to actual bytes read
   - Return STATUS_SUCCESS

**Error Handling**:
- File not found: Return ERROR_FILESYSTEM
- Read errors: Return ERROR_FILESYSTEM
- Memory allocation failure: Return ERROR_MEMORY_ALLOC

### 4.6. cmd_key_alert
**Purpose**: Handles key-related alerts (currently placeholder implementation).

**Processing Logic**:
- Immediately return STATUS_SUCCESS
- No additional processing or validation
- Serves as placeholder for future key alert functionality

## 5. File System Integration

### 5.1 Encryption Coin Files
- **Location**: "{config.cwd}/coins/{denomination:02x}.{serial_number}.bin"
- **Size**: Exactly 440 bytes
- **Format**:
  - Bytes 0-1: Reserved
  - Bytes 2-3: Coin ID (16-bit big-endian)
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
- **Error Handling**: Comprehensive error logging with system error details
- **Path Security**: Fixed directory structure prevents path traversal

## 6. Cryptographic Security Features

### 6.1 Enhanced Random Generation
- **Secure Key Generation**: Uses generate_random_bytes() for session keys
- **Cryptographic Quality**: Platform-specific secure random sources
- **Error Handling**: Validates successful random generation
- **Key Uniqueness**: Each session key is cryptographically unique

### 6.2 Nonce-Based Encryption
- **CTR Mode Enhancement**: Uses request_nonce for decryption operations
- **Nonce Source**: Client-provided nonce from request header
- **Security Improvement**: Prevents nonce reuse vulnerabilities
- **Compatibility**: Works with enhanced protocol nonce management

### 6.3 Coin-Based Cryptography
- **Public Key Simulation**: Coin ANs serve as public key identifiers
- **Key Distribution**: Multi-server key storage and retrieval
- **Authentication**: Coin ownership proves key access rights
- **Scalability**: Supports 25 RAIDA servers with individual keys

## 7. Error Handling and Validation

### 7.1 Request Validation
- **Fixed Size Requirements**: Exact byte count validation for all commands
- **Parameter Bounds**: Validate all numeric parameters within acceptable ranges
- **Data Alignment**: Verify record counts match expected data sizes
- **Format Validation**: Check required format markers and trailing bytes

### 7.2 Cryptographic Error Handling
- **Random Generation Failures**: Return ERROR_INTERNAL for secure random failures
- **Decryption Validation**: Verify decrypted data meets expected format
- **Key Format Checks**: Validate key structure and magic bytes
- **Authentication Failures**: Handle invalid coin data gracefully

### 7.3 File System Error Handling
- **Access Errors**: Log specific system errors with context
- **Permission Issues**: Handle file creation and access permission problems
- **Size Validation**: Verify file sizes match expected values
- **Resource Cleanup**: Close file descriptors on all error paths

## 8. Integration Requirements

### 8.1 Database Layer Integration
- **Page Management**: get_page_by_sn_lock(), unlock_page() for coin access
- **Data Persistence**: add_page_to_dirty_queue() for modified pages
- **Record Access**: Calculate coin record offsets within pages
- **MFS Management**: Update modification flag sequence appropriately

### 8.2 Cryptographic Integration
- **Enhanced CTR Mode**: crypt_ctr() with nonce support for encryption/decryption
- **Secure Random**: generate_random_bytes() for cryptographically secure key generation
- **Utility Functions**: get_sn() for serial number extraction
- **MFS Generation**: get_mfs() for modification sequence numbers

### 8.3 Configuration Integration
- **Directory Access**: config.cwd for base directory path construction
- **Coin ID Validation**: config.coin_id for encryption coin verification
- **File System Setup**: Proper directory structure for coins and keys

## 9. Performance Considerations

### 9.1 File I/O Optimization
- **Single Read Operations**: Complete file reads in single system calls
- **Buffer Management**: Appropriate buffer sizes for different operations
- **File Descriptor Lifecycle**: Minimize open file descriptor duration
- **Error Path Efficiency**: Quick cleanup on file operation failures

### 9.2 Cryptographic Performance
- **Secure Random Optimization**: Efficient use of platform random sources
- **Batch Processing**: Process multiple key records in single operation
- **Memory Efficiency**: Appropriate buffer allocation for key operations
- **CPU Optimization**: Minimize cryptographic operations per request

### 9.3 Database Access Patterns
- **Page Locking**: Minimize database page lock duration
- **Batch Updates**: Group related database modifications
- **Index Usage**: Efficient coin lookup by denomination and serial number
- **Cache Efficiency**: Optimize memory access patterns for coin data

This specification provides complete implementation guidance for secure key exchange operations while remaining language-agnostic and accurately reflecting the enhanced security features and cryptographic improvements.