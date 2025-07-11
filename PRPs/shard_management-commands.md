# Shard Management Commands Implementation (cmd_shards)

## Module Purpose
This module implements shard-related operations for managing coin transfers between different blockchain systems and coin creation/migration processes. It handles the complex logic of moving coins from legacy systems (CloudCoin, SuperCoin) to the current RAIDA system, including transaction rollback capabilities and coin pickup operations. The module operates within an on-demand page cache system for efficient coin data management.

## Core Functionality

### 1. Transaction Rollback (`cmd_rollback_switch_shard`)
**Parameters:**
- Connection information structure

**Returns:** None (modifies connection structure)

**Purpose:** Placeholder function for rolling back failed shard switch transactions. Currently returns not implemented status.

**Process:**
1. Logs operation start
2. Sets error status to not implemented
3. Logs operation completion

**Dependencies:**
- Logging system for operation tracking

**Status:** Stub implementation - future enhancement placeholder

### 2. Available Serial Numbers Query (`cmd_get_sns`)
**Parameters:**
- Connection information structure
- Input: 39-byte payload containing session ID, operation type, and denomination flags

**Returns:** None (modifies connection structure with available serial number ranges)

**Purpose:** Retrieves available coin serial numbers from the database for shard switch operations, organized by denomination and returned as ranges and individual numbers.

**Process:**
1. Validates exact payload size (39 bytes)
2. Extracts session ID and operation type (must be 3 or 4)
3. Processes denomination flags to determine which denominations to query
4. For each requested denomination:
   - Scans all database pages for available coins (MFS = 0)
   - Identifies consecutive ranges and individual coins
   - Limits results to maximum available coins per denomination
   - Formats response with range data and individual serial numbers
5. Returns compressed representation of available coins

**Dependencies:**
- Database layer for page access and coin status checking
- Utility functions for data formatting and extraction
- Memory management for response buffer allocation

**Output Format:**
- Per denomination: 1-byte denomination + 1-byte range count + 1-byte individual count + range data + individual serial numbers
- Range data: 8 bytes per range (4-byte start + 4-byte end)
- Individual data: 4 bytes per serial number

### 3. Shard Switch with Serial Numbers (`cmd_switch_shard_sum_with_sns`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 50 bytes) containing session data, shard information, and coin definitions

**Returns:** None (modifies connection structure with operation status)

**Purpose:** Executes complex shard migration by deleting coins from legacy systems and creating new coins in the current system, with value verification and test mode support.

**Process:**
1. Validates minimum payload size and extracts session parameters
2. Determines operation mode (test vs. actual) based on session ID
3. Validates target shard ID and extracts legacy coin data
4. Calculates total value of legacy coins being deleted
5. Calculates total value of new coins being created
6. Verifies value conservation between legacy and new systems
7. For CloudCoin shard:
   - Validates total value matches expected conversion
   - Performs detection or deletion of legacy coins
8. For SuperCoin shard:
   - Calculates fixed conversion rate (85.125 per coin)
   - Performs detection or deletion operations
9. Creates new coins in current system:
   - Updates authentication numbers using provided data
   - Sets appropriate MFS (Months From Start) values
   - Updates statistics for coin creation
10. Marks operation as successful

**Dependencies:**
- Legacy coin management systems (CloudCoin, SuperCoin)
- Database layer for new coin creation
- Value calculation utilities
- Statistics system for tracking coin operations
- Cryptographic utilities for authentication number generation

### 4. Shard Switch (Legacy) (`cmd_switch_shard_sum`)
**Parameters:**
- Connection information structure

**Returns:** None (modifies connection structure)

**Purpose:** Legacy shard switch implementation. Currently returns not implemented status.

**Process:**
1. Logs operation start
2. Sets error status to not implemented
3. Logs operation completion

**Dependencies:**
- Logging system for operation tracking

**Status:** Stub implementation - replaced by `cmd_switch_shard_sum_with_sns`

### 5. Coin Pickup Operation (`cmd_pickup_coins`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 43 bytes) containing session ID, authentication data, and coin list

**Returns:** None (modifies connection structure with operation status)

**Purpose:** Takes ownership of coins that have been moved from another shard by creating them in the current system with proper authentication.

**Process:**
1. Validates minimum payload size and extracts session ID
2. Validates coin data alignment (5 bytes per coin)
3. For each coin in the list:
   - Extracts denomination and serial number
   - Retrieves corresponding database page with locking
   - Verifies page is reserved by the provided session ID
   - Generates authentication number using MD5 hash of session data
   - Creates coin record with new authentication number
   - Updates statistics for coin creation
   - Releases page lock
4. Marks operation as successful

**Dependencies:**
- Database layer for page reservation system and coin creation
- Cryptographic utilities for MD5 hash generation
- Statistics system for tracking coin operations
- Page locking mechanism for concurrency control

**Security Features:**
- Session-based page reservation prevents unauthorized coin creation
- Cryptographic authentication number generation ensures unique coin identity
- Atomic operation with rollback on validation failures

## Data Structures and Constants

### Input Data Formats
- **Session Data:** 32-bit session ID for operation tracking
- **Shard Information:** 8-bit shard ID, operation parameters
- **Coin Records:** 5 bytes per coin (1-byte denomination + 4-byte serial number)
- **Legacy Coin Data:** Variable format depending on source shard type
- **Authentication Data:** 16-byte session authentication information

### Output Data Formats
- **Serial Number Ranges:** Compressed format with range counts and boundaries
- **Operation Status:** Success/failure indicators with error details
- **Coin Creation Results:** Updated statistics and confirmation data

### Shard Types
- **SHARD_CLOUDCOIN (1):** Legacy CloudCoin system
- **SHARD_SUPERCOIN (2):** Legacy SuperCoin system  
- **SHARD_NEW (3):** Current RAIDA system

### conn_info_t (Connection Structure)
Fields:
  - bodySize: Integer
  - commandStatus: Enum(StatusCode)
  - output: BinaryBuffer or String (based on command)
  - outputSize: Integer

### Structure: Page
Description: A cacheable unit containing coin records and metadata for concurrency and persistence management.

Fields:
  - number: Integer
    Description: Unique page identifier (e.g., page index in database).
  
  - data: List of CoinRecord
    Description: A fixed-size list of coin records. Each coin record is 17 bytes in serialized binary form.

  - reservedBy: Integer (32-bit)
    Description: Session ID of the client that currently holds a reservation on this page. Used to control exclusive access.

  - isDirty: Boolean
    Description: Indicates whether this page has been modified and needs to be flushed (persisted to disk).



### Error Conditions
- `ERROR_INVALID_PACKET_LENGTH`: Incorrect payload size
- `ERROR_INVALID_PARAMETER`: Invalid operation type or shard ID
- `ERROR_INVALID_SHARD`: Unsupported shard identifier
- `ERROR_INVALID_SHARD_ID`: Shard ID out of valid range
- `ERROR_AMOUNT_MISMATCH`: Value conservation violation
- `ERROR_BAD_COINS`: Legacy coin validation failure
- `ERROR_PAGE_IS_NOT_RESERVED`: Session reservation mismatch
- `ERROR_COINS_NOT_DIV`: Coin data alignment error

### Constants
- `MAX_AVAILABLE_COINS`: 1024 coins maximum per denomination query
- `MAX_SHARD`: Maximum valid shard identifier
- Conversion rates for legacy systems

## Value Conservation and Validation

### CloudCoin Conversion
- Direct value mapping from legacy CloudCoin system
- Whole value validation ensures no fractional coin creation
- Detection and deletion operations with full verification

### SuperCoin Conversion  
- Fixed conversion rate: 85.125 units per SuperCoin
- Bulk conversion with value conservation checks
- Simplified coin format handling

### New Coin Creation
- Value-based coin generation in target denominations
- Authentication number generation using session-specific data
- Statistics tracking for audit and monitoring

## Session Management and Security

### Page Reservation System
- Session-based page locking prevents concurrent access conflicts
- Automatic reservation validation during coin pickup
- Session timeout handling for abandoned operations

### Authentication and Integrity
- MD5-based authentication number generation
- MD5(input, output): Generates a 16-byte authentication number (AN) using the session ID as input
- Used during coin creation to assign unique, reproducible ANs
- Session-specific cryptographic parameters
- Value conservation verification across shard boundaries

### Test Mode Operations
- Non-destructive validation mode (session ID = 0)
- Full operation simulation without permanent changes
- Comprehensive error checking and reporting

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache with reservation system
- **Legacy Coin Systems:** CloudCoin and SuperCoin management interfaces
- **Cryptographic Utilities:** MD5 hashing, secure random generation
- **Statistics System:** Coin operation tracking and reporting
- **Utilities Module:** Value calculation, data extraction, endianness handling
- **Configuration System:** Server parameters and validation rules

### External Constants Required
- `RECORDS_PER_PAGE`: Database page organization
- `ERROR_*`: Protocol error definitions
- `STATUS_SUCCESS`: Success indicator
- Denomination and value calculation constants

### Used By
- **Shard Migration Tools:** Automated coin transfer systems
- **Legacy System Interfaces:** CloudCoin and SuperCoin bridges
- **Administrative Tools:** Bulk coin operations and management

## Performance Considerations

### Database Efficiency
- On-demand page loading minimizes memory usage
- Page-level locking reduces contention
- Batch operations for improved throughput

### Memory Management
- Dynamic allocation for variable-sized responses
- Efficient range compression for serial number data
- Proper cleanup on operation completion or failure

### Legacy System Integration
- Optimized detection and deletion operations
- Bulk processing for improved performance
- Error recovery and rollback capabilities

## Threading and Concurrency
- Operations execute within thread pool context
- Page reservation system prevents race conditions
- Atomic database updates ensure consistency
- Session-based locking provides operation isolation

This module enables seamless migration of coins between different blockchain systems while maintaining value conservation, security, and operational integrity throughout the transfer process.