# RAIDA Protocol Handler (protocol.c)

## Module Purpose
This module implements the complete RAIDA X protocol system, supporting both legacy 128-bit and modern 256-bit AES protocols. It provides request/response handling, command dispatch, dual encryption support, ticket management, challenge-response authentication, and comprehensive protocol validation for secure and efficient client-server communication.

## Constants and Configuration

### Protocol Structure
| Constant | Value | Description |
|----------|-------|-------------|
| `REQUEST_HEADER_SIZE` | 48 | Maximum request header size (supports both 32-byte and 48-byte headers) |
| `RESPONSE_HEADER_SIZE` | 48 | Response header size (adaptive based on request type) |
| `NONCE_SIZE` | 24 | Maximum nonce size for 256-bit protocol |
| `TOTAL_RAIDA_SERVERS` | 25 | Number of RAIDA servers in network |

### Encryption Types
| Constant | Value | Description |
|----------|-------|-------------|
| `ENCRYPTION_TYPE_NONE` | 0 | No encryption (reserved) |
| `ENCRYPTION_TYPE_AES` | 1 | Legacy 128-bit AES with single coin |
| `ENCRYPTION_TYPE_LOCKER` | 2 | Legacy 128-bit AES with locker prefix |
| `ENCRYPTION_TYPE_AES_256_SINGLE_KEY` | 4 | Modern 256-bit AES with single coin key |
| `ENCRYPTION_TYPE_AES_256_DOUBLE_KEY` | 5 | Modern 256-bit AES with double coin key |

### Ticket Management
| Constant | Value | Description |
|----------|-------|-------------|
| `TICKET_POOL_SIZE` | 512 | Maximum number of concurrent tickets |
| `MAX_COINS_PER_TICKET` | 4096 | Maximum coins that can be included in a single ticket |
| `TICKET_RELEASE_SECONDS` | 300 | Automatic ticket expiration time |

### Command Groups
| Constant | Value | Description |
|----------|-------|-------------|
| `AUTH` | 1 | Authentication and ownership commands |
| `HEALING` | 2 | Network healing and recovery commands |
| `ADMIN` | 3 | Administrative and executive commands |
| `KEY_EXCHANGE` | 4 | Key exchange and management commands |
| `LOCKER` | 8 | Locker storage and retrieval commands |
| `CHANGE` | 9 | Change-making and denomination conversion |
| `SHARD` | 10 | Shard switching and migration commands |
| `CROSSOVER` | 11 | Cross-platform integration commands |
| `RPC` | 12 | Remote procedure call commands |
| `FILESYSTEM` | 13 | File system and object storage commands |
| `INTEGRITY` | 14 | Merkle tree integrity commands |

## Data Structures

### Connection Information Structure
| Field | Type | Description |
|-------|------|-------------|
| `sk` | Integer | Socket file descriptor |
| `sa` | Socket Address Pointer | Client address (NULL for TCP, set for UDP) |
| `body_size` | Integer | Size of request body |
| `body` | Byte Pointer | Request body data |
| `encryption_type` | Integer | Protocol encryption type identifier |
| `encryption_denomination` | 8-bit Integer | Coin denomination for encryption key |
| `encryption_sn` | 32-bit Integer | Coin serial number for encryption key |
| `encryption_an` | Byte Array[16] | Authentication number used for encryption |
| `request_nonce` | Byte Array[24] | Client-provided nonce (size varies by protocol) |
| `response_nonce` | Byte Array[24] | Server-generated response nonce |
| `challenge_hash` | Byte Array[16] | Legacy challenge-response hash |
| `output` | Byte Pointer | Response data buffer |
| `output_size` | Integer | Size of response data |
| `command_status` | 8-bit Integer | Command execution status code |
| `cgroup` | 8-bit Integer | Command group identifier |
| `command` | 8-bit Integer | Specific command identifier |
| `coin_id` | 8-bit Integer | Coin type identifier |
| `shard_id` | 8-bit Integer | Shard identifier for request |
| `start_time` | Timestamp | Request start time for performance measurement |
| `exec_time` | Integer | Command execution time in microseconds |
| `ip` | String[16] | Client IP address |

### Ticket Entry Structure
| Field | Type | Description |
|-------|------|-------------|
| `created_at` | Timestamp | Ticket creation time |
| `ticket` | 32-bit Integer | Unique ticket identifier |
| `coins` | Coin Array | Array of coins included in ticket |
| `num_coins` | 32-bit Integer | Number of coins in ticket |
| `claims` | Byte Array[25] | Bitmap of RAIDA servers that have claimed ticket |
| `mtx` | Mutex | Thread safety lock for ticket operations |

### Coin Structure
| Field | Type | Description |
|-------|------|-------------|
| `denomination` | 8-bit Integer | Coin denomination |
| `sn` | 32-bit Integer | Coin serial number |

## Core Functionality

### 1. Run Command (`run_command`)
**Parameters:**
- Thread argument containing connection info pointer

**Returns:** None

**Purpose:** Main command execution entry point that dispatches requests to appropriate command handlers based on command group and command identifier.

**Process:**
1. **Command Lookup:**
   - Extracts command group and command identifier from connection
   - Uses two-dimensional dispatch table for O(1) command lookup
   - Validates command exists in dispatch table

2. **Command Execution:**
   - Calls appropriate command handler function
   - Passes connection info to handler for processing
   - Logs command execution for debugging and statistics

3. **Response Preparation:**
   - Calls response preparation after command completion
   - Handles command execution errors gracefully
   - Ensures proper cleanup on error conditions

**Used By:** Thread pool for request processing

**Dependencies:** Command dispatch table, response preparation

### 2. Validate Header (`validate_header`)
**Parameters:**
- Header buffer (byte array)
- Connection info pointer

**Returns:** Integer status code (NO_ERROR for success)

**Purpose:** Comprehensive validation and parsing of request headers, supporting both legacy 32-byte and modern 48-byte header formats with automatic protocol detection.

**Process:**
1. **Basic Header Validation:**
   - Validates routing byte (must be 1)
   - Validates split ID (must be 0)
   - Validates RAIDA number matches server configuration
   - Validates command group and command identifier
   - Validates coin ID matches server configuration

2. **Protocol Version Detection:**
   - Determines protocol version from encryption type field
   - **Legacy Protocol (encryption_type ≤ 3):** Uses 32-byte header format
   - **Modern Protocol (encryption_type ≥ 4):** Uses 48-byte header format

3. **Legacy Protocol Processing (32-byte header):**
   - Extracts 12-byte nonce from header
   - Parses body size from header
   - **AES Encryption:** Retrieves coin authentication number from database
   - **Locker Encryption:** Retrieves authentication number from locker index

4. **Modern Protocol Processing (48-byte header):**
   - Extracts 24-byte nonce from header
   - Parses body size from header
   - **Single Key (type 4):** Retrieves single coin authentication number
   - **Double Key (type 5):** Retrieves and combines two coin authentication numbers

5. **Double Key Processing:**
   - Retrieves authentication numbers from both specified coins
   - Combines the two 16-byte keys into 32-byte buffer
   - Generates final 256-bit key using SHA-256 hash
   - Stores resulting key for encryption operations

**Security Features:**
- **Comprehensive Validation:** All header fields validated before processing
- **Protocol Version Detection:** Automatic adaptation to client protocol version
- **Key Derivation:** Secure key generation for 256-bit protocols
- **Database Verification:** Coin existence verified before key extraction

**Used By:** Network layer during request processing

**Dependencies:** Database layer for coin retrieval, cryptographic functions

### 3. Validate and Decrypt Body (`validate_decrypt_body`)
**Parameters:**
- Connection info pointer containing encrypted body

**Returns:** Integer status code (NO_ERROR for success)

**Purpose:** Validates request body format and performs decryption based on the protocol version and encryption type determined during header validation.

**Process:**
1. **Basic Validation:**
   - Validates minimum body size (at least 2 bytes for trailer)
   - Validates trailer bytes (must be 0x3e, 0x3e)
   - Ensures body structure integrity

2. **Decryption Based on Protocol Version:**
   - **Legacy AES/Locker (types 1-2):** Uses 128-bit CTR mode with request nonce
   - **Modern Single Key (type 4):** Derives 256-bit key from authentication number, uses hardware AES
   - **Modern Double Key (type 5):** Uses pre-computed 256-bit key from header validation

3. **Legacy Challenge Validation:**
   - For legacy protocols, validates decrypted challenge-response
   - Computes CRC32 of challenge data
   - Verifies CRC matches embedded checksum
   - Extracts challenge hash for response generation

4. **Modern Protocol Processing:**
   - Modern protocols skip challenge validation
   - Payload starts immediately after header
   - No embedded challenge-response mechanism

**Security Features:**
- **Encryption Validation:** Proper decryption required for valid processing
- **Challenge-Response:** Legacy protocols include replay protection
- **Hardware Acceleration:** Uses Intel AES-NI when available for performance
- **Key Security:** Encryption keys derived from authenticated coin ownership

**Used By:** Network layer after header validation

**Dependencies:** Cryptographic library, hardware AES interface

### 4. Prepare Response (`prepare_response`)
**Parameters:**
- Connection info pointer

**Returns:** None

**Purpose:** Constructs complete response including headers and encrypted body, then initiates response transmission through network layer.

**Process:**
1. **Response Size Calculation:**
   - Calculates final output size including trailer
   - Determines header size based on protocol version
   - Allocates response buffer for complete response

2. **Header Generation:**
   - Calls header generation function with execution timing
   - Includes command status and execution statistics
   - Adapts header format to match request protocol version

3. **Body Encryption:**
   - **Legacy Protocols:** Uses 128-bit CTR with client's request nonce
   - **Modern Single Key:** Derives 256-bit key and uses server's response nonce
   - **Modern Double Key:** Uses pre-computed key with server's response nonce
   - Encrypts response data with appropriate algorithm

4. **Response Assembly:**
   - Combines header and encrypted body
   - Adds protocol trailer bytes
   - Prepares for network transmission

5. **Transmission Initiation:**
   - **UDP:** Sends response immediately
   - **TCP:** Signals network layer for asynchronous transmission

**Protocol Adaptation:**
- **Legacy Compatibility:** Full support for existing client implementations
- **Modern Security:** Enhanced security for newer clients
- **Transparent Operation:** Protocol selection transparent to command handlers

**Used By:** Command execution completion

**Dependencies:** Cryptographic operations, network layer

### 5. Get Response Header (`get_response_header`)
**Parameters:**
- Response buffer (byte array)
- Status code (integer)
- Command group (integer)
- Connection info pointer

**Returns:** None

**Purpose:** Generates appropriate response header format based on request protocol version, including timing information and proper authentication data.

**Process:**
1. **Timing Calculation:**
   - Calculates execution time from request start
   - Converts to microseconds for precision
   - Stores in connection info for statistics

2. **Legacy Header Generation (32-byte):**
   - Sets RAIDA number and status fields
   - Includes execution timing and response size
   - Generates challenge response by XORing challenge hash with encryption key
   - Maintains backward compatibility with existing clients

3. **Modern Header Generation (48-byte):**
   - Sets RAIDA number, status, and timing fields
   - Includes response size and encryption type echo
   - Echoes last two bytes of client nonce for verification
   - Generates fresh 24-byte server nonce for response encryption

**Protocol Features:**
- **Timing Information:** Provides execution time for performance monitoring
- **Authentication Echo:** Proper echo of client authentication data
- **Nonce Management:** Secure nonce generation for response encryption

**Used By:** Response preparation process

**Dependencies:** Timing functions, random number generation

### 6. Ticket Management System

#### Initialize Ticket Storage (`init_ticket_storage`)
**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Initializes ticket pool with fine-grained locking for thread-safe ticket management across concurrent healing operations.

**Process:**
1. **Ticket Pool Initialization:**
   - Initializes all ticket entries to empty state
   - Creates individual mutex for each ticket slot
   - Sets up ticket expiration tracking

2. **Thread Safety Setup:**
   - Each ticket has independent mutex for fine-grained locking
   - Prevents contention between different ticket operations
   - Enables concurrent ticket usage

**Used By:** Server initialization

**Dependencies:** Threading system

#### Get Free Ticket Slot (`get_free_ticket_slot`)
**Parameters:** None

**Returns:** Locked ticket entry pointer (NULL if none available)

**Purpose:** Allocates new ticket from pool using non-blocking trylock for optimal performance and deadlock avoidance.

**Process:**
1. **Non-Blocking Search:**
   - Uses pthread_trylock to avoid blocking
   - Searches through ticket pool for available slot
   - Returns immediately if no slots available

2. **Ticket Initialization:**
   - Sets creation timestamp for expiration tracking
   - Generates unique ticket identifier
   - Initializes coin array and claim tracking

3. **Lock Retention:**
   - Returns ticket with lock held for caller
   - Caller responsible for releasing lock after use
   - Prevents race conditions during ticket usage

**Used By:** Healing operations for ticket creation

**Dependencies:** Random number generation, timing functions

#### Get Ticket Entry (`get_ticket_entry`)
**Parameters:**
- Ticket identifier (32-bit integer)

**Returns:** Locked ticket entry pointer (NULL if not found)

**Purpose:** Locates existing ticket by identifier and locks it for exclusive access during claim operations.

**Process:**
1. **Ticket Search:**
   - Searches ticket pool for matching identifier
   - Validates ticket has not expired
   - Ensures ticket is still valid

2. **Lock Acquisition:**
   - Acquires ticket-specific lock for exclusive access
   - Returns ticket with lock held
   - Ensures atomic claim operations

**Used By:** Ticket validation operations

**Dependencies:** Ticket pool management

#### Check Tickets (`check_tickets`)
**Parameters:** None

**Returns:** None

**Purpose:** Background maintenance function that releases expired tickets to prevent resource leaks.

**Process:**
1. **Expiration Check:**
   - Checks each ticket for expiration
   - Compares creation time with current time
   - Uses configured timeout value

2. **Ticket Release:**
   - Marks expired tickets as available
   - Resets ticket state for reuse
   - Logs ticket expiration for debugging

**Used By:** Background maintenance threads

**Dependencies:** Timing functions

### 7. Get Body Payload (`get_body_payload`)
**Parameters:**
- Connection info pointer

**Returns:** Byte pointer to payload start

**Purpose:** Returns pointer to actual command payload, accounting for protocol version differences in body structure.

**Process:**
1. **Protocol Version Detection:**
   - Checks encryption type to determine protocol version
   - **Legacy Protocols:** Skip 16-byte challenge at beginning of body
   - **Modern Protocols:** Payload starts at beginning of body

2. **Pointer Calculation:**
   - Returns appropriate offset into body buffer
   - Ensures command handlers receive clean payload data
   - Abstracts protocol differences from command implementations

**Used By:** All command handlers for payload access

**Dependencies:** Protocol version detection

## Protocol Versions and Compatibility

### Legacy 128-bit Protocol (Types 0-3)
- **Header Size:** 32 bytes
- **Nonce Size:** 12 bytes
- **Body Challenge:** 16-byte challenge-response at body start
- **Encryption:** 128-bit AES CTR mode
- **Key Source:** Single coin authentication number
- **Response Nonce:** Uses client's request nonce

### Modern 256-bit Protocol (Types 4-5)
- **Header Size:** 48 bytes
- **Nonce Size:** 24 bytes
- **Body Challenge:** No challenge (payload starts immediately)
- **Encryption:** 256-bit AES CTR mode with hardware acceleration
- **Key Source:** Single coin (type 4) or double coin with hash combination (type 5)
- **Response Nonce:** Server-generated unique nonce

### Protocol Detection and Adaptation
- **Automatic Detection:** Protocol version determined from encryption type field
- **Transparent Operation:** Command handlers unaware of protocol differences
- **Backward Compatibility:** Legacy clients fully supported
- **Forward Security:** Modern clients benefit from enhanced security

## Error Codes and Status Management

### Protocol-Level Errors
| Constant | Description |
|----------|-------------|
| `NO_ERROR` | 0 - Successful operation |
| `ERROR_INVALID_ROUTING` | 1 - Invalid routing byte in header |
| `ERROR_INVALID_SPLIT_ID` | 2 - Invalid split identifier |
| `ERROR_INVALID_RAIDA_ID` | 3 - Wrong RAIDA server identifier |
| `ERROR_INVALID_SHARD_ID` | 4 - Invalid shard identifier |
| `ERROR_INVALID_COMMAND_GROUP` | 5 - Unknown command group |
| `ERROR_INVALID_COMMAND` | 6 - Unknown command in group |
| `ERROR_INVALID_COIN_ID` | 7 - Wrong coin type identifier |
| `ERROR_INVALID_PACKET_LENGTH` | 16 - Incorrect packet size |
| `ERROR_ENCRYPTION_COIN_NOT_FOUND` | 25 - Encryption key coin not found |
| `ERROR_INVALID_ENCRYPTION` | 34 - Invalid encryption type |
| `ERROR_INVALID_EOF` | 33 - Invalid end-of-frame trailer |
| `ERROR_INVALID_CRC` | 37 - Challenge CRC validation failed |
| `ERROR_MEMORY_ALLOC` | 254 - Memory allocation failure |

### Ticket Management Errors
| Constant | Description |
|----------|-------------|
| `ERROR_NO_TICKET_SLOT` | 42 - No available ticket slots |
| `ERROR_NO_TICKET_FOUND` | 43 - Specified ticket not found |
| `ERROR_TICKET_CLAIMED_ALREADY` | 44 - Ticket already claimed by RAIDA |

### Command Execution Status
| Constant | Description |
|----------|-------------|
| `STATUS_SUCCESS` | 250 - Command executed successfully |
| `STATUS_ALL_PASS` | 241 - All items in operation passed |
| `STATUS_ALL_FAIL` | 242 - All items in operation failed |
| `STATUS_MIXED` | 243 - Some items passed, others failed |

## Command Dispatch System

### Dispatch Table Structure
```
commands[MAX_COMMAND_GROUP + 1][MAX_COMMAND] = {
    [AUTH] = {
        [10] = cmd_detect,
        [11] = cmd_detect_sum,
        [20] = cmd_pown,
        [21] = cmd_pown_sum,
    },
    [HEALING] = {
        [40] = cmd_get_ticket,
        [50] = cmd_validate_ticket,
        [60] = cmd_find,
        [80] = cmd_fix,
    },
    // ... additional command groups
};
```

### Command Handler Interface
- **Function Signature:** `void command_handler(conn_info_t *ci)`
- **Input:** Complete connection info with parsed request
- **Output:** Sets `command_status` and optional `output` buffer
- **Error Handling:** Sets appropriate error status codes

### Command Registration
- **Static Registration:** Commands registered at compile time
- **Two-Level Lookup:** Command group and command identifier
- **Validation:** Unknown commands return ERROR_INVALID_COMMAND
- **Extensibility:** New commands easily added to dispatch table

## Cryptographic Operations

### Key Derivation
- **Legacy Single Key:** Uses 128-bit authentication number directly
- **Modern Single Key:** Derives 256-bit key using SHA-256 hash of authentication number
- **Modern Double Key:** Combines two authentication numbers and hashes result

### Encryption Modes
- **Legacy CTR:** 128-bit AES CTR with client nonce
- **Modern CTR:** 256-bit AES CTR with server nonce
- **Hardware Acceleration:** Intel AES-NI when available
- **Software Fallback:** Error for 256-bit when hardware unavailable

### Nonce Management
- **Legacy Nonce:** 12-byte client nonce reused for response
- **Modern Nonce:** 24-byte server-generated nonce for each response
- **Replay Protection:** Nonce echoing provides replay attack protection
- **Randomness:** Cryptographically secure random number generation

## Performance Characteristics

### Protocol Processing
- **Header Parsing:** O(1) time complexity for all protocol versions
- **Command Dispatch:** O(1) lookup using two-dimensional array
- **Encryption Operations:** Hardware-accelerated when available
- **Memory Management:** Dynamic allocation only for response data

### Ticket Management
- **Fine-Grained Locking:** Individual ticket locks prevent contention
- **Non-Blocking Allocation:** Trylock prevents deadlock during allocation
- **Efficient Search:** Linear search acceptable for reasonable ticket pool size
- **Automatic Cleanup:** Background expiration prevents resource leaks

### Protocol Adaptation
- **Zero Overhead:** Protocol detection adds no processing overhead
- **Transparent Operation:** Command handlers unaware of protocol differences
- **Optimal Encryption:** Best available encryption used for each client

## Security Considerations

### Authentication and Authorization
- **Coin-Based Keys:** Encryption keys derived from coin ownership
- **Challenge-Response:** Legacy protocols include replay protection
- **Nonce Management:** Modern protocols use server-controlled nonces
- **Key Derivation:** Secure hash-based key derivation for enhanced keys

### Protocol Security
- **Version Isolation:** Different protocol versions properly isolated
- **Validation Depth:** Comprehensive validation at all protocol layers
- **Error Handling:** Secure error handling prevents information leakage
- **Resource Protection:** All resource usage bounded and validated

### Ticket Security
- **Expiration Management:** Automatic ticket expiration prevents stale usage
- **Claim Tracking:** Prevents duplicate claims from same RAIDA
- **Thread Safety:** Ticket operations fully thread-safe
- **Random Identifiers:** Cryptographically secure ticket identifier generation

## Dependencies and Integration

### Required Modules
- **Database Layer:** Coin authentication number retrieval
- **Network Layer:** Request/response transmission
- **Cryptographic Library:** AES encryption, SHA-256 hashing
- **Threading System:** Ticket management synchronization
- **Configuration System:** Server identification and validation

### External Hardware Dependencies
- **Intel AES-NI:** Hardware acceleration for 256-bit AES operations
- **Random Number Generator:** Cryptographically secure randomness
- **High-Resolution Timing:** Microsecond timing for performance measurement

### Used By
- **All Command Handlers:** Protocol parsing and response generation
- **Network Layer:** Request processing and response transmission
- **Healing System:** Ticket-based consensus and validation
- **Administrative Tools:** Command execution and response handling

### Cross-File Dependencies
- **Database Module:** Coin data access and authentication number retrieval
- **Network Module:** Connection management and data transmission
- **Command Modules:** All command handler implementations
- **Utilities Module:** Cryptographic functions and random number generation
- **Configuration Module:** Server settings and protocol parameters

## Threading and Concurrency

### Thread Safety Design
- **Connection Isolation:** Each connection processed independently
- **Ticket Synchronization:** Fine-grained locking for ticket operations
- **Stateless Processing:** Protocol operations are stateless
- **Resource Protection:** All shared resources properly synchronized

### Concurrent Operations
- **Multiple Protocols:** Different protocol versions processed concurrently
- **Ticket Independence:** Different tickets can be processed simultaneously
- **Command Parallelism:** Different commands execute in parallel
- **Response Generation:** Response preparation thread-safe

## Administrative Interface

### Protocol Statistics
- **Version Tracking:** Statistics on protocol version usage
- **Performance Metrics:** Timing statistics for protocol operations
- **Error Reporting:** Detailed error statistics and logging
- **Ticket Usage:** Monitoring of ticket pool utilization

### Configuration Control
- **Protocol Support:** Ability to disable specific protocol versions
- **Ticket Pool Size:** Configurable ticket pool parameters
- **Timeout Settings:** Adjustable timeout values
- **Security Settings:** Cryptographic algorithm preferences

## Backward Compatibility and Migration

### Legacy Support
- **Full Compatibility:** Complete support for existing client implementations
- **Gradual Migration:** Clients can upgrade at their own pace
- **Feature Preservation:** All existing functionality preserved
- **Performance Maintenance:** Legacy performance characteristics maintained

### Migration Path
- **Transparent Upgrade:** Servers support both protocols simultaneously
- **Client Detection:** Automatic detection of client capabilities
- **Security Enhancement:** Enhanced security available immediately for capable clients
- **Smooth Transition:** No disruption to existing client operations

## Error Handling and Recovery

### Protocol Error Recovery
- **Graceful Degradation:** Invalid requests handled gracefully
- **Resource Cleanup:** Proper cleanup on all error conditions
- **Client Notification:** Clear error codes returned to clients
- **Logging:** Comprehensive error logging for debugging

### Security Error Handling
- **Invalid Authentication:** Secure handling of authentication failures
- **Encryption Errors:** Proper handling of encryption/decryption failures
- **Key Errors:** Graceful handling of missing or invalid encryption keys
- **Timeout Handling:** Proper cleanup of expired operations

This protocol module provides the comprehensive foundation for secure, efficient, and backward-compatible communication in the RAIDA network, supporting both legacy and modern clients while providing enhanced security features and maintaining optimal performance across all supported protocol versions.