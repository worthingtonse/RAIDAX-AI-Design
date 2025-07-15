# RAIDA Protocol Implementation (protocol.c)

## Module Purpose
This module implements the core RAIDA network protocol, providing request/response processing, command dispatching, connection management, and network communication. It serves as the central communication hub that handles all client interactions, validates requests, executes commands, and manages the complete request lifecycle from initial reception to final response delivery.


### Constants Required
| Name                     | Value     | Description |
|--------------------------|-----------|-------------|
| REQUEST_HEADER_SIZE      | 32 bytes  | Size of the fixed request header |
| RESPONSE_HEADER_SIZE     | 32 bytes  | Size of the fixed response header |
| NONCE_SIZE               | 12 bytes  | Secure random nonce (96 bits) |
| TOTAL_RAIDA_SERVERS      | 25        | Total number of RAIDA nodes |
| TICKET_POOL_SIZE         | 512       | Maximum number of in-memory ticket entries |
| MAX_COINS_PER_TICKET     | 4096      | Maximum coins associated with a single ticket |
| TICKET_RELEASE_SECONDS   | 300       | Expiration time for a ticket in seconds |
| RAIDA_SERVER_RCV_TIMEOUT | 32        | Timeout duration for server response (seconds) |
| MAX_COMMAND_GROUP        | 13        | Highest command group index (Filesystem) |
| MAX_COMMAND              | 255       | Maximum number of commands per group |
| MAX_SHARD                | 3         | Maximum shard ID (e.g., SHARD_NEW) |


### Error Codes

| Code Name                  | Meaning |
|---------------------------|---------|
| ERROR_INVALID_COMMAND      | Command not found in dispatch table |
| ERROR_INVALID_ENCRYPTION   | Encryption type not supported or failed lookup |
| ERROR_MEMORY_ALLOC         | Memory allocation failure during response preparation |
| ERROR_INVALID_PACKET_LENGTH| Request body too short |
| ERROR_INVALID_EOF          | Trailer bytes incorrect |
| ERROR_INVALID_CRC          | Challenge data checksum failed |
| NO_ERROR                   | Indicates successful processing |


## Core Functionality

### 1. Command Execution (`run_command`)
**Parameters:**
- Thread argument containing connection information structure

**Returns:** None

**Purpose:** Executes a validated command in the thread pool context and prepares the response.

**Process:**
1. **Connection Validation:**
   - Validates connection information structure
   - Ensures command handler exists for requested command
   - Checks command group and command number validity

2. **Command Dispatch:**
   - Looks up command handler in dispatch table
   - Logs command execution details
   - Calls appropriate command handler function

3. **Response Preparation:**
   - Calls response preparation function
   - Handles command execution results
   - Prepares final response for client

**Threading Features:**
- Executed in thread pool context
- Thread-safe command execution
- Proper resource management
- Error handling and recovery

### 2. Response Preparation (`prepare_response`)
**Parameters:**
- Connection information structure

**Returns:** None

**Purpose:** Prepares the complete response buffer (header + body) and manages response delivery.

**Process:**
1. **Protocol Type Handling:**
   - **UDP Protocol:** Immediate response sending
   - **TCP Protocol:** Asynchronous response preparation

2. **Response Buffer Construction:**
   - Allocates response buffer for header and body
   - Generates response header with status and metadata
   - Copies response body data if present
   - Adds protocol trailers (0x3e, 0x3e)

3. **Encryption Handling:**
   - Generates secure random nonce for response
   - Applies encryption if required (AES or locker encryption)
   - Uses CTR mode encryption with unique nonces
   - Ensures response security

4. **Network Delivery:**
   - **UDP:** Immediate sendto() with complete response
   - **TCP:** Sets up for asynchronous write completion
   - Arms socket for write operations
   - Manages connection state transitions

**Security Features:**
- Secure random nonce generation for each response
- CTR mode encryption with unique nonces
- Proper encryption key management
- Response integrity protection

### 3. Request Header Validation (`validate_header`)
**Parameters:**
- Header buffer (32 bytes)
- Connection information structure

**Returns:** Integer error code (NO_ERROR on success)

**Purpose:** Validates and processes the request header, extracting all protocol fields and setting up encryption context.

**Process:**
1. **Basic Protocol Validation:**
   - Validates routing version (must be 1)
   - Validates split ID (must be 0)
   - Validates target RAIDA ID matches server ID
   - Validates shard ID within acceptable range

2. **Command Validation:**
   - Validates command group within supported range
   - Validates command number within command group
   - Ensures command handler exists
   - Validates coin ID matches server configuration

3. **Encryption Setup:**
   - Extracts encryption type and parameters
   - **AES Encryption:** Retrieves coin authentication number (Must use an AES-128-CTR)
   - **Locker Encryption:** Retrieves locker authentication number
   - **No Encryption:** Bypasses encryption setup
   - Validates encryption parameters and coin existence

4. **Request Context Setup:**
   - Extracts request nonce for encryption
   - Sets up command execution context
   - Configures response preparation parameters
   - Initializes connection state

**Validation Categories:**
- **Routing Validation:** Protocol version, split ID, RAIDA ID
- **Command Validation:** Command group, command number, coin ID
- **Encryption Validation:** Encryption type, key availability
- **Parameter Validation:** All header fields within acceptable ranges

### 4. Request Body Validation (`validate_decrypt_body`)
**Parameters:**
- Connection information structure

**Returns:** Integer error code (NO_ERROR on success)

**Purpose:** Validates and decrypts the request body, ensuring data integrity and proper format.

**Process:**
1. **Size and Format Validation:**
   - Validates minimum body size (18 bytes)
   - Checks end-of-frame markers (0x3e, 0x3e)
   - Validates body structure integrity

2. **Decryption Processing:**
   - **Encrypted Bodies:** Decrypts using CTR mode with request nonce
   - **Unencrypted Bodies:** Processes directly without decryption
   - Extracts challenge data from decrypted/raw body
   - Handles encryption key application

3. **Challenge Validation:**
   - Extracts 16-byte challenge from body
   - Applies encryption key XOR if encrypted
   - Validates challenge format and content
   - Prepares challenge for response

4. **Integrity Verification:**
   - Calculates CRC32 checksum of challenge data
   - Compares calculated checksum with provided checksum
   - Validates data integrity during transmission
   - Ensures request authenticity

**Security Features:**
- CTR mode decryption with unique nonces
- Challenge/response authentication
- CRC32 integrity verification
- Proper key management

### 5. Response Header Generation (`get_response_header`)
**Parameters:**
- Response buffer (32 bytes)
- Status code (integer)
- Command group (integer)
- Connection information structure

**Returns:** None

**Purpose:** Generates the complete response header with all protocol fields and metadata.

**Process:**
1. **Header Field Population:**
   - Sets server RAIDA ID in response
   - Includes command status and group
   - Adds echo bytes from request
   - Sets protocol version information

2. **Timing Information:**
   - Calculates request execution time
   - Includes timing in response header
   - Provides performance metrics
   - Enables client-side monitoring

3. **Response Size Information:**
   - Calculates total response body size
   - Includes size in header fields
   - Enables proper response parsing
   - Supports variable-length responses

4. **Nonce Integration:**
   - Includes response nonce in header
   - Enables secure response decryption
   - Supports CTR mode encryption
   - Ensures response authenticity

**Header Format:**
- Server identification and routing information
- Command status and execution results
- Response size and format information
- Timing and performance metrics
- Cryptographic nonces for security

### 6. Command Error Handling (`send_command_error`)
**Parameters:**
- Error code (integer)
- Connection information structure

**Returns:** None

**Purpose:** Prepares and sends error responses for failed command processing.

**Process:**
1. **Error State Setup:**
   - Sets command status to error code
   - Clears response body (errors have no body)
   - Frees any allocated response buffers
   - Resets connection state

2. **Error Response Preparation:**
   - Calls standard response preparation
   - Generates error response header
   - Includes error code in response
   - Maintains protocol compliance

**Error Handling:**
- Consistent error response format
- Proper resource cleanup
- Protocol compliance maintenance
- Client error communication

### 7. Connection Management (`finish_command`)
**Parameters:**
- Connection information structure

**Returns:** None

**Purpose:** Completes command processing and manages connection lifecycle.

**Process:**
1. **Protocol Type Handling:**
   - **TCP Connections:** Logs completion and maintains connection
   - **UDP Connections:** Frees connection resources immediately
   - Handles connection state transitions
   - Manages resource cleanup

2. **Resource Management:**
   - Frees allocated buffers and structures
   - Releases network resources
   - Cleans up connection state
   - Prevents resource leaks

**Connection Lifecycle:**
- TCP connections may be reused for multiple requests
- UDP connections are stateless and cleaned up immediately
- Proper resource management for both protocols
- Connection state tracking and cleanup

## Ticket Management System

### 1. Ticket Storage Initialization (`init_ticket_storage`)
**Parameters:**
- None

**Returns:** Integer status code (0 for success, -1 for failure)

**Purpose:** Initializes the ticket memory pool with per-slot locking for healing operations.

**Process:**
1. **Slot Initialization:**
   - Initializes all ticket slots to empty state
   - Creates per-slot mutexes for fine-grained locking
   - Sets up ticket expiration tracking
   - Validates initialization success

2. **Concurrency Setup:**
   - Configures thread-safe ticket access
   - Enables non-blocking ticket operations
   - Sets up concurrent ticket management
   - Initializes locking mechanisms

**Ticket Features:**
- Fixed-size memory pool for predictable performance
- Per-slot locking for high concurrency
- Automatic expiration handling
- Thread-safe operations

### 2. Ticket Expiration Check (`check_tickets`)
**Parameters:**
- None

**Returns:** None

**Purpose:** Checks for and releases expired tickets from the memory pool.

**Process:**
1. **Expiration Scanning:**
   - Scans all ticket slots for expiration
   - Calculates ticket age based on creation time
   - Identifies expired tickets for cleanup
   - Uses configurable expiration timeout

2. **Cleanup Processing:**
   - Releases expired tickets back to pool
   - Clears ticket data and metadata
   - Maintains pool availability
   - Prevents memory leaks

**Cleanup Features:**
- Automatic expired ticket cleanup
- Configurable expiration timeouts
- Thread-safe cleanup operations
- Memory pool management

### 3. Ticket Allocation (`get_free_ticket_slot`)
**Parameters:**
- None

**Returns:** Ticket entry structure pointer or null if no slots available

**Purpose:** Allocates a free ticket slot from the memory pool using non-blocking access.

**Process:**
1. **Slot Search:**
   - Scans ticket pool for available slots
   - Uses non-blocking trylock for concurrency
   - Finds first available slot
   - Avoids blocking on busy slots

2. **Slot Initialization:**
   - Sets ticket creation timestamp
   - Generates unique ticket identifier
   - Initializes coin storage
   - Clears claim tracking

**Allocation Features:**
- Non-blocking slot allocation
- Unique ticket identifier generation
- Automatic slot initialization
- Concurrent access support

### 4. Ticket Retrieval (`get_ticket_entry`)
**Parameters:**
- Ticket identifier (4 bytes)

**Returns:** Ticket entry structure pointer or null if not found

**Purpose:** Retrieves and locks a ticket entry by ticket identifier.

**Process:**
1. **Ticket Search:**
   - Scans ticket pool for matching identifier
   - Validates ticket existence and validity
   - Locks ticket for exclusive access
   - Prevents concurrent modifications

2. **Ticket Validation:**
   - Checks ticket expiration status
   - Validates ticket data integrity
   - Ensures ticket is active
   - Returns locked ticket entry

**Retrieval Features:**
- Efficient ticket lookup by identifier
- Automatic expiration checking
- Exclusive access locking
- Thread-safe operations

### 5. Ticket Unlocking (`unlock_ticket_entry`)
**Parameters:**
- Ticket entry structure pointer

**Returns:** None

**Purpose:** Unlocks a ticket entry after use, allowing other threads to access it.

**Process:**
1. **Lock Release:**
   - Releases ticket-specific mutex
   - Validates ticket pointer
   - Enables concurrent access
   - Maintains lock consistency

## Command Dispatch System

### 1. Command Handler Table
**Structure:** Two-dimensional array indexed by command group and command number
**Purpose:** Maps command identifiers to handler functions
**Organization:**
- **Status Commands (0):** Echo, version, stats, audit
- **Authentication Commands (1):** Detect, pown operations
- **Healing Commands (2):** Ticket operations, find, fix
- **Executive Commands (3):** Admin operations, coin management
- **Key Exchange Commands (4):** Cryptographic key operations
- **Locker Commands (8):** Coin storage and trading
- **Change Commands (9):** Denomination conversion
- **Shard Commands (10):** Cross-shard operations
- **Crossover Commands (11):** External integration
- **RPC Commands (12):** Remote procedure calls
- **Filesystem Commands (13):** Distributed file operations
- **Integrity Commands (14):** Merkle tree operations

### 2. Command Validation
- All command groups and numbers validated before dispatch
- Command handler existence verified
- Invalid commands generate appropriate errors
- Consistent error handling across all command types

### 3. Command Execution
- All commands executed in thread pool context
- Thread-safe command processing
- Proper resource management
- Error handling and recovery

## Network Protocol Features

### 1. Protocol Support
- **TCP Protocol:** Connection-oriented, reliable delivery
- **UDP Protocol:** Connectionless, immediate response
- **Dual Protocol:** Simultaneous TCP and UDP support
- **Protocol Detection:** Automatic protocol type detection

### 2. Encryption Support
- **No Encryption:** Plain text communication
- **AES Encryption:** Coin-based symmetric encryption (Must use an AES-128-CTR)
- **Locker Encryption:** Locker-based symmetric encryption
- **CTR Mode:** Counter mode for secure encryption

### 3. Request/Response Format
- **Fixed Headers:** 32-byte request and response headers
- **Variable Bodies:** Dynamic body sizes based on operation
- **Protocol Trailers:** End-of-frame markers for validation
- **Nonce Support:** Unique nonces for each request/response

## Security and Validation

### 1. Input Validation
- **Header Validation:** All header fields validated
- **Body Validation:** Size, format, and integrity checking
- **Parameter Validation:** All parameters within acceptable ranges
- **Command Validation:** Command existence and authorization

### 2. Encryption Security
- **Secure Nonces:** Cryptographically secure random nonces
- **CTR Mode:** Secure counter mode encryption
- **Key Management:** Proper encryption key handling
- **Unique Nonces:** Different nonce for each message

### 3. Integrity Protection
- **CRC32 Checksums:** Data integrity verification
- **Challenge/Response:** Authentication mechanism
- **Nonce Validation:** Prevents replay attacks
- **Protocol Compliance:** Strict protocol adherence

## Error Handling and Recovery

### 1. Error Categories
- **Protocol Errors:** Invalid headers, format errors
- **Command Errors:** Invalid commands, authorization failures
- **Encryption Errors:** Key errors, decryption failures
- **System Errors:** Memory allocation, resource failures

### 2. Error Responses
- **Consistent Format:** All errors use standard response format
- **Detailed Codes:** Specific error codes for different conditions
- **Resource Cleanup:** Proper cleanup on all error paths
- **Client Communication:** Clear error communication to clients

### 3. Recovery Mechanisms
- **Graceful Degradation:** System continues operation despite errors
- **Resource Recovery:** Automatic resource cleanup and recovery
- **Connection Management:** Proper connection state management
- **Error Logging:** Comprehensive error logging for debugging

## Performance and Scalability

### 1. Threading Model
- **Thread Pool:** Worker threads for command processing
- **Asynchronous I/O:** Non-blocking network operations
- **Concurrent Processing:** Multiple requests processed simultaneously
- **Resource Management:** Efficient thread and memory management

### 2. Memory Management
- **Buffer Allocation:** Dynamic buffer allocation for responses
- **Memory Pools:** Ticket memory pool for predictable performance
- **Resource Cleanup:** Automatic resource cleanup and deallocation
- **Memory Monitoring:** Statistics and monitoring

### 3. Network Performance
- **Connection Pooling:** Efficient connection management
- **Protocol Optimization:** Optimized protocol processing
- **Concurrent Access:** High concurrency support
- **Performance Metrics:** Timing and performance tracking

## Dependencies and Integration

### Required Modules
- **Database System:** For coin data access and validation
- **Network System:** For client communication and connection management
- **Cryptographic System:** For encryption and security operations
- **Threading System:** For parallel processing and concurrency
- **Configuration System:** For server parameters and settings
- **Logging System:** For debugging and monitoring

### External Dependencies
- **Network Libraries:** TCP/UDP socket operations
- **Cryptographic Libraries:** AES encryption, random number generation
- **Threading Libraries:** Thread pool and synchronization
- **System Libraries:** Memory allocation, time functions

### Integration Points
- **Command Handlers:** All command processing modules
- **Database Layer:** Coin data access and management
- **Network Layer:** Client communication and protocol handling
- **Security Layer:** Encryption and authentication


### Connection States

| State Name        | Description |
|-------------------|-------------|
| STATE_WANT_WRITE  | Response is ready and waiting to be sent |
| STATE_READING     | Reading incoming data from the socket |
| STATE_PROCESSING  | Command being executed in a thread |
| STATE_CLOSED      | Connection closed or finished |


This protocol implementation module provides the complete communication infrastructure for the RAIDA network, handling all client interactions while maintaining security, performance, and reliability.