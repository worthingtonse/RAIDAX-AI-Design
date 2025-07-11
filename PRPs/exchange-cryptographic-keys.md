# Key Exchange Commands Implementation (cmd_key_exchange)

## Module Purpose
This module implements cryptographic key exchange operations for secure communication between RAIDA servers and clients. It handles session key generation, encryption/decryption of keys for inter-server communication, and key storage/retrieval for chat systems. The module operates within an on-demand page cache database system for efficient coin data access.

## Core Functionality

### 1. Session Key Encryption (`cmd_encrypt_key`)
**Parameters:**
- Connection information structure containing request data
- Input: 23-byte payload (1-byte denomination + 4-byte serial number + 18 bytes additional data)

**Returns:** None (modifies connection structure)

**Purpose:** Generates a new 16-byte session key and encrypts it using the recipient's authentication number (AN). The encryption is performed by XOR operation with the recipient's AN.

**Process:**
1. Validates input payload size (must be exactly 23 bytes)
2. Extracts denomination and serial number from payload
3. Retrieves recipient coin data using on-demand page cache
4. Generates cryptographically secure random session key
5. Encrypts session key by XORing with recipient's AN
6. Returns encrypted key as response

**Dependencies:** 
- Database layer for page retrieval and locking
- Cryptographic utilities for secure random generation
- Logging system for debug output

### 2. RAIDA Key Decryption (`cmd_decrypt_raida_key`)
**Parameters:**
- Connection information structure containing encrypted key data
- Input: Variable-length payload (minimum 49 bytes) containing multiple encrypted coin records

**Returns:** None (modifies connection structure with success/failure indicators)

**Purpose:** Decrypts messages from other RAIDA servers containing coin authentication data. Each coin record contains 26 bytes of encrypted information.

**Process:**
1. Validates minimum payload size and coin record alignment
2. Loads encryption coin data for the requesting entity
3. For each coin record:
   - Validates split ID and denomination authority
   - Decrypts 16-byte key using CTR mode encryption
   - Verifies decrypted data integrity
   - Updates coin data in database if valid
4. Returns array of success/failure indicators for each coin

**Dependencies:**
- Database layer for coin data access and modification
- Encryption utilities for CTR mode decryption
- Utility functions for data extraction

### 3. Key Storage for Chat System (`cmd_post_key`)
**Parameters:**
- Connection information structure
- Input: 185-byte payload containing key data and metadata

**Returns:** None (modifies connection structure)

**Purpose:** Stores cryptographic keys for the chat system in the filesystem using a structured naming convention.

**Process:**
1. Validates payload size (must be exactly 185 bytes)
2. Extracts key size and key length parameters
3. Validates combined size does not exceed limits
4. Creates file path using hexadecimal representation of first 16 bytes
5. Stores denomination, serial number, and key data to file
6. Sets appropriate file permissions

**file format:**
- Byte 0: Denomination
- Bytes 1–4: Serial number (4 bytes)
- Bytes 5+: Key segment data (key_length bytes)
   
**Dependencies:**
- Filesystem operations for file creation and writing
- Configuration system for base directory path
- Logging system for error reporting

### 4. Key Retrieval for Chat System (`cmd_get_key`)
**Parameters:**
- Connection information structure
- Input: 55-byte payload containing key identifier

**Returns:** None (modifies connection structure with retrieved key data)

**Purpose:** Retrieves previously stored keys from the chat system filesystem.

**Process:**
1. Validates payload size (must be exactly 55 bytes)
2. Constructs file path from first 16 bytes of payload
3. Reads complete file contents
4. Returns file data as response payload

**Dependencies:**
- Filesystem operations for file reading
- Configuration system for base directory path
- Memory management for dynamic buffer allocation

### 5. Key Alert Handler (`cmd_key_alert`)
**Parameters:**
- Connection information structure

**Returns:** None (modifies connection structure)

**Purpose:** Handles key alert notifications. Currently implemented as a no-operation placeholder that acknowledges receipt of the alert.

### 6. Encryption Coin Loading (`load_my_enc_coin`)
**Parameters:**
- Denomination identifier (signed 8-bit integer)
- Serial number (32-bit unsigned integer)  
- Buffer pointer for output data (minimum 400 bytes)

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Loads encryption coin data from local filesystem for use in cryptographic operations.

**Process:**
1. Constructs file path using denomination and serial number
2. Opens and reads 440-byte coin file
3. Validates coin ID matches server configuration
4. Extracts 400 bytes of encryption data starting at offset 40
5. Returns extracted data in provided buffer

**Dependencies:**
- Filesystem operations for file access
- Configuration system for coin ID validation
- Error handling for file operation failures

**Used by:** RAIDA key decryption operations, inter-server communication

## Data Structures and Constants

### Input Data Formats
- **Session Key Request:** 1-byte denomination + 4-byte serial number + 18 bytes metadata
- **RAIDA Key Data:** 26-byte records containing split ID, denomination authority, denomination, serial number, and 16-byte encrypted key
- **Chat Key Storage:** Key size parameter, key length parameter, and variable-length key data
- **Chat Key Retrieval:** 16-byte key identifier + 39 bytes additional data

### Output Data Formats
- **Encrypted Session Key:** 16-byte XOR-encrypted key
- **Decryption Results:** Array of status bytes (0x1 for success, 0x0 for failure)
- **Retrieved Chat Key:** Variable-length key data with denomination and serial number prefix

### Error Conditions
- `ERROR_INVALID_PACKET_LENGTH`: Incorrect input payload size
- `ERROR_INVALID_SN_OR_DENOMINATION`: Invalid coin reference
- `ERROR_MEMORY_ALLOC`: Memory allocation failure
- `ERROR_INTERNAL`: Cryptographic operation failure
- `ERROR_FILESYSTEM`: File operation failure
- `ERROR_INVALID_KEY_LENGTH`: Key size validation failure

## Security Considerations

### Cryptographic Operations
- Session keys generated using cryptographically secure random number generation
- Encryption performed using XOR with authentication numbers
- CTR mode encryption for inter-server key exchange
- Key validation through integrity checks

### Access Control
- Coin data access controlled through database layer locking
- File operations use restricted permissions (0640)
- Invalid requests rejected with appropriate error codes

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache for coin data access with locking mechanisms
- **Cryptographic Utilities:** Random number generation, CTR mode encryption/decryption
- **Utilities Module:** Data extraction functions, CRC calculations, endianness conversion
- **Configuration System:** Server identification, file paths, coin validation
- **Logging System:** Debug output, error reporting
- **Network Protocol:** Connection management, response formatting

### External Constants Required
- `RECORDS_PER_PAGE`: Database page organization constant
- `ERROR_*`: Protocol error code definitions
- `STATUS_SUCCESS`: Success status indicator
- `ENCRYPTION_TYPE_*`: Encryption mode identifiers

### File System Dependencies
- Coin storage directory structure: `{base_path}/coins/{denomination}.{serial}.bin`
- Key storage directory structure: `{base_path}/Keys/{hex_identifier}`
- File format specifications for coin and key data

## Threading and Concurrency
- Functions operate within thread pool execution context
- Database page locking ensures thread-safe coin data access
- File operations are atomic at the system level
- No shared state between concurrent operations

## Performance Characteristics
- On-demand page loading minimizes memory usage
- Page caching reduces database I/O for repeated access
- File-based key storage provides persistent key management
- Efficient validation prevents unnecessary processing

This module is essential for secure communication within the RAIDA network, providing the cryptographic foundation for authentication and secure message exchange between servers and clients.