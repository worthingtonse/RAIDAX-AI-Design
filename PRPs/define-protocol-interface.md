# RAIDA Protocol Header Definitions (protocol.h)

## Module Purpose
This header defines the complete RAIDA network protocol specification, including data structures, constants, enumerations, and function interfaces for all protocol operations. It serves as the central specification for client-server communication, request/response handling, and network protocol compliance across the RAIDA network.

## Core Protocol Constants

### 1. Header Size Definitions
**Constants:**
- `REQUEST_HEADER_SIZE` (32 bytes): Fixed size for all request headers
- `RESPONSE_HEADER_SIZE` (32 bytes): Fixed size for all response headers
- `NONCE_SIZE` (12 bytes): Standard size for cryptographic nonces in CTR mode

**Purpose:** Ensures consistent header formatting and parsing across all network operations
**Usage:** All protocol processing functions use these constants for buffer allocation and parsing

### 2. Network Configuration
**Constants:**
- `TOTAL_RAIDA_SERVERS` (25): Total number of RAIDA servers in the network
- `RAIDA_SERVER_RCV_TIMEOUT` (32 seconds): Timeout for inter-server communication

**Purpose:** Defines network topology and communication parameters
**Usage:** Used for consensus operations, healing, and inter-server communication

## Data Structure Definitions

### 1. Basic Coin Structure (`coin_t`)
**Purpose:** Fundamental coin identification structure used throughout the system
**Fields:**
- `denomination`: Coin denomination identifier (1 byte signed integer)
- `sn`: Serial number (4 bytes unsigned integer)

**Usage:** All coin-related operations and data structures
**Scope:** Used across all modules for consistent coin identification

### 2. Connection State Enumeration (`connection_state_t`)
**Purpose:** Tracks the current state of non-blocking network connections
**Values:**
- `STATE_WANT_READ_HEADER`: Waiting to read request header
- `STATE_WANT_READ_BODY`: Waiting to read request body
- `STATE_PROCESSING`: Request being processed by worker thread
- `STATE_WANT_WRITE`: Ready to write response
- `STATE_DONE`: Connection finished, ready to close

**Usage:** Network connection management and state tracking
**Threading:** Enables proper coordination between network and worker threads

### 3. Connection Information Structure (`conn_info_t`)
**Purpose:** Complete context for client connection and request processing
**Fields:**
- **Network Information:**
  - `sa`: Client socket address pointer (UDP only)
  - `sk`: Client socket descriptor
  - `ip`: Client IP address string (16 bytes)
  - `state`: Current connection state
  - `bytes_to_read/read`: Non-blocking read state
  - `bytes_to_write/written`: Non-blocking write state

- **Request Data:**
  - `body_size`: Size of request body
  - `body`: Request body data pointer
  - `e0`, `e1`: Echo bytes for response
  - `cgroup`: Command group identifier
  - `command`: Command number within group
  - `shard_id`: Target shard identifier
  - `coin_id`: Coin ID for validation

- **Encryption Context:**
  - `encryption_type`: Type of encryption used
  - `encryption_denomination`: Coin denomination for encryption
  - `encryption_sn`: Coin serial number for encryption
  - `encryption_an`: Authentication number for encryption (16 bytes)
  - `request_nonce`: Nonce from client request (12 bytes)
  - `response_nonce`: Nonce for server response (12 bytes)

- **Response Data:**
  - `output_size`: Size of response body
  - `output`: Response body data pointer
  - `command_status`: Command execution status
  - `challenge_hash`: Challenge data for authentication (16 bytes)

- **Timing and Statistics:**
  - `start_time`: Request start timestamp
  - `exec_time`: Command execution time
  - `read_buf`: Buffer for reading headers (32 bytes)
  - `write_buf`: Buffer for writing responses

**Usage:** Passed to all command handlers and protocol functions
**Lifecycle:** Created for each request, maintained throughout processing

## Protocol Enumerations

### 1. Encryption Types
**Purpose:** Defines supported encryption methods for secure communication
**Values:**
- `ENCRYPTION_TYPE_NONE` (0): No encryption, plain text communication
- `ENCRYPTION_TYPE_AES` (1): AES encryption using coin authentication number
- `ENCRYPTION_TYPE_LOCKER` (2): AES encryption using locker authentication number

**Usage:** Request header encryption type field
**Security:** Determines encryption/decryption methods for request/response bodies

### 2. Status Code Enumeration
**Purpose:** Comprehensive error and success codes for all protocol operations
**Categories:**

**Success Codes:**
- `NO_ERROR` (0): Operation completed successfully
- `STATUS_SUCCESS` (250): General success indicator
- `STATUS_ALL_PASS` (241): All items in batch succeeded
- `STATUS_ALL_FAIL` (242): All items in batch failed
- `STATUS_MIXED` (243): Mixed success/failure in batch

**Protocol Errors:**
- `ERROR_INVALID_CLOUD_ID` (1): Invalid cloud identifier
- `ERROR_INVALID_SPLIT_ID` (2): Invalid split identifier
- `ERROR_INVALID_RAIDA_ID` (3): Invalid RAIDA server identifier
- `ERROR_INVALID_SHARD_ID` (4): Invalid shard identifier
- `ERROR_INVALID_COMMAND_GROUP` (5): Invalid command group
- `ERROR_INVALID_COMMAND` (6): Invalid command number
- `ERROR_INVALID_COIN_ID` (7): Invalid coin identifier
- `ERROR_INVALID_PACKET_LENGTH` (16): Incorrect packet size
- `ERROR_INVALID_EOF` (33): Invalid end-of-frame marker
- `ERROR_INVALID_CRC` (37): CRC checksum mismatch

**Authentication Errors:**
- `ERROR_ADMIN_AUTH` (38): Invalid admin authentication
- `ERROR_INVALID_ENCRYPTION` (34): Invalid encryption parameters
- `ERROR_INVALID_SN_OR_DENOMINATION` (40): Invalid coin reference

**Operation Errors:**
- `ERROR_MEMORY_ALLOC` (254): Memory allocation failure
- `ERROR_INTERNAL` (252): Internal processing error
- `ERROR_NETWORK` (253): Network communication error
- `ERROR_NOT_IMPLEMENTED` (89): Feature not implemented

**Healing-Specific Codes:**
- `ERROR_NO_TICKET_SLOT` (42): No ticket slots available
- `ERROR_NO_TICKET_FOUND` (43): Ticket not found
- `ERROR_TICKET_CLAIMED_ALREADY` (44): Ticket already claimed
- `STATUS_FIND_ALL_AN` (209): All coins found with current AN
- `STATUS_FIND_ALL_PAN` (210): All coins found with proposed AN
- `STATUS_FIND_NEITHER` (208): No coins found with either AN
- `STATUS_FIND_MIXED` (211): Mixed results for find operation

**Usage:** All command handlers return these codes to indicate operation results
**Client Communication:** Enables detailed error reporting to clients

### 3. Command Group Enumeration
**Purpose:** Organizes commands into logical functional groups
**Values:**
- `NO_COMMAND_GROUP` (0): Default/status operations
- `AUTH` (1): Authentication and ownership operations
- `HEALING` (2): Recovery and healing operations
- `ADMIN` (3): Administrative operations
- `KEY_EXCHANGE` (4): Cryptographic key operations
- `BANKING` (5): Banking and financial operations
- `CHAT` (6): Chat and messaging operations
- `BLOCKCHAIN` (7): Blockchain operations
- `LOCKER` (8): Coin storage and trading operations
- `CHANGE` (9): Denomination conversion operations
- `SHARD` (10): Cross-shard operations
- `CROSSOVER` (11): External system integration
- `RPC` (12): Remote procedure calls
- `FILESYSTEM` (13): Distributed file operations

**Usage:** Request header command group field
**Validation:** Used for command validation and dispatch

### 4. Shard Enumeration
**Purpose:** Identifies different coin system shards
**Values:**
- `SHARD_UNKNOWN` (0): Unknown or invalid shard
- `SHARD_CLOUDCOIN` (1): CloudCoin v1 legacy system
- `SHARD_SUPERCOIN` (2): CloudCoin v2/SuperCoin system
- `SHARD_NEW` (3): Current RAIDA system
- `MAX_SHARD`: Maximum valid shard identifier

**Usage:** Cross-shard operations and legacy system integration
**Validation:** Used for shard validation and routing

## Function Interface Definitions

### 1. Protocol Validation Functions
**Functions:**
- `validate_header`: Validates and processes request headers
  - **Parameters:** Header buffer, connection info structure
  - **Returns:** Integer error code
  - **Purpose:** Comprehensive header validation and context setup

- `validate_decrypt_body`: Validates and decrypts request bodies
  - **Parameters:** Connection info structure
  - **Returns:** Integer error code
  - **Purpose:** Body validation, decryption, and integrity checking

**Usage:** Called by protocol processing for all incoming requests
**Security:** Ensures all requests are properly validated before processing

### 2. Response Generation Functions
**Functions:**
- `get_response_header`: Generates response headers
  - **Parameters:** Response buffer, status code, command group, connection info
  - **Returns:** None
  - **Purpose:** Creates properly formatted response headers

- `prepare_response`: Prepares complete response for transmission
  - **Parameters:** Connection info structure
  - **Returns:** None
  - **Purpose:** Assembles complete response with encryption

**Usage:** Called by all command handlers for response generation
**Protocol:** Ensures consistent response formatting

### 3. Command Processing Functions
**Functions:**
- `run_command`: Executes validated commands in thread pool
  - **Parameters:** Thread argument with connection info
  - **Returns:** None
  - **Purpose:** Thread-safe command execution

- `send_command_error`: Sends error responses
  - **Parameters:** Error code, connection info structure
  - **Returns:** None
  - **Purpose:** Consistent error response handling

- `finish_command`: Completes command processing
  - **Parameters:** Connection info structure
  - **Returns:** None
  - **Purpose:** Connection cleanup and resource management

**Usage:** Core command processing pipeline
**Threading:** Designed for thread pool execution

### 4. Utility Functions
**Functions:**
- `get_body_payload`: Extracts payload from request body
  - **Parameters:** Connection info structure
  - **Returns:** Payload data pointer
  - **Purpose:** Skips challenge data to access command payload

- `write_stat`: Writes operation statistics
  - **Parameters:** Connection info structure
  - **Returns:** None
  - **Purpose:** Records operation metrics and timing

**Usage:** Helper functions for command processing
**Utility:** Provides common functionality across commands

## Ticket System Definitions

### 1. Ticket Constants
**Constants:**
- `TICKET_POOL_SIZE` (512): Maximum number of concurrent tickets
- `MAX_COINS_PER_TICKET` (4096): Maximum coins per ticket
- `TICKET_RELEASE_SECONDS` (300): Ticket expiration timeout

**Purpose:** Configures ticket system for healing operations
**Performance:** Balances memory usage with operational capacity

### 2. Ticket Entry Structure (`ticket_entry_t`)
**Purpose:** Represents a single ticket in the healing system
**Fields:**
- `created_at`: Ticket creation timestamp
- `ticket`: Unique ticket identifier (4 bytes)
- `coins`: Array of coins associated with ticket
- `claims`: Bitmap of server claims (25 bytes)
- `num_coins`: Number of coins in ticket
- `mtx`: Per-slot mutex for thread safety

**Usage:** Healing operations and inter-server communication
**Concurrency:** Per-ticket locking for high performance

### 3. Ticket Management Functions
**Functions:**
- `init_ticket_storage`: Initializes ticket memory pool
- `check_tickets`: Checks for and releases expired tickets
- `get_free_ticket_slot`: Allocates free ticket slot
- `get_ticket_entry`: Retrieves ticket by identifier
- `unlock_ticket_entry`: Releases ticket lock

**Usage:** Healing operations and ticket lifecycle management
**Thread Safety:** All functions are thread-safe with proper locking

## Command Handler Interface

### 1. Command Handler Type
**Type:** `command_handler_t`
**Signature:** Function pointer taking connection info structure
**Purpose:** Standard interface for all command processing functions
**Usage:** All command handlers must conform to this interface

### 2. Command Dispatch
**Mechanism:** Two-dimensional function pointer array
**Indexing:** Command group and command number
**Validation:** Null pointer check before dispatch
**Error Handling:** Invalid commands generate appropriate errors

## Protocol Configuration

### 1. Size Limits
**Constants:**
- `MAX_COMMAND_GROUP`: Maximum command group number
- `MAX_COMMAND`: Maximum command number (255)
- Header and body size limits
- Buffer size requirements

**Purpose:** Defines protocol limits and constraints
**Validation:** Used for input validation and buffer allocation

### 2. Timeout Values
**Constants:**
- Inter-server communication timeouts
- Ticket expiration timeouts
- Network operation timeouts
- Connection timeouts

**Purpose:** Configures timing for various operations
**Reliability:** Ensures operations complete within reasonable time

## Security Considerations

### 1. Encryption Parameters
- Nonce size for CTR mode security
- Authentication number size (16 bytes)
- Challenge/response size (16 bytes)
- Encryption type validation

### 2. Validation Requirements
- All inputs must be validated before processing
- Proper error codes for all validation failures
- Resource cleanup on all error paths
- Secure error handling without information leakage

### 3. Thread Safety
- All protocol functions must be thread-safe
- Proper locking for shared resources
- Atomic operations where required
- Resource cleanup in multithreaded environment

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

This protocol header provides the complete specification for RAIDA network communication, ensuring consistent, secure, and reliable operation across all network nodes.