# Protocol Definitions Header (protocol.h)

## Module Purpose
This header file defines the complete RAIDA X protocol specification, including dual encryption support (128-bit and 256-bit AES), connection state management, error codes, command groups, ticket structures, and all data types required for secure client-server communication in the RAIDA network.

## Protocol Constants

### Header and Message Sizes
| Constant | Value | Description |
|----------|-------|-------------|
| `REQUEST_HEADER_SIZE` | 48 | Maximum request header size (supports both 32-byte and 48-byte headers) |
| `RESPONSE_HEADER_SIZE` | 48 | Response header size (adaptive based on request protocol version) |
| `NONCE_SIZE` | 24 | Maximum nonce size for 256-bit AES protocol |
| `MAX_BODY_SIZE` | Variable | Maximum allowed request body size for security |

### Network Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `TOTAL_RAIDA_SERVERS` | 25 | Total number of RAIDA servers in the network |
| `RAIDA_SERVER_RCV_TIMEOUT` | 32 | Timeout for inter-RAIDA server communication (seconds) |

## Encryption Type Definitions

### Protocol Encryption Types
| Constant | Value | Description |
|----------|-------|-------------|
| `ENCRYPTION_TYPE_NONE` | 0 | No encryption (reserved for future use) |
| `ENCRYPTION_TYPE_AES` | 1 | Legacy 128-bit AES with single coin authentication |
| `ENCRYPTION_TYPE_LOCKER` | 2 | Legacy 128-bit AES with locker-based authentication |
| `ENCRYPTION_TYPE_AES_256_SINGLE_KEY` | 4 | Modern 256-bit AES with single coin key derivation |
| `ENCRYPTION_TYPE_AES_256_DOUBLE_KEY` | 5 | Modern 256-bit AES with double coin key combination |

### Protocol Version Characteristics
| Version | Header Size | Nonce Size | Key Size | Body Challenge | Response Nonce |
|---------|-------------|------------|----------|----------------|----------------|
| Legacy (0-3) | 32 bytes | 12 bytes | 128-bit | 16-byte challenge | Client nonce reused |
| Modern (4-5) | 48 bytes | 24 bytes | 256-bit | No challenge | Server-generated |

## Connection State Management

### Connection States
| State | Description |
|-------|-------------|
| `STATE_WANT_READ_HEADER` | Connection waiting to read complete request header |
| `STATE_WANT_READ_BODY` | Connection waiting to read request body data |
| `STATE_PROCESSING` | Request being processed by worker thread |
| `STATE_WANT_WRITE` | Connection ready to write response data |
| `STATE_DONE` | Connection finished, ready for cleanup |

### State Transition Flow
```
STATE_WANT_READ_HEADER → STATE_WANT_READ_BODY → STATE_PROCESSING → STATE_WANT_WRITE → STATE_DONE
```

## Data Structure Definitions

### Basic Coin Structure
| Field | Type | Description |
|-------|------|-------------|
| `denomination` | 8-bit Integer | Coin denomination identifier |
| `sn` | 32-bit Integer | Unique coin serial number |

### Connection Information Structure
| Field | Type | Description |
|-------|------|-------------|
| `sa` | Socket Address Pointer | Client address (UDP only, NULL for TCP) |
| `sk` | Integer | Socket file descriptor for connection |
| `e0` | 8-bit Integer | Echo byte 0 from legacy protocol header |
| `e1` | 8-bit Integer | Echo byte 1 from legacy protocol header |
| `body_size` | Integer | Size of request body in bytes |
| `body` | Byte Pointer | Dynamically allocated request body buffer |
| `encryption_denomination` | 8-bit Integer | Coin denomination for encryption key |
| `encryption_sn` | 32-bit Integer | Coin serial number for encryption key |
| `encryption_type` | Integer | Protocol encryption type identifier |
| `encryption_an` | Byte Array[16] | Authentication number for encryption (128-bit) |
| `request_nonce` | Byte Array[24] | Client-provided nonce (size varies by protocol) |
| `response_nonce` | Byte Array[24] | Server-generated response nonce |
| `output_size` | Integer | Size of response data |
| `output` | Byte Pointer | Response data buffer |
| `command_status` | 8-bit Integer | Command execution result status |
| `challenge_hash` | Byte Array[16] | Legacy challenge-response hash |
| `start_time` | Timestamp | Request start time for performance measurement |
| `cgroup` | 8-bit Integer | Command group identifier |
| `command` | 8-bit Integer | Specific command identifier |
| `shard_id` | 8-bit Integer | Shard identifier for request |
| `ip` | String[16] | Client IP address for logging |
| `exec_time` | Integer | Command execution time in microseconds |
| `coin_id` | 8-bit Integer | Coin type identifier |
| `state` | Connection State | Current connection state for non-blocking I/O |
| `read_buf` | Byte Array | Buffer for reading request headers |
| `bytes_to_read` | Integer | Expected bytes for current read operation |
| `bytes_read` | Integer | Bytes actually read so far |
| `write_buf` | Byte Pointer | Complete response buffer for writing |
| `bytes_to_write` | Integer | Total bytes to write in response |
| `bytes_written` | Integer | Bytes already written to socket |

## Error Code Definitions

### Protocol-Level Errors
| Constant | Value | Description |
|----------|--------|-------------|
| `NO_ERROR` | 0 | Successful operation |
| `ERROR_INVALID_CLOUD_ID` | 1 | Invalid cloud identifier |
| `ERROR_INVALID_SPLIT_ID` | 2 | Invalid split identifier |
| `ERROR_INVALID_RAIDA_ID` | 3 | Wrong RAIDA server identifier |
| `ERROR_INVALID_SHARD_ID` | 4 | Invalid shard identifier |
| `ERROR_INVALID_COMMAND_GROUP` | 5 | Unknown command group |
| `ERROR_INVALID_COMMAND` | 6 | Unknown command in group |
| `ERROR_INVALID_COIN_ID` | 7 | Wrong coin type identifier |
| `ERROR_INVALID_UDP_FRAME_COUNT` | 15 | Invalid UDP frame count |
| `ERROR_INVALID_PACKET_LENGTH` | 16 | Incorrect packet size |
| `ERROR_UDP_FRAME_TIMEOUT` | 17 | UDP frame reception timeout |
| `ERROR_WRONG_RAIDA` | 18 | Request sent to wrong RAIDA server |
| `ERROR_SHARD_NOT_AVAILABLE` | 20 | Requested shard not available |
| `ERROR_ENCRYPTION_COIN_NOT_FOUND` | 25 | Encryption key coin not found |
| `ERROR_INVALID_ENCRYPTION_CODE` | 27 | Invalid encryption code |
| `ERROR_INVALID_EOF` | 33 | Invalid end-of-frame trailer |
| `ERROR_INVALID_ENCRYPTION` | 34 | Invalid encryption type |
| `ERROR_EMPTY_REQUEST` | 36 | Empty request received |
| `ERROR_INVALID_CRC` | 37 | Challenge CRC validation failed |
| `ERROR_ADMIN_AUTH` | 38 | Administrative authentication failed |

### Coin and Database Errors
| Constant | Value | Description |
|----------|--------|-------------|
| `ERROR_COINS_NOT_DIV` | 39 | Coin data size not properly divisible |
| `ERROR_INVALID_SN_OR_DENOMINATION` | 40 | Invalid serial number or denomination |
| `ERROR_PAGE_IS_NOT_RESERVED` | 41 | Required page not reserved by session |
| `ERROR_TOO_MANY_COINS` | 45 | Too many coins in single operation |
| `ERROR_INVALID_SHARD` | 46 | Invalid shard identifier |
| `ERROR_DELETE_COINS` | 47 | Coin deletion operation failed |
| `ERROR_LEGACY_DB` | 48 | Legacy database operation error |

### Ticket Management Errors
| Constant | Value | Description |
|----------|--------|-------------|
| `ERROR_NO_TICKET_SLOT` | 42 | No available ticket slots |
| `ERROR_NO_TICKET_FOUND` | 43 | Specified ticket not found |
| `ERROR_TICKET_CLAIMED_ALREADY` | 44 | Ticket already claimed by RAIDA |

### System and Resource Errors
| Constant | Value | Description |
|----------|--------|-------------|
| `ERROR_NOT_IMPLEMENTED` | 89 | Operation not yet implemented |
| `ERROR_MEMORY_ALLOC` | 254 | Memory allocation failure |
| `ERROR_NETWORK` | 253 | Network communication error |
| `ERROR_INTERNAL` | 252 | Internal server error |
| `ERROR_INVALID_ROUTING` | 255 | Invalid routing information |

## Status Code Definitions

### Success Status Codes
| Constant | Value | Description |
|----------|--------|-------------|
| `STATUS_SUCCESS` | 250 | General success status |
| `STATUS_ALL_PASS` | 241 | All items in operation passed |
| `STATUS_ALL_FAIL` | 242 | All items in operation failed |
| `STATUS_MIXED` | 243 | Some items passed, others failed |

### Find Command Status Codes
| Constant | Value | Description |
|----------|--------|-------------|
| `STATUS_FIND_NEITHER` | 208 | No coins found with either authentication number |
| `STATUS_FIND_ALL_AN` | 209 | All coins found with current authentication numbers |
| `STATUS_FIND_ALL_PAN` | 210 | All coins found with proposed authentication numbers |
| `STATUS_FIND_MIXED` | 211 | Mixed results for coin authentication status |

## Command Group Definitions

### Primary Command Groups
| Constant | Value | Description |
|----------|--------|-------------|
| `NO_COMMAND_GROUP` | 0 | Status and information commands |
| `AUTH` | 1 | Authentication and ownership commands |
| `HEALING` | 2 | Network healing and recovery commands |
| `ADMIN` | 3 | Administrative and executive commands |
| `KEY_EXCHANGE` | 4 | Key exchange and management commands |
| `BANKING` | 5 | Banking and financial operations |
| `CHAT` | 6 | Communication and messaging |
| `BLOCKCHAIN` | 7 | Blockchain integration commands |
| `LOCKER` | 8 | Locker storage and retrieval commands |
| `CHANGE` | 9 | Change-making and denomination conversion |
| `SHARD` | 10 | Shard switching and migration commands |
| `CROSSOVER` | 11 | Cross-platform integration commands |
| `RPC` | 12 | Remote procedure call commands |
| `FILESYSTEM` | 13 | File system and object storage commands |
| `INTEGRITY` | 14 | Merkle tree integrity commands |

### Command Group Limits
| Constant | Value | Description |
|----------|--------|-------------|
| `MAX_COMMAND_GROUP` | 14 | Maximum valid command group identifier |
| `MAX_COMMAND` | 255 | Maximum valid command identifier within group |

## Shard Definitions

### Shard Types
| Constant | Value | Description |
|----------|--------|-------------|
| `SHARD_UNKNOWN` | 0 | Unknown or unspecified shard |
| `SHARD_CLOUDCOIN` | 1 | Legacy CloudCoin v1 shard |
| `SHARD_SUPERCOIN` | 2 | CloudCoin v2 (SuperCoin) shard |
| `SHARD_NEW` | 3 | Current RAIDA system shard |

### Shard Limits
| Constant | Value | Description |
|----------|--------|-------------|
| `MAX_SHARD` | 3 | Maximum valid shard identifier |

## Ticket System Definitions

### Ticket Configuration
| Constant | Value | Description |
|----------|--------|-------------|
| `TICKET_POOL_SIZE` | 512 | Maximum number of concurrent tickets |
| `MAX_COINS_PER_TICKET` | 4096 | Maximum coins that can be included in single ticket |
| `TICKET_RELEASE_SECONDS` | 300 | Automatic ticket expiration time in seconds |

### Ticket Entry Structure
| Field | Type | Description |
|-------|------|-------------|
| `created_at` | Timestamp | Ticket creation time for expiration tracking |
| `ticket` | 32-bit Integer | Unique ticket identifier |
| `coins` | Coin Array | Array of coins included in ticket |
| `claims` | Byte Array[25] | Bitmap of RAIDA servers that have claimed ticket |
| `num_coins` | 32-bit Integer | Number of coins stored in ticket |
| `mtx` | Mutex | Thread safety lock for ticket operations |

## Function Type Definitions

### Command Handler Type
```
typedef void(command_handler_t)(conn_info_t *ci);
```

### Function Declarations
| Function | Purpose |
|----------|---------|
| `validate_header` | Validates and parses request headers |
| `validate_decrypt_body` | Validates and decrypts request bodies |
| `send_command_error` | Sends error response to client |
| `get_response_header` | Generates response headers |
| `run_command` | Main command execution entry point |
| `prepare_response` | Prepares complete response for transmission |
| `finish_command` | Completes command processing and cleanup |
| `get_body_payload` | Returns pointer to command payload data |
| `check_tickets` | Background ticket expiration management |
| `init_ticket_storage` | Initializes ticket management system |
| `get_free_ticket_slot` | Allocates new ticket from pool |
| `get_ticket_entry` | Retrieves existing ticket by identifier |
| `unlock_ticket_entry` | Releases ticket lock after use |
| `write_stat` | Records performance and operation statistics |

## Protocol Version Compatibility

### Legacy Protocol Support (Types 0-3)
- **Full Backward Compatibility:** Complete support for existing clients
- **32-byte Headers:** Fixed header size for legacy protocols
- **Challenge-Response:** Includes replay protection mechanism
- **128-bit Security:** AES-128 encryption with coin-based keys

### Modern Protocol Support (Types 4-5)
- **Enhanced Security:** 256-bit AES encryption with hardware acceleration
- **48-byte Headers:** Extended headers for additional security data
- **Server Nonces:** Server-controlled nonces for enhanced security
- **Key Derivation:** Secure key derivation for enhanced encryption

### Protocol Detection
- **Automatic Detection:** Protocol version determined from encryption type field
- **Transparent Operation:** Command processing independent of protocol version
- **Seamless Migration:** Clients can upgrade without server changes

## Security Features

### Authentication Mechanisms
- **Coin-Based Keys:** Encryption keys derived from coin ownership
- **Challenge-Response:** Legacy protocols include replay protection
- **Nonce Management:** Modern protocols use server-controlled nonces
- **Administrative Keys:** Separate authentication for administrative operations

### Encryption Security
- **Dual Algorithm Support:** Both AES-128 and AES-256 supported
- **Hardware Acceleration:** Intel AES-NI support for performance
- **Key Derivation:** Secure hash-based key derivation for 256-bit keys
- **Protocol Isolation:** Different encryption types properly isolated

### Network Security
- **Timeout Management:** Configurable timeouts prevent resource holding
- **Size Limits:** All message sizes bounded for security
- **Validation Depth:** Comprehensive validation at all protocol layers

## Threading and Concurrency

### Thread Safety Requirements
- **Connection Isolation:** Each connection processed independently
- **Ticket Synchronization:** Thread-safe ticket management with fine-grained locking
- **State Machine Safety:** Connection state transitions are atomic
- **Resource Protection:** All shared resources properly synchronized

### Concurrency Design
- **Stateless Processing:** Protocol operations designed to be stateless
- **Independent Operations:** Different connections processed independently
- **Lock-Free Design:** Minimal locking required for protocol operations

This protocol header provides the complete specification for secure, efficient, and backward-compatible communication in the RAIDA network, supporting both legacy and modern clients while enabling enhanced security features and maintaining optimal performance across all supported protocol versions.