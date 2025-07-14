# Command Interface Definitions (commands.h)

## Module Purpose
This module defines the complete interface for all RAIDA command handlers, providing function declarations, data structures, and type definitions for the entire command processing system. It serves as the central interface specification for all command groups including status, authentication, healing, executive, key exchange, banking, locker, change-making, shard management, and integrity operations.

## Core Function Categories

### 1. Status Command Handlers
**Functions:**
- `cmd_echo`: Echo service for connectivity testing
- `cmd_version`: Server version information retrieval
- `cmd_audit`: System audit and health check operations
- `cmd_show_stats`: Statistical information display

**Parameters:** All status commands take connection information structure
**Returns:** None (modify connection structure with results)
**Purpose:** Provide system status, health monitoring, and basic connectivity verification

### 2. Authentication Command Handlers
**Functions:**
- `cmd_detect`: Individual coin authenticity verification
- `cmd_detect_sum`: Batch coin authenticity verification using cryptographic sums
- `cmd_pown`: Individual coin ownership transfer operations
- `cmd_pown_sum`: Batch coin ownership transfer using delta transformations

**Parameters:** All authentication commands take connection information structure
**Returns:** None (modify connection structure with authentication results)
**Purpose:** Core coin authentication and ownership management functionality

### 3. Healing Command Handlers
**Functions:**
- `cmd_get_ticket`: Ticket acquisition for healing operations
- `cmd_validate_ticket`: Ticket validation by other RAIDA servers
- `cmd_find`: Coin discovery and location operations
- `cmd_fix`: Distributed consensus-based coin recovery

**Parameters:** All healing commands take connection information structure
**Returns:** None (modify connection structure with healing results)
**Purpose:** Distributed coin recovery and network healing operations

**Threading Functions:**
- `send_validate_ticket_job`: Thread worker for parallel ticket validation
  - **Parameters:** Thread argument structure containing server info and ticket data
  - **Returns:** Thread result (modifies argument structure with response)
  - **Purpose:** Parallel network communication for consensus operations

### 4. Executive Command Handlers
**Functions:**
- `cmd_get_available_sns`: Available serial number query for administrative operations
- `cmd_create_coins`: Administrative coin creation with secure authentication numbers
- `cmd_delete_coins`: Administrative coin deletion with authentication verification
- `cmd_free_coins`: Administrative coin liberation without authentication
- `cmd_get_all_sns`: Complete denomination bitmap retrieval

**Parameters:** All executive commands take connection information structure
**Returns:** None (modify connection structure with administrative results)
**Purpose:** High-level administrative coin management and system maintenance

### 5. Key Exchange Command Handlers
**Functions:**
- `cmd_encrypt_key`: Key encryption operations for secure communication
- `cmd_post_key`: Key posting and distribution operations
- `cmd_chat_get_key`: Chat system key retrieval
- `cmd_chat_post_key`: Chat system key posting
- `cmd_get_key`: General key retrieval operations
- `cmd_key_alert`: Key change notification system
- `cmd_decrypt_raida_key`: RAIDA-specific key decryption

**Parameters:** All key exchange commands take connection information structure
**Returns:** None (modify connection structure with key exchange results)
**Purpose:** Secure key management and cryptographic communication setup

**Helper Functions:**
- `load_my_enc_coin`: Load encrypted coin data for key operations
  - **Parameters:** Denomination (1 byte), serial number (4 bytes), output buffer (16 bytes)
  - **Returns:** Integer status code
  - **Purpose:** Retrieve encrypted coin data for cryptographic operations

### 6. Locker Command Handlers
**Functions:**
- `cmd_store_sum`: Store coins in locker with sum verification
- `cmd_store_multiple_sum`: Store multiple coin sets in locker
- `cmd_peek`: Examine locker contents without removal
- `cmd_remove`: Remove coins from locker
- `cmd_put_for_sale`: Place locker for sale in marketplace
- `cmd_list_lockers_for_sale`: List available lockers for purchase
- `cmd_buy`: Purchase locker from marketplace
- `cmd_remove_trade_locker`: Remove locker from trading system
- `cmd_peek_trade_locker`: Examine trade locker contents

**Parameters:** All locker commands take connection information structure
**Returns:** None (modify connection structure with locker operation results)
**Purpose:** Secure coin storage, trading, and marketplace operations

### 7. Change-Making Command Handlers
**Functions:**
- `cmd_get_available_change_sns`: Available serial numbers for change operations
- `cmd_break`: Break larger denomination coin into smaller coins
- `cmd_join`: Join smaller denomination coins into larger coin

**Parameters:** All change commands take connection information structure
**Returns:** None (modify connection structure with change operation results)
**Purpose:** Denomination conversion and flexible value management

### 8. Shard Management Command Handlers
**Functions:**
- `cmd_switch_shard_sum`: Switch coins between shards (legacy interface)
- `cmd_pickup_coins`: Take ownership of coins moved from another shard
- `cmd_get_sns`: Get available serial numbers for shard operations
- `cmd_rollback_switch_shard`: Rollback failed shard switching operations
- `cmd_switch_shard_sum_with_sns`: Switch coins between shards with serial number management

**Parameters:** All shard commands take connection information structure
**Returns:** None (modify connection structure with shard operation results)
**Purpose:** Cross-shard operations and legacy system integration

### 9. Crossover Command Handlers
**Functions:**
- `cmd_reserve_locker`: Reserve locker for crossover operations
- `cmd_check_depository`: Check depository status and contents
- `cmd_withdraw_from_depository`: Withdraw coins from depository
- `cmd_trigger_transaction`: Trigger crossover transaction execution
- `cmd_get_exchange_rate`: Get current exchange rates

**Parameters:** All crossover commands take connection information structure
**Returns:** None (modify connection structure with crossover operation results)
**Purpose:** Cross-platform integration and external system interfaces

### 10. RPC Command Handlers
**Functions:**
- `cmd_nslookup`: DNS lookup and name resolution services

**Parameters:** All RPC commands take connection information structure
**Returns:** None (modify connection structure with RPC results)
**Purpose:** Network services and remote procedure call operations

### 11. Filesystem Command Handlers
**Functions:**
- `cmd_put_object`: Store object in distributed filesystem
- `cmd_get_object`: Retrieve object from distributed filesystem
- `cmd_rm_object`: Remove object from distributed filesystem

**Parameters:** All filesystem commands take connection information structure
**Returns:** None (modify connection structure with filesystem operation results)
**Purpose:** Distributed file storage and retrieval operations

**Helper Functions:**
- `get_crypto_key`: Retrieve cryptographic key for filesystem operations
  - **Parameters:** Key identifier string, output length pointer
  - **Returns:** Key data pointer (or null on failure)
  - **Purpose:** Cryptographic key management for secure file operations

### 12. Integrity Command Handlers
**Functions:**
- `cmd_get_merkle_root`: Retrieve Merkle tree root hash for denomination
- `cmd_get_merkle_node`: Retrieve specific Merkle tree node hash

**Parameters:** All integrity commands take connection information structure
**Returns:** None (modify connection structure with integrity verification results)
**Purpose:** Cryptographic integrity verification and network synchronization

## Data Structures and Type Definitions

### Connection Information Structure
**Type:** `conn_info_t`
**Purpose:** Contains all request/response data and connection state information
**Used By:** All command handlers as primary parameter
**Contains:**
- Request payload data and size
- Response buffer and size
- Connection state and networking information
- Authentication and encryption context
- Timing and statistical information

### Coin Counter Structure
**Type:** `coin_counter_t`
**Purpose:** Tracks coin information and vote counts for consensus operations
**Fields:**
- `coin`: Basic coin structure with denomination and serial number
- `cnt`: Vote count for consensus operations
**Used By:** Healing operations for distributed consensus tracking

### Ticket Validation Arguments
**Type:** `validate_ticket_arg_t`
**Purpose:** Thread argument structure for parallel ticket validation
**Fields:**
- `raida_idx`: Target RAIDA server index (1 byte)
- `ticket`: Ticket identifier (4 bytes)
- `ci`: Connection information pointer
- `rv_coins`: Response coin array pointer
- `rv_num_coins`: Response coin count (4 bytes)
**Used By:** Healing operations for parallel server communication

### Basic Coin Structure
**Type:** `coin_t`
**Purpose:** Fundamental coin identification structure
**Fields:**
- `denomination`: Coin denomination identifier (1 byte)
- `sn`: Serial number (4 bytes)
**Used By:** All coin-related operations throughout the system

## Function Pointer Types

### Command Handler Type
**Type:** `command_handler_t`
**Signature:** `void (*handler)(conn_info_t *)`
**Purpose:** Standard interface for all command processing functions
**Used By:** Protocol system for command dispatch and execution

## Integration Requirements

### Protocol Integration
- All command handlers must conform to standard `command_handler_t` interface
- All handlers receive `conn_info_t` structure containing request data
- All handlers modify connection structure with response data and status
- All handlers must handle error conditions gracefully

### Database Integration
- Commands requiring coin data must use database layer for page access
- Page locking must be used for thread-safe coin data access
- Modified pages must be marked as dirty for persistence
- Proper resource cleanup required on all code paths

### Security Integration
- Authentication commands must verify cryptographic signatures
- Administrative commands must validate admin keys
- All sensitive operations must use secure random number generation
- Error conditions must not leak sensitive information

### Statistics Integration
- Successful operations must update relevant statistics counters
- Value-based operations must update value tracking statistics
- Performance metrics must be recorded for monitoring
- Error rates must be tracked for health monitoring

## Error Handling Standards

### Input Validation
- All payloads must be validated for size and alignment
- All parameters must be checked against acceptable ranges
- All authentication data must be cryptographically verified
- All resource allocations must be checked for success

### Error Reporting
- All errors must use standard error codes from protocol definitions
- Error conditions must be logged appropriately
- Resource cleanup must occur on all error paths
- Connection state must be properly set for error responses

### Recovery Mechanisms
- Failed operations must leave system state unchanged
- Partial operations must be rolled back on failure
- Resources must be properly released on all code paths
- Error conditions must not corrupt system state

## Threading and Concurrency

### Thread Safety Requirements
- All command handlers must be thread-safe
- Database page locking must be used for coin data access
- Shared resources must be properly synchronized
- Thread-local storage must be used where appropriate

### Parallel Processing
- Some operations support parallel execution (healing consensus)
- Thread pools may be used for concurrent request processing
- Network operations may use separate threads for efficiency
- Blocking operations must not interfere with other requests

### Resource Management
- All allocated memory must be properly freed
- All acquired locks must be properly released
- All open files or network connections must be closed
- Thread resources must be cleaned up on exit

## Dependencies and Integration Points

### Required Modules
- **Protocol System:** For request/response handling and command dispatch
- **Database Layer:** For coin data access and page management
- **Network Layer:** For inter-server communication and client connections
- **Cryptographic Utilities:** For secure operations and authentication
- **Configuration System:** For server parameters and validation
- **Statistics System:** For operation tracking and monitoring
- **Logging System:** For debugging and audit trail

### External Interfaces
- **Legacy Systems:** Integration with CloudCoin v1 and SuperCoin systems
- **Cryptographic Libraries:** SHA-256, AES, and other cryptographic functions
- **Network Libraries:** TCP/UDP communication and protocol handling
- **File System:** For persistent storage and data management
- **Threading Libraries:** For concurrent operation support

### Configuration Dependencies
- Server identification and network configuration
- Cryptographic key management and validation
- Database configuration and storage parameters
- Network timeouts and operational parameters
- Administrative authentication and access control

This command interface module provides the complete specification for all RAIDA command operations, ensuring consistent interfaces, proper error handling, and secure operation across the entire system.