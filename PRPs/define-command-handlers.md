
# Command Function Declarations Header (commands.h)

## Module Purpose
This header file defines the complete interface for all command handler functions in the RAIDA server system. It serves as the central registry of available commands organized by functional groups, providing function declarations for status monitoring, authentication, healing, administrative operations, key exchange, locker services, change operations, shard management, crossover transactions, RPC services, and filesystem operations.

## Function Categories and Declarations

### 1. Status and Monitoring Commands
**Purpose:** Server health, version, and operational status reporting

#### `cmd_echo`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure)
**Purpose:** Health check endpoint for server responsiveness verification

#### `cmd_version`
**Parameters:** Connection information structure pointer  
**Returns:** None (modifies connection structure with version data)
**Purpose:** Returns server build version information

#### `cmd_audit`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with circulation data)
**Purpose:** Provides comprehensive coin circulation audit across all denominations

#### `cmd_show_stats`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with statistics data)
**Purpose:** Returns administrative statistics (requires authentication)

### 2. Authentication Commands
**Purpose:** Coin detection and proof-of-ownership operations

#### `cmd_detect`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with detection results)
**Purpose:** Detects individual coin authenticity and ownership

#### `cmd_detect_sum`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with batch detection results)
**Purpose:** Batch detection operation for multiple coins with sum verification

#### `cmd_pown`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with ownership proof)
**Purpose:** Proves ownership of individual coin and updates authentication

#### `cmd_pown_sum`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with batch ownership results)
**Purpose:** Batch ownership proof operation with cryptographic sum verification

### 3. Healing Commands
**Purpose:** Network healing and coin recovery operations

#### `cmd_get_ticket`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with ticket data)
**Purpose:** Issues healing ticket for failed coin recovery operations

#### `cmd_validate_ticket`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with validation results)
**Purpose:** Validates healing ticket and coordinates inter-server healing

#### `cmd_find`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with search results)
**Purpose:** Searches for coins across the network for healing operations

#### `cmd_fix`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with fix results)
**Purpose:** Repairs coin data inconsistencies identified during healing

### 4. Executive/Administrative Commands
**Purpose:** System administration and coin lifecycle management

#### `cmd_get_available_sns`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with available serial numbers)
**Purpose:** Returns available serial numbers for coin creation

#### `cmd_create_coins`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with creation results)
**Purpose:** Creates new coins in the system (administrative operation)

#### `cmd_delete_coins`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with deletion results)
**Purpose:** Deletes coins from the system (administrative operation)

#### `cmd_free_coins`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with liberation results)
**Purpose:** Frees coins for reuse (administrative operation)

#### `cmd_get_all_sns`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with complete serial number list)
**Purpose:** Returns comprehensive serial number inventory

### 5. Key Exchange Commands
**Purpose:** Cryptographic key management and secure communication

#### `cmd_encrypt_key`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with encrypted key)
**Purpose:** Generates and encrypts session keys for secure communication

#### `cmd_post_key`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with storage confirmation)
**Purpose:** Stores cryptographic keys in the chat system

#### `cmd_get_key`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with retrieved key data)
**Purpose:** Retrieves stored cryptographic keys from chat system

#### `cmd_key_alert`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure)
**Purpose:** Handles key alert notifications

#### `cmd_decrypt_raida_key`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with decryption results)
**Purpose:** Decrypts messages from other RAIDA servers

### 6. Locker Service Commands
**Purpose:** Secure coin storage and trading marketplace operations

#### `cmd_store_sum`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with storage results)
**Purpose:** Stores collection of coins in secure locker

#### `cmd_store_multiple_sum`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with batch storage results)
**Purpose:** Batch storage operation for multiple lockers

#### `cmd_peek`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with locker contents)
**Purpose:** Inspects contents of locker without transfer

#### `cmd_remove`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with removal results)
**Purpose:** Removes coins from locker and transfers ownership

#### `cmd_put_for_sale`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with sale listing confirmation)
**Purpose:** Creates trade locker for marketplace sales

#### `cmd_list_lockers_for_sale`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with marketplace listings)
**Purpose:** Lists available trade lockers by currency type

#### `cmd_buy`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with purchase results)
**Purpose:** Executes purchase of trade locker

#### `cmd_remove_trade_locker`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with removal confirmation)
**Purpose:** Removes trade locker from marketplace

#### `cmd_peek_trade_locker`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with trade locker contents)
**Purpose:** Inspects trade locker contents without affecting sale status

### 7. Change Operations Commands
**Purpose:** Coin denomination conversion and value exchange

#### `cmd_get_available_change_sns`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with available change serial numbers)
**Purpose:** Returns serial numbers available for change operations

#### `cmd_break`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with break operation results)
**Purpose:** Breaks larger denomination coins into smaller ones

#### `cmd_join`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with join operation results)
**Purpose:** Combines smaller denomination coins into larger ones

### 8. Shard Management Commands
**Purpose:** Cross-blockchain coin migration and shard operations

#### `cmd_switch_shard_sum`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure)
**Purpose:** Legacy shard switch operation (stub implementation)

#### `cmd_pickup_coins`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with pickup results)
**Purpose:** Takes ownership of coins transferred from another shard

#### `cmd_get_sns`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with available serial numbers)
**Purpose:** Returns available serial numbers for shard operations

#### `cmd_rollback_switch_shard`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure)
**Purpose:** Rolls back failed shard switch operations (stub implementation)

#### `cmd_switch_shard_sum_with_sns`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with migration results)
**Purpose:** Executes shard migration with serial number specification

### 9. Crossover Transaction Commands
**Purpose:** Inter-blockchain value transfer and exchange operations

#### `cmd_reserve_locker`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with reservation confirmation)
**Purpose:** Reserves locker for crossover transactions

#### `cmd_check_depository`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with depository status)
**Purpose:** Checks blockchain transaction confirmation status

#### `cmd_withdraw_from_depository`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with withdrawal results)
**Purpose:** Withdraws cryptocurrency from depository

#### `cmd_trigger_transaction`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure)
**Purpose:** Triggers crossover transaction execution

#### `cmd_get_exchange_rate`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with exchange rate data)
**Purpose:** Returns current cryptocurrency exchange rates

### 10. RPC Service Commands
**Purpose:** Remote procedure call and network utility operations

#### `cmd_nslookup`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with DNS lookup results)
**Purpose:** Performs DNS lookups for network operations

### 11. Filesystem Commands
**Purpose:** Object storage and file management operations

#### `cmd_put_object`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with storage confirmation)
**Purpose:** Stores objects in filesystem

#### `cmd_get_object`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with object data)
**Purpose:** Retrieves objects from filesystem

#### `cmd_rm_object`
**Parameters:** Connection information structure pointer
**Returns:** None (modifies connection structure with deletion confirmation)
**Purpose:** Removes objects from filesystem

## Supporting Function Declarations

### Key Exchange Utilities

#### `load_my_enc_coin`
**Parameters:**
- Denomination identifier (8-bit unsigned integer)
- Serial number (32-bit unsigned integer)
- Output buffer pointer (minimum 400 bytes)

**Returns:** Integer status code (0 for success, negative for error)
**Purpose:** Loads encryption coin data from local filesystem for cryptographic operations

#### `get_crypto_key`
**Parameters:**
- Currency ticker string pointer
- Size output pointer (integer)

**Returns:** Character buffer pointer (NULL on failure)
**Purpose:** Retrieves cryptocurrency private keys for withdrawal operations

### Threading Support Structures

#### `validate_ticket_arg_t`
**Purpose:** Structure for passing parameters to threaded ticket validation operations
**Members:**
- RAIDA server index (8-bit signed integer)
- Ticket identifier (32-bit unsigned integer) 
- Connection information pointer
- Result coin array pointer
- Result coin count (32-bit unsigned integer)

#### `send_validate_ticket_job`
**Parameters:** Thread argument structure pointer
**Returns:** Thread result pointer
**Purpose:** Worker thread function for distributed ticket validation

### Data Structures

#### `coin_counter_t`
**Purpose:** Structure for counting coins by type
**Members:**
- Coin identification structure
- Count value (32-bit unsigned integer)

## Integration and Dependencies

## data structure context 
`ConnectionContext` (conn_info_t)
`body_size`: Size of incoming request body
`command_status`: Numeric status/result code

`output`: Byte array or structured result

`output_size`: Size of output buffer

`CoinRecord`:Contains metadata used to represent a coin (format defined by protocol layer)

`Statistics`:Collection of counters and metrics for each command

`CoinCounter`
`coin`: CoinRecord

`count`: Number of matching coins found

`TicketValidationParams`:
`raida_index`: Server index initiating the validation

`ticket`: Ticket ID

`connection`: Connection context

`result_coins`: Output list of verified coins

`result_count`: Total number of coins validated

### Required External Types
- Connection information structure from protocol layer
- Coin identification structure from protocol definitions
- Database page structures from database layer
- Thread parameter structures for concurrent operations

### Protocol Integration
- All command functions follow standard signature pattern ( Every command function receives a request context (connection structure) and populates its output buffer    and status code in place. No values are returned from the function.”)
- Connection structure modification for response data
- Status code setting for operation results
- Output buffer management for response payloads

### Module Dependencies
- **Protocol Layer:** Connection management, request/response handling
- **Database Layer:** Coin data access, page cache management
- **Cryptographic Utilities:** Key generation, encryption/decryption
- **Network Layer:** Inter-server communication
- **Filesystem Interface:** Object storage, key management
- **Threading System:** Concurrent operation support

### Used By
- **Command Dispatcher:** Function pointer resolution and invocation
- **Protocol Handler:** Command routing and execution
- **Network Layer:** Request processing and response generation

## Security Considerations

### Authentication Requirements
- Administrative commands require authentication key validation
- Executive operations restricted to authorized users
- Key exchange operations require proper encryption coin possession

### Input Validation
- All functions must validate connection structure integrity
- Payload size validation required before processing
- Parameter bounds checking for all numeric inputs

### Error Handling
- Consistent error code reporting through connection structure
- Resource cleanup on operation failure
- Security event logging for administrative operations

###  Command Organization
Commands are organized into the following functional categories:
- **Status**: System status and diagnostics (4 commands)
- **Authentication**: Coin authentication and ownership (4 commands)
- **Healing**: Coin repair and recovery (4 commands)
- **Executive**: Administrative coin operations (5 commands)
- **Key Exchange**: Cryptographic key management (7 commands)
- **Locker**: Coin storage and trading (9 commands)
- **Change**: Denomination conversion (3 commands)
- **Shards**: Shard management (5 commands)
- **Crossover**: Cross-system operations (5 commands)
- **RPC**: Remote procedure calls (1 command)
- **Filesystem**: Object storage operations (3 commands)

This header provides the complete command interface definition for the RAIDA server system, enabling modular command implementation while maintaining consistent function signatures and error handling patterns across all operational categories.