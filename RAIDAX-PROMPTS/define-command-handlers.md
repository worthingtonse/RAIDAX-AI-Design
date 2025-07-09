# Language-Agnostic Specification: Commands Header Interface

## 1. Module Purpose
This specification defines the complete public interface for all CloudCoin RAIDA command handlers. It establishes the function signatures and data structures required to implement the full command set across 14 command groups, supporting authentication, healing, administrative, filesystem, and specialized operations.

## 2. Command Function Interface

### 2.1 Status Commands (Group 0)
Basic system status and diagnostic operations:

#### 2.1.1 cmd_echo(connection_info)
```
Function: cmd_echo
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Echo request data back to client for connectivity testing
```

#### 2.1.2 cmd_version(connection_info)
```
Function: cmd_version
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Return system version information
```

#### 2.1.3 cmd_audit(connection_info)
```
Function: cmd_audit
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Perform system audit and return results
```

#### 2.1.4 cmd_show_stats(connection_info)
```
Function: cmd_show_stats
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Return system performance and operation statistics
```

### 2.2 Authentication Commands (Group 1)
Core coin authentication and ownership operations:

#### 2.2.1 cmd_detect(connection_info)
```
Function: cmd_detect
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Validate coin authenticity without ownership transfer
```

#### 2.2.2 cmd_detect_sum(connection_info)
```
Function: cmd_detect_sum
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Batch coin validation using XOR checksum
```

#### 2.2.3 cmd_pown(connection_info)
```
Function: cmd_pown
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Validate and transfer coin ownership
```

#### 2.2.4 cmd_pown_sum(connection_info)
```
Function: cmd_pown_sum
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Batch ownership transfer using XOR-based authentication
```

### 2.3 Healing Commands (Group 2)
Distributed coin recovery and repair operations:

#### 2.3.1 cmd_get_ticket(connection_info)
```
Function: cmd_get_ticket
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Create healing ticket for authenticated coins
```

#### 2.3.2 cmd_validate_ticket(connection_info)
```
Function: cmd_validate_ticket
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Validate ticket authenticity and mark RAIDA claims
```

#### 2.3.3 cmd_find(connection_info)
```
Function: cmd_find
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Determine coin authentication state (AN vs PAN)
```

#### 2.3.4 cmd_fix(connection_info)
```
Function: cmd_fix
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Perform distributed coin recovery using consensus
```

#### 2.3.5 send_validate_ticket_job(thread_argument)
```
Function: send_validate_ticket_job
Parameters: arg (generic pointer to validate_ticket_arg_structure)
Returns: platform_thread_return_type
Purpose: Thread function for cross-RAIDA ticket validation
Thread Context: Executed in separate thread for parallel processing
```

### 2.4 Executive Commands (Group 3)
Administrative coin management operations:

#### 2.4.1 cmd_get_available_sns(connection_info)
```
Function: cmd_get_available_sns
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Return available serial numbers for coin creation
```

#### 2.4.2 cmd_create_coins(connection_info)
```
Function: cmd_create_coins
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Create new coins in the system
```

#### 2.4.3 cmd_delete_coins(connection_info)
```
Function: cmd_delete_coins
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Remove coins from the system
```

#### 2.4.4 cmd_free_coins(connection_info)
```
Function: cmd_free_coins
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Release coins from reserved state
```

#### 2.4.5 cmd_get_all_sns(connection_info)
```
Function: cmd_get_all_sns
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Return all serial numbers in the system
```

### 2.5 Key Exchange Commands (Group 4)
Cryptographic key management operations:

#### 2.5.1 cmd_encrypt_key(connection_info)
```
Function: cmd_encrypt_key
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Encrypt key data using system cryptography
```

#### 2.5.2 cmd_post_key(connection_info)
```
Function: cmd_post_key
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Store cryptographic key in system
```

#### 2.5.3 cmd_get_key(connection_info)
```
Function: cmd_get_key
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Retrieve stored cryptographic key
```

#### 2.5.4 cmd_key_alert(connection_info)
```
Function: cmd_key_alert
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Process key-related alert or notification
```

#### 2.5.5 cmd_decrypt_raida_key(connection_info)
```
Function: cmd_decrypt_raida_key
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Decrypt RAIDA-specific key data
```

#### 2.5.6 load_my_enc_coin(denomination, serial_number, output_buffer)
```
Function: load_my_enc_coin
Parameters:
    denomination: unsigned_8bit_integer
    serial_number: unsigned_32bit_integer
    output_buffer: pointer_to_byte_array
Returns: integer (0 = success, negative = error)
Purpose: Load encryption coin data for key operations
```

#### 2.5.7 cmd_chat_get_key(connection_info)
```
Function: cmd_chat_get_key
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Retrieve key for chat/communication purposes
```

#### 2.5.8 cmd_chat_post_key(connection_info)
```
Function: cmd_chat_post_key
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Store key for chat/communication purposes
```

### 2.6 Locker Commands (Group 8)
Coin storage and trading operations:

#### 2.6.1 cmd_store_sum(connection_info)
```
Function: cmd_store_sum
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Store coins in standard locker using sum validation
```

#### 2.6.2 cmd_store_multiple_sum(connection_info)
```
Function: cmd_store_multiple_sum
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Create multiple lockers in batch operation
```

#### 2.6.3 cmd_peek(connection_info)
```
Function: cmd_peek
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: View contents of standard locker (read-only)
```

#### 2.6.4 cmd_remove(connection_info)
```
Function: cmd_remove
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Remove coins from standard locker
```

#### 2.6.5 cmd_put_for_sale(connection_info)
```
Function: cmd_put_for_sale
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Create trade locker for selling coins
```

#### 2.6.6 cmd_list_lockers_for_sale(connection_info)
```
Function: cmd_list_lockers_for_sale
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: List available trade lockers by currency type
```

#### 2.6.7 cmd_buy(connection_info)
```
Function: cmd_buy
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Purchase trade locker with payment verification
```

#### 2.6.8 cmd_remove_trade_locker(connection_info)
```
Function: cmd_remove_trade_locker
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Remove coins from trade locker
```

#### 2.6.9 cmd_peek_trade_locker(connection_info)
```
Function: cmd_peek_trade_locker
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: View contents of trade locker (read-only)
```

### 2.7 Change Commands (Group 9)
Denomination conversion operations:

#### 2.7.1 cmd_get_available_change_sns(connection_info)
```
Function: cmd_get_available_change_sns
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Get available serial numbers for denomination changes
```

#### 2.7.2 cmd_break(connection_info)
```
Function: cmd_break
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Break larger denomination into smaller denominations
```

#### 2.7.3 cmd_join(connection_info)
```
Function: cmd_join
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Combine smaller denominations into larger denomination
```

### 2.8 Shard Commands (Group 10)
Cross-shard coin management operations:

#### 2.8.1 cmd_switch_shard_sum(connection_info)
```
Function: cmd_switch_shard_sum
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Move coins between shards using sum validation
```

#### 2.8.2 cmd_pickup_coins(connection_info)
```
Function: cmd_pickup_coins
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Retrieve coins from cross-shard operation
```

#### 2.8.3 cmd_get_sns(connection_info)
```
Function: cmd_get_sns
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Get serial numbers for shard operations
```

#### 2.8.4 cmd_rollback_switch_shard(connection_info)
```
Function: cmd_rollback_switch_shard
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Reverse shard switch operation
```

#### 2.8.5 cmd_switch_shard_sum_with_sns(connection_info)
```
Function: cmd_switch_shard_sum_with_sns
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Move coins between shards with specific serial numbers
```

### 2.9 Crossover Commands (Group 11)
Cross-network and external integration operations:

#### 2.9.1 cmd_reserve_locker(connection_info)
```
Function: cmd_reserve_locker
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Reserve locker for cross-network operations
```

#### 2.9.2 cmd_check_depository(connection_info)
```
Function: cmd_check_depository
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Check status of external depository
```

#### 2.9.3 cmd_withdraw_from_depository(connection_info)
```
Function: cmd_withdraw_from_depository
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Withdraw funds from external depository
```

#### 2.9.4 cmd_trigger_transaction(connection_info)
```
Function: cmd_trigger_transaction
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Initiate cross-network transaction
```

#### 2.9.5 cmd_get_exchange_rate(connection_info)
```
Function: cmd_get_exchange_rate
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Get current exchange rates for currency conversion
```

### 2.10 RPC Commands (Group 12)
Remote procedure call operations:

#### 2.10.1 cmd_nslookup(connection_info)
```
Function: cmd_nslookup
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Perform DNS lookup operations
```

### 2.11 Filesystem Commands (Group 13)
Secure file management operations:

#### 2.11.1 cmd_put_object(connection_info)
```
Function: cmd_put_object
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Store file in sandboxed filesystem with authentication
```

#### 2.11.2 cmd_get_object(connection_info)
```
Function: cmd_get_object
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Retrieve file from sandboxed filesystem with authentication
```

#### 2.11.3 cmd_rm_object(connection_info)
```
Function: cmd_rm_object
Parameters: ci (pointer to connection_info_structure)
Returns: void
Purpose: Remove file from sandboxed filesystem with authentication
```

#### 2.11.4 get_crypto_key(ticker, size_output)
```
Function: get_crypto_key
Parameters:
    ticker: pointer_to_character_array (key identifier)
    size_output: pointer_to_integer (for returning size)
Returns: pointer_to_character_array (NULL on error)
Purpose: Retrieve cryptographic key content from filesystem
Memory Management: Caller must free returned buffer
```

## 3. Supporting Data Structures

### 3.1 Coin Counter Structure
```
coin_counter_structure:
    coin: coin_structure                    // Coin identification
    cnt: unsigned_32bit_integer            // Vote count for consensus operations
```

### 3.2 Validate Ticket Arguments Structure
```
validate_ticket_arg_structure:
    raida_idx: signed_8bit_integer         // Target RAIDA server index
    ticket: unsigned_32bit_integer         // Ticket identifier
    ci: pointer_to_connection_info         // Original connection context
    rv_coins: pointer_to_coin_array        // Returned coin data
    rv_num_coins: unsigned_32bit_integer   // Number of returned coins
```

## 4. Function Implementation Requirements

### 4.1 Universal Requirements
All command functions must:
- Accept single connection_info parameter
- Return void
- Set ci->command_status to appropriate status code
- Set ci->output and ci->output_size for response data
- Handle all error conditions gracefully
- Free allocated resources on all code paths

### 4.2 Threading Requirements
- **Most commands**: Execute in worker thread context
- **send_validate_ticket_job**: Execute in separate thread for parallel processing
- **Thread safety**: Access to shared resources must be synchronized
- **Resource management**: Thread-local allocations must be cleaned up

### 4.3 Memory Management
- **Dynamic allocation**: Use malloc/free for variable-size responses
- **Error cleanup**: Free all allocated memory on error paths
- **Output buffers**: Allocated by command handlers, freed by protocol layer
- **Input validation**: Validate all input before allocation

## 5. Integration Requirements

### 5.1 Protocol Layer Integration
All commands must integrate with:
- **Connection Info**: Access to request body, response output, status codes
- **Protocol Functions**: get_body_payload(), status code constants
- **Error Handling**: Standard error response mechanism
- **Encryption**: Support for encrypted request/response handling

### 5.2 Database Layer Integration
Commands requiring database access must use:
- **Page Management**: get_page_by_sn_lock(), unlock_page()
- **Dirty Queue**: add_page_to_dirty_queue() for persistence
- **Index Operations**: Locker index functions for coin storage
- **Transaction Safety**: Proper locking and error handling

### 5.3 Network Layer Integration
Commands requiring external communication must use:
- **Socket Operations**: Standard socket API for RAIDA communication
- **Timeout Handling**: Appropriate timeout values for network operations
- **Error Recovery**: Graceful handling of network failures
- **Resource Cleanup**: Proper socket and connection cleanup

## 6. Security Requirements

### 6.1 Input Validation
All commands must validate:
- **Request size**: Minimum and expected size validation
- **Parameter bounds**: All numeric parameters within valid ranges
- **Buffer sizes**: Prevent buffer overflows and excessive allocations
- **Authentication**: Admin keys and authorization where required

### 6.2 Authentication and Authorization
Commands requiring elevated access must:
- **Admin Authentication**: Validate admin keys for privileged operations
- **Coin Ownership**: Verify coin ownership before modifications
- **Access Control**: Enforce appropriate access restrictions
- **Error Responses**: Use generic errors to prevent information disclosure

### 6.3 Resource Protection
All commands must implement:
- **Memory Limits**: Prevent excessive memory allocation
- **File System**: Sandbox enforcement for filesystem operations
- **Network Limits**: Reasonable timeouts and connection limits
- **Database Protection**: Proper locking and transaction handling

## 7. Error Handling Standards

### 7.1 Status Code Usage
Commands must use appropriate status codes:
- **SUCCESS**: STATUS_SUCCESS, STATUS_ALL_PASS, STATUS_ALL_FAIL, STATUS_MIXED
- **Validation Errors**: ERROR_INVALID_PACKET_LENGTH, ERROR_INVALID_PARAMETER
- **Authentication Errors**: ERROR_ADMIN_AUTH, ERROR_INVALID_ENCRYPTION
- **System Errors**: ERROR_MEMORY_ALLOC, ERROR_INTERNAL, ERROR_FILESYSTEM

### 7.2 Error Response Requirements
- **Status Setting**: Always set ci->command_status appropriately
- **Resource Cleanup**: Free all allocated resources on error paths
- **Logging**: Log errors with appropriate context and detail level
- **Information Security**: Avoid revealing sensitive information in errors

This specification provides the complete command interface definition needed to implement all CloudCoin RAIDA command handlers while remaining language-agnostic and accurately reflecting the comprehensive command system architecture.