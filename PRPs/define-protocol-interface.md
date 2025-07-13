# Protocol Header Definitions (protocol.h)

## Module Purpose
This header file defines the complete RAIDA X protocol interface, data structures, constants, and function declarations for secure client-server communication. It establishes the connection state management system, protocol constants, error codes, command categories, encryption types, and enhanced security features including unique nonce management for each message exchange.

## Protocol Constants and Structure

## request header

| Byte Offset | Field                    | Size | Description                                                        |
| ----------- | ------------------------ | ---- | ------------------------------------------------------------------ |
| 0           | Router version           | 1    | Must be 1                                                          |
| 1           | Split ID                 | 1    | For future use (always 0 for now)                                  |
| 2           | RAIDA ID                 | 1    | Must match the RAIDA server receiving the request                  |
| 3           | Shard ID                 | 1    | Used to select blockchain shard                                    |
| 4           | Command group            | 1    | e.g., AUTH, ADMIN, LOCKER, etc.                                    |
| 5           | Command index            | 1    | Specific command in that group                                     |
| 6–7         | Coin ID                  | 2    | 16-bit big-endian identifier (coin type)                           |
| 8–15        | Reserved                 | 8    | Currently unused, may be filled with 0                             |
| 16          | Encryption type          | 1    | 0 = None, 1 = AES, 2 = Locker-based                                |
| 17          | Encryption denomination  | 1    | If encryption is used                                              |
| 18–21       | Encryption serial number | 4    | 32-bit serial for encryption key selection                         |
| 22–23       | Body size                | 2    | How many bytes follow after the header (big-endian)                |
| 20–31       | Request nonce            | 12   | Used in CTR encryption mode for security                           |
| 30–31       | Echo bytes               | 2    | Echoed back by the server in the response (like a transaction tag) |

## response header 

| Byte Offset | Field               | Size | Description                                                         |
| ----------- | ------------------- | ---- | ------------------------------------------------------------------- |
| 0           | RAIDA number        | 1    | Which RAIDA server sent the response                                |
| 1           | Shard ID            | 1    | Matches request                                                     |
| 2           | Response status     | 1    | e.g., `STATUS_SUCCESS`, `ERROR_INVALID_COMMAND`                     |
| 3           | Command group       | 1    | Same as request                                                     |
| 4–5         | UDP frame count     | 2    | If UDP used, how many packets make up the full response             |
| 6–7         | Echo bytes          | 2    | Mirrors the request’s echo bytes                                    |
| 8           | Reserved            | 1    | Always 0                                                            |
| 9–11        | Output size         | 3    | 24-bit big-endian size of response body                             |
| 12–15       | Execution time      | 4    | In microseconds, for performance/stats                              |
| 16–27       | Response nonce      | 12   | Used to decrypt encrypted response body (matches CTR mode security) |
| 28–31       | Reserved or padding | 4    | May be set to 0, reserved for future protocol extensions            |



### Fixed Protocol Sizes
- **REQUEST_HEADER_SIZE:** 32 bytes - Fixed size for all request headers
- **RESPONSE_HEADER_SIZE:** 32 bytes - Fixed size for all response headers
- **NONCE_SIZE:** 12 bytes - Standard cryptographically secure nonce size (96 bits for CTR mode)

### Network Topology
- **TOTAL_RAIDA_SERVERS:** 25 - Number of RAIDA servers in the network

## Core Data Structures

### Basic Coin Structure (`coin_t`)
**Purpose:** Fundamental representation of a coin in the RAIDA system

#### Fields
- **denomination:** 8-bit signed integer identifying coin type (-8 to +6 range)
- **sn:** 32-bit unsigned integer serial number (unique within denomination)

**Usage:** Universal coin identification across all system operations

### Connection State Management (`connection_state_t`)
**Purpose:** Enumeration defining connection processing phases for non-blocking I/O

#### State Values
- **STATE_WANT_READ_HEADER:** Waiting to receive complete request header (32 bytes)
- **STATE_WANT_READ_BODY:** Waiting to receive complete request body (variable size)
- **STATE_PROCESSING:** Request being processed by worker thread
- **STATE_WANT_WRITE:** Response ready for transmission to client
- **STATE_DONE:** Connection completed, ready for cleanup

### Connection Information Structure (`conn_info_t`)
**Purpose:** Complete state management for client connections with enhanced security features

#### Network and Connection Fields
- **sa:** Socket address pointer (for UDP connections, NULL for TCP)
- **sk:** Socket file descriptor (integer)
- **ip:** 16-character array containing client IP address (including null terminator)
- **start_time:** Timestamp structure recording connection start time

#### Protocol and Command Fields
- **e0, e1:** 8-bit echo bytes for client request correlation
- **cgroup:** 8-bit command group identifier
- **command:** 8-bit command number within group
- **shard_id:** 8-bit shard identifier for cross-blockchain operations
- **coin_id:** 8-bit coin type identifier

#### Request Data Management
- **body_size:** Integer size of request body in bytes
- **body:** Pointer to dynamically allocated request body buffer

#### ENHANCED: Encryption and Security Fields
- **encryption_type:** Integer encryption mode (NONE, AES, or LOCKER)
- **encryption_denomination:** 8-bit signed integer for encryption coin denomination
- **encryption_sn:** 32-bit unsigned integer for encryption coin serial number
- **encryption_an:** 16-byte array containing authentication number for encryption

#### CRITICAL: Enhanced Nonce Management
- **request_nonce:** 12-byte array containing nonce from client request header
- **response_nonce:** 12-byte array containing server-generated unique response nonce

**Security Enhancement:** Separate nonces for request and response eliminate nonce reuse vulnerabilities

#### Response and Processing Fields
- **output_size:** Integer size of response body
- **output:** Pointer to dynamically allocated response data
- **command_status:** 8-bit status code for command execution result
- **challenge_hash:** 16-byte array for cryptographic challenge verification
- **exec_time:** Unsigned long integer execution time in microseconds

#### Non-Blocking I/O State Management
- **state:** Connection state enumeration value
- **read_buf:** 32-byte fixed array for reading request headers
- **bytes_to_read:** Integer total bytes expected for current read operation
- **bytes_read:** Integer bytes already read for current operation
- **write_buf:** Pointer to dynamically allocated response buffer
- **bytes_to_write:** Integer total bytes to transmit
- **bytes_written:** Integer bytes already transmitted

## Protocol Encryption Types

### Encryption Mode Constants
- **ENCRYPTION_TYPE_NONE:** 0 - No encryption, plaintext communication
- **ENCRYPTION_TYPE_AES:** 1 - AES encryption using specific coin's authentication number
- **ENCRYPTION_TYPE_LOCKER:** 2 - Locker-based encryption using locker authentication number

## Protocol status and  Error Codes

## Success Status Codes

| Code | Name                   | Description                            |
| ---- | ---------------------- | -------------------------------------- |
| 208  | STATUS\_FIND\_NEITHER  | Find command: neither PAN nor AN found |
| 209  | STATUS\_FIND\_ALL\_AN  | Find command: all ANs found            |
| 210  | STATUS\_FIND\_ALL\_PAN | Find command: all PANs found           |
| 211  | STATUS\_FIND\_MIXED    | Find command: mixed PAN/AN result      |
| 241  | STATUS\_ALL\_PASS      | All items in batch operation succeeded |
| 242  | STATUS\_ALL\_FAIL      | All items in batch operation failed    |
| 243  | STATUS\_MIXED          | Partial success in batch operation     |
| 250  | STATUS\_SUCCESS        | General successful completion          |



## Communication and Protocol Errors
| Code | Name                           | Description                         |
| ---- | ------------------------------ | ----------------------------------- |
| 0    | NO\_ERROR                      | Successful operation completion     |
| 1    | ERROR\_INVALID\_CLOUD\_ID      | Invalid cloud identifier in request |
| 2    | ERROR\_INVALID\_SPLIT\_ID      | Invalid split identifier            |
| 3    | ERROR\_INVALID\_RAIDA\_ID      | Request sent to wrong RAIDA server  |
| 4    | ERROR\_INVALID\_SHARD\_ID      | Invalid shard identifier            |
| 5    | ERROR\_INVALID\_COMMAND\_GROUP | Command group out of valid range    |
| 6    | ERROR\_INVALID\_COMMAND        | Command not implemented or invalid  |
| 7    | ERROR\_INVALID\_COIN\_ID       | Coin type identifier mismatch       |

##  Packet and Network Errors
| Code | Name                              | Description                        |
| ---- | --------------------------------- | ---------------------------------- |
| 15   | ERROR\_INVALID\_UDP\_FRAME\_COUNT | UDP frame sequencing error         |
| 16   | ERROR\_INVALID\_PACKET\_LENGTH    | Packet size validation failure     |
| 17   | ERROR\_UDP\_FRAME\_TIMEOUT        | UDP frame assembly timeout         |
| 18   | ERROR\_WRONG\_RAIDA               | Request routed to incorrect server |

##  Encryption and Security Errors
| Code | Name                                | Description                               |
| ---- | ----------------------------------- | ----------------------------------------- |
| 20   | ERROR\_SHARD\_NOT\_AVAILABLE        | Requested shard not accessible            |
| 25   | ERROR\_ENCRYPTION\_COIN\_NOT\_FOUND | Encryption coin doesn't exist             |
| 27   | ERROR\_INVALID\_ENCRYPTION\_CODE    | Encryption parameters invalid             |
| 33   | ERROR\_INVALID\_EOF                 | Protocol trailer validation failure       |
| 34   | ERROR\_INVALID\_ENCRYPTION          | Encryption type or parameters invalid     |
| 36   | ERROR\_EMPTY\_REQUEST               | Request body missing when required        |
| 37   | ERROR\_INVALID\_CRC                 | Cryptographic checksum validation failure |
| 38   | ERROR\_ADMIN\_AUTH                  | Administrative authentication failure     |

##  Operational Errors
| Code | Name                                 | Description                          |
| ---- | ------------------------------------ | ------------------------------------ |
| 39   | ERROR\_COINS\_NOT\_DIV               | Coin data not properly aligned       |
| 40   | ERROR\_INVALID\_SN\_OR\_DENOMINATION | Invalid coin reference               |
| 41   | ERROR\_PAGE\_IS\_NOT\_RESERVED       | Database page reservation conflict   |
| 42   | ERROR\_NO\_TICKET\_SLOT              | No available ticket for operation    |
| 43   | ERROR\_NO\_TICKET\_FOUND             | Ticket does not exist                |
| 44   | ERROR\_TICKET\_CLAIMED\_ALREADY      | Ticket already used                  |
| 45   | ERROR\_TOO\_MANY\_COINS              | Request exceeds coin quantity limits |
| 46   | ERROR\_INVALID\_SHARD                | Shard operation parameter error      |
| 47   | ERROR\_DELETE\_COINS                 | Coin deletion operation failure      |
| 48   | ERROR\_LEGACY\_DB                    | Legacy database format not supported |
| 49   | ERROR\_CROSSOVER\_FULL               | Crossover table full                 |
| 50   | ERROR\_INVALID\_TRADE\_COIN          | Trade coin not valid                 |
| 51   | ERROR\_TRADE\_LOCKER\_NOT\_FOUND     | Trade locker not found               |
| 52   | ERROR\_NO\_PRIVATE\_KEY              | Missing private key for operation    |

##  Locker / Advanced Errors
| Code | Name                          | Description                            |
| ---- | ----------------------------- | -------------------------------------- |
| 148  | ERROR\_TRADE\_LOCKER\_EXISTS  | Trade locker already exists            |
| 149  | ERROR\_NO\_TRADE\_LOCKER      | Trade locker does not exist            |
| 150  | STATUS\_WAITING               | Waiting for more input or confirmation |
| 152  | ERROR\_NO\_BTC\_IN\_WALLET    | No Bitcoin in wallet                   |
| 153  | ERROR\_FEW\_COINS\_IN\_LOCKER | Not enough coins in locker             |
| 154  | ERROR\_LOCKER\_USED           | Locker already used                    |
| 160  | ERROR\_REQUEST\_RATE          | Too many requests from client          |


## Transaction and Proxy Errors
| Code | Name                                  | Description                         |
| ---- | ------------------------------------- | ----------------------------------- |
| 177  | ERROR\_TXN\_PROCESSED                 | Transaction already processed       |
| 178  | ERROR\_CRYPTO\_CONNECT                | Failed to connect to crypto backend |
| 179  | ERROR\_LOCKER\_EMPTY\_OR\_NOT\_EXISTS | Locker missing or empty             |
| 180  | ERROR\_PROXY\_CONNECT                 | Failed to connect to proxy          |
| 181  | ERROR\_PRICE                          | Invalid or mismatched price         |
| 182  | ERROR\_NO\_COINS                      | No coins available in wallet        |

##  DNS and Domain Errors
| Code | Name             | Description               |
| ---- | ---------------- | ------------------------- |
| 183  | STATUS\_TX\_SEEN | Transaction observed      |
| 184  | ERROR\_NXRECORD  | DNS record does not exist |
| 185  | ERROR\_NXDOMAIN  | DNS domain does not exist |
| 186  | ERROR\_UNKNOWN   | Unknown error             |
| 187  | ERROR\_PROXY     | Proxy failure             |

## Key and External Backend Errors
| Code | Name                     | Description            |
| ---- | ------------------------ | ---------------------- |
| 188  | ERROR\_KEY\_BUILD        | Failed to build key    |
| 189  | ERROR\_EXTERNAL\_BACKEND | External backend error |

## Transaction Validation Errors
| Code | Name                    | Description                 |
| ---- | ----------------------- | --------------------------- |
| 190  | ERROR\_TX\_EMPTY        | Transaction is empty        |
| 191  | ERROR\_TX\_NOT\_EXIST   | Transaction not found       |
| 192  | ERROR\_AMOUNT\_MISMATCH | Transaction amount mismatch |
| 193  | ERROR\_NO\_ENTRY        | Entry not found             |

##  File and Filesystem Errors
| Code | Name                        | Description                       |
| ---- | --------------------------- | --------------------------------- |
| 194  | ERROR\_FILESYSTEM           | General filesystem error          |
| 195  | ERROR\_INVALID\_KEY\_START  | Key start bytes are invalid       |
| 196  | ERROR\_INVALID\_KEY\_LENGTH | Key length is invalid             |
| 197  | ERROR\_COIN\_LOAD           | Failed to load coin               |
| 198  | ERROR\_INVALID\_PARAMETER   | Invalid parameter provided        |
| 199  | ERROR\_INVALID\_PAN         | PAN (private auth number) invalid |
| 201  | ERROR\_FILE\_EXISTS         | File already exists               |
| 202  | ERROR\_FILE\_NOT\_EXIST     | File does not exist               |
| 203  | ERROR\_INVALID\_TRANSACTION | Transaction data malformed        |
| 204  | ERROR\_BLOCKCHAIN           | Blockchain logic error            |
| 205  | ERROR\_ASSEMBLE             | Failed to assemble final output   |

## System and Resource Errors

| Code | Name                    | Description                   |
| ---- | ----------------------- | ----------------------------- |
| 252  | ERROR\_INTERNAL         | Internal server error         |
| 253  | ERROR\_NETWORK          | Network communication failure |
| 254  | ERROR\_MEMORY\_ALLOC    | Memory allocation failure     |
| 255  | ERROR\_INVALID\_ROUTING | Request routing error         |


## Command Group Definitions

### Command Category Enumeration
- **NO_COMMAND_GROUP:** 0 - Invalid/unspecified command group
- **AUTH:** 1 - Authentication and proof-of-ownership operations
- **HEALING:** 2 - Network healing and coin recovery operations
- **ADMIN:** 3 - Administrative and executive operations
- **KEY_EXCHANGE:** 4 - Cryptographic key management operations
- **LOCKER:** 5 - Secure coin storage and trading operations
- **CHANGE:** 6 - Denomination conversion operations
- **SHARD:** 7 - Cross-blockchain migration operations
- **CROSSOVER:** 8 - Inter-blockchain value transfer operations
- **RPC:** 9 - Remote procedure call operations
- **FILESYSTEM:** 10 - Object storage and file management operations

### Command Validation Constants
- **MAX_COMMAND_GROUP:** FILESYSTEM (10) - Highest valid command group
- **MAX_COMMAND:** 255 - Maximum command number within any group

## Shard System Definitions

###  Shard Types
- **SHARD_UNKNOWN:** 0 - Unspecified or invalid shard
- **SHARD_CLOUDCOIN:** 1 - Legacy CloudCoin blockchain system
- **SHARD_SUPERCOIN:** 2 - Legacy SuperCoin blockchain system
- **SHARD_NEW:** 3 - Current RAIDA system

### Shard Validation
- **MAX_SHARD:** SHARD_NEW (3) - Maximum valid shard identifier

## Function Interface Declarations

### Protocol Validation Functions

#### `validate_header`
**Parameters:**
- Request buffer (unsigned character array pointer)
- Connection information structure pointer

**Returns:** Integer error code (NO_ERROR for success)
**Purpose:** Validates request header fields and extracts protocol parameters

#### `validate_decrypt_body`
**Parameters:**
- Connection information structure pointer

**Returns:** Integer error code (NO_ERROR for success)
**Purpose:** Validates and decrypts request body using enhanced nonce security

### Response Management Functions

#### `send_command_error`
**Parameters:**
- Error code (integer)
- Connection information structure pointer

**Returns:** None
**Purpose:** Sends standardized error response for protocol or processing failures

#### `get_response_header`
**Parameters:**
- Response buffer (character array pointer)
- Status code (integer)
- Command group (integer)
- Connection information structure pointer

**Returns:** None
**Purpose:** Generates response header with enhanced nonce security

### Request Processing Functions

#### `run_command`
**Parameters:**
- Thread argument pointer (connection information structure)

**Returns:** None
**Purpose:** Main request processing function executed in thread pool context

#### `prepare_response`
**Parameters:**
- Connection information structure pointer

**Returns:** None
**Purpose:** Prepares complete response with enhanced security through unique response nonces

#### `finish_command`
**Parameters:**
- Connection information structure pointer

**Returns:** None
**Purpose:** Finalizes request processing and cleans up resources

### Utility Functions

#### `get_body_payload`
**Parameters:**
- Connection information structure pointer

**Returns:** Pointer to request payload (skipping 16-byte challenge)
**Purpose:** Provides access to actual request data after protocol overhead

#### `write_stat`
**Parameters:**
- Connection information structure pointer

**Returns:** None
**Purpose:** Logs comprehensive request statistics for monitoring and analysis

## Enhanced Ticket Management System

### Ticket Configuration Constants
- **TICKET_POOL_SIZE:** 512 - Maximum concurrent healing tickets
- **MAX_COINS_PER_TICKET:** 4096 - Maximum coins per healing operation
- **TICKET_RELEASE_SECONDS:** 300 - Automatic ticket expiry time

### Ticket Entry Structure (`ticket_entry_t`)
**Purpose:** Manages healing ticket state with enhanced concurrency

#### Core Fields
- **created_at:** Timestamp of ticket creation
- **ticket:** 32-bit unsigned integer ticket identifier
- **coins:** Array of coin structures (up to MAX_COINS_PER_TICKET)
- **claims:** Character array tracking claims from each RAIDA server (25 elements)
- **num_coins:** 32-bit unsigned integer count of coins in ticket

#### ENHANCED: Fine-Grained Locking
- **mtx:** Per-ticket mutex for fine-grained concurrency control

**Concurrency Improvement:** Individual ticket locks eliminate global contention

### Ticket Management Functions

#### `init_ticket_storage`
**Parameters:** None
**Returns:** Integer status code (0 for success, negative for error)
**Purpose:** Initializes ticket memory pool with fine-grained locking

#### `check_tickets`
**Parameters:** None
**Returns:** None
**Purpose:** Removes expired tickets using per-ticket locking

#### `get_free_ticket_slot`
**Parameters:** None
**Returns:** Pointer to ticket entry (NULL if no slots available, returned locked)
**Purpose:** Allocates ticket slot using non-blocking approach

#### `get_ticket_entry`
**Parameters:**
- Ticket identifier (32-bit unsigned integer)

**Returns:** Pointer to ticket entry (NULL if not found, returned locked)
**Purpose:** Retrieves ticket by identifier with automatic locking

#### `unlock_ticket_entry`
**Parameters:**
- Ticket entry structure pointer

**Returns:** None
**Purpose:** Releases ticket lock after use

## Command Handler Type Definition

### Function Pointer Type
```
typedef void(command_handler_t)(connection_info_structure_pointer);
```

**Purpose:** Standard signature for all command handler functions enabling uniform command dispatch

## Security Enhancements

### Nonce Management Security
- **Unique Per-Message:** Each request and response uses unique nonce
- **Cryptographically Secure:** All nonces generated using secure random sources
- **CTR Mode Support:** Proper nonce size and handling for counter mode encryption
- **Replay Prevention:** Unique nonces prevent replay attack vulnerabilities

### Protocol Security
- **Input Validation:** Comprehensive validation of all protocol fields
- **Size Limits:** Maximum body size prevents resource exhaustion attacks
- **Error Handling:** Secure error responses prevent information leakage
- **Authentication:** Multiple authentication modes for different security levels

### Connection Security
- **State Management:** Secure state transitions prevent protocol confusion
- **Resource Limits:** Connection limits prevent resource exhaustion
- **Timeout Management:** Automatic cleanup prevents resource hoarding
- **Audit Logging:** Comprehensive logging for security monitoring

## Integration Dependencies

### Required External Types
- **System Types:** Socket address structures, timestamp types, thread synchronization
- **Network Types:** File descriptor types, socket operation constants
- **Cryptographic Types:** Nonce and authentication number sizes
- **Memory Management:** Dynamic allocation and cleanup functions

### Provided Interface
- **Protocol Processing:** Complete request/response handling
- **Security Framework:** Encryption and authentication services
- **Command Dispatch:** Uniform command execution framework
- **Ticket Management:** Healing operation coordination

### Used By
- **Network Layer:** Protocol validation and response formatting
- **Command Modules:** All command implementations use protocol utilities
- **Main Server:** Protocol initialization and ticket management
- **Security Systems:** Encryption and authentication operations

This protocol header provides the complete interface definition for secure, efficient RAIDA network communication with enhanced security through proper nonce management and comprehensive error handling across all operational categories.