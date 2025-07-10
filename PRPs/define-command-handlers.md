# Specification: RAIDAX Commands Header Interface (commands.h)

# Implementation Prompt: CloudCoin Commands Interface

## 1. Module Purpose
This prompt defines the requirements for implementing the CloudCoin commands interface layer. It establishes the command function signatures, data structures, and external dependencies required for implementing a comprehensive CloudCoin protocol command system based on the provided header interface.

## 2. System Dependencies

### 2.1 Required External Headers
```
protocol.h - Must be included for conn_info_t and coin_t type definitions
```

### 2.2 Core Data Types
The following types are used throughout the interface:
- **conn_info_t**: Connection information structure (defined in protocol.h)
- **coin_t**: Coin data structure (defined in protocol.h)
- **uint8_t**: 8-bit unsigned integer
- **uint32_t**: 32-bit unsigned integer
- **int8_t**: 8-bit signed integer

## 3. Core Data Structures

### 3.1 Coin Counter Structure
```
coin_counter_structure:
    coin: coin_t                    // Coin data structure
    cnt: unsigned_32bit_integer     // Counter value
```

### 3.2 Validate Ticket Argument Structure
```
validate_ticket_arg_structure:
    raida_idx: signed_8bit_integer          // RAIDA index
    ticket: unsigned_32bit_integer          // Ticket identifier
    ci: pointer to conn_info_t              // Connection information
    rv_coins: pointer to coin_t             // Return value coins array
    rv_num_coins: unsigned_32bit_integer    // Number of return value coins
```

## 4. Available Functions - DECLARED FOR EXTERNAL USE

### 4.1 Data Structure Access
The coin_counter_structure and validate_ticket_arg_structure can be accessed through direct field access.

## 5. External Dependencies - CALLED FROM EXTERNAL SOURCES

### 5.1 Command Functions (Implemented Elsewhere)
The following functions are declared in the header and implemented in external commands module:

#### 5.1.1 Status Functions
- **cmd_echo(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle echo command for connection testing

- **cmd_version(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle version command to return system version information

- **cmd_audit(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle audit command for system auditing

- **cmd_show_stats(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle show statistics command

#### 5.1.2 Authentication Functions
- **cmd_detect(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle detect command for coin authentication

- **cmd_detect_sum(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle detect sum command for batch coin authentication

- **cmd_pown(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle pown command for coin ownership verification

- **cmd_pown_sum(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle pown sum command for batch ownership verification

#### 5.1.3 Healing Functions
- **cmd_get_ticket(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle get ticket command for healing process initiation

- **cmd_validate_ticket(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle validate ticket command for healing verification

- **cmd_find(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle find command for locating coins

- **cmd_fix(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle fix command for coin repair operations

#### 5.1.4 Executive Functions
- **cmd_get_available_sns(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle get available serial numbers command

- **cmd_create_coins(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle create coins command for new coin generation

- **cmd_delete_coins(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle delete coins command for coin removal

- **cmd_free_coins(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle free coins command for coin liberation

- **cmd_get_all_sns(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle get all serial numbers command

#### 5.1.5 Key Exchange Functions
- **cmd_encrypt_key(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle encrypt key command for key encryption

- **cmd_post_key(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle post key command for key submission

- **cmd_chat_get_key(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle chat get key command for chat key retrieval

- **cmd_chat_post_key(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle chat post key command for chat key submission

- **cmd_get_key(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle get key command for key retrieval

- **cmd_key_alert(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle key alert command for key notifications

- **cmd_decrypt_raida_key(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle decrypt RAIDA key command

#### 5.1.6 Locker Functions
- **cmd_store_sum(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle store sum command for coin storage

- **cmd_store_multiple_sum(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle store multiple sum command for batch coin storage

- **cmd_peek(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle peek command for locker inspection

- **cmd_remove(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle remove command for coin removal from locker

- **cmd_put_for_sale(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle put for sale command for trade locker creation

- **cmd_list_lockers_for_sale(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle list lockers for sale command

- **cmd_buy(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle buy command for purchasing from trade locker

- **cmd_remove_trade_locker(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle remove trade locker command

- **cmd_peek_trade_locker(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle peek trade locker command for trade locker inspection

#### 5.1.7 Change Functions
- **cmd_get_available_change_sns(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle get available change serial numbers command

- **cmd_break(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle break command for coin denomination breaking

- **cmd_join(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle join command for coin denomination joining

#### 5.1.8 Shards Functions
- **cmd_switch_shard_sum(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle switch shard sum command for shard switching

- **cmd_pickup_coins(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle pickup coins command for coin retrieval

- **cmd_get_sns(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle get serial numbers command

- **cmd_rollback_switch_shard(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle rollback switch shard command for shard switch reversal

- **cmd_switch_shard_sum_with_sns(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle switch shard sum with serial numbers command

#### 5.1.9 Crossover Functions
- **cmd_reserve_locker(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle reserve locker command for locker reservation

- **cmd_check_depository(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle check depository command for depository status check

- **cmd_withdraw_from_depository(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle withdraw from depository command

- **cmd_trigger_transaction(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle trigger transaction command for transaction initiation

- **cmd_get_exchange_rate(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle get exchange rate command for rate retrieval

#### 5.1.10 RPC Functions
- **cmd_nslookup(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle DNS lookup command for name resolution

#### 5.1.11 Filesystem Functions
- **cmd_put_object(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle put object command for object storage

- **cmd_get_object(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle get object command for object retrieval

- **cmd_rm_object(conn_info_pointer)** → void
  - Parameters: pointer to conn_info_t
  - Purpose: Handle remove object command for object deletion

#### 5.1.12 Utility Functions
- **load_my_enc_coin(denomination, serial_number, buffer)** → integer
  - Parameters: unsigned_8bit_integer denomination, unsigned_32bit_integer serial_number, pointer to unsigned_char buffer
  - Purpose: Load encrypted coin data into buffer

- **get_crypto_key(key_identifier, key_length)** → char_pointer
  - Parameters: pointer to char key_identifier, pointer to integer key_length
  - Purpose: Retrieve cryptographic key by identifier

#### 5.1.13 Threading Functions
- **send_validate_ticket_job(thread_argument)** → void_pointer
  - Parameters: void_pointer thread_argument
  - Purpose: Background thread function for validate ticket job processing

### 5.2 External Header Dependencies
The following are included from external headers:
- **protocol.h**: Provides conn_info_t and coin_t type definitions
- Standard integer types (uint8_t, uint32_t, int8_t)

### 5.3 Platform Dependencies
The following may be called from platform libraries:
- Memory allocation functions
- Network communication functions
- File I/O operations
- Threading primitives
- Cryptographic functions

## 6. Data Type Requirements

### 6.1 Standard Integer Types
- **int8_t**: 8-bit signed integer (-128 to 127)
- **uint8_t**: 8-bit unsigned integer (0 to 255)
- **uint32_t**: 32-bit unsigned integer (0 to 4294967295)

### 6.2 External Type Dependencies
- **conn_info_t**: Connection information structure (defined in protocol.h)
- **coin_t**: Coin data structure (defined in protocol.h)
- **char**: Character type for string operations
- **void**: Void type for functions with no return value

## 7. Integration Requirements

### 7.1 Required Dependencies
Implementations must include:
- protocol.h header file
- Standard integer type definitions
- Platform-specific networking libraries
- Platform-specific threading libraries

### 7.2 Command Processing Architecture
- All command functions take conn_info_t pointer as parameter
- Command functions are void return type (response via connection)
- Connection information structure handles input/output
- Commands are categorized by functional area

## 8. Functional Categories

### 8.1 Command Organization
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

### 8.2 Data Structure Usage
- **coin_counter_t**: Used for counting coin operations
- **validate_ticket_arg_t**: Used for threaded ticket validation operations

This implementation prompt provides complete interface definition for implementing a CloudCoin commands system. Developers should implement all specified functions while integrating with the protocol.h definitions and platform-specific libraries.