# Shard Command Handlers (cmd_shards)

## Module Purpose
This module implements shard-based coin management commands for the RAIDA network, enabling secure migration of coins between different shards (CloudCoin, SuperCoin, and the current RAIDA system). It provides comprehensive coin switching operations, value verification, dual hashing support, and integrates with legacy systems while maintaining monetary conservation across all operations.

## Constants and Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_AVAILABLE_COINS` | 1024 | Maximum number of coins returned per denomination in availability queries |
| `SHARD_CLOUDCOIN` | 1 | Legacy CloudCoin v1 shard identifier |
| `SHARD_SUPERCOIN` | 2 | CloudCoin v2 (SuperCoin) shard identifier |
| `MAX_SHARD` | Variable | Maximum valid shard identifier |

## Error Codes
| Constant | Description |
|----------|-------------|
| `ERROR_INVALID_PACKET_LENGTH` | Request payload size is incorrect for the operation |
| `ERROR_INVALID_PARAMETER` | Invalid operation type or parameter provided |
| `ERROR_INVALID_SHARD` | Invalid shard identifier specified |
| `ERROR_INVALID_SHARD_ID` | Shard ID is not supported or recognized |
| `ERROR_INVALID_SN_OR_DENOMINATION` | Invalid coin serial number or denomination |
| `ERROR_PAGE_IS_NOT_RESERVED` | Required page is not reserved by the requesting session |
| `ERROR_AMOUNT_MISMATCH` | Total value of input coins does not match output coins |
| `ERROR_BAD_COINS` | Input coins failed validation in legacy systems |
| `ERROR_NOT_IMPLEMENTED` | Operation is not yet implemented |
| `ERROR_COINS_NOT_DIV` | Coin data size not properly divisible by record size |

## Status Codes
| Constant | Description |
|----------|-------------|
| `STATUS_SUCCESS` | Operation completed successfully |

## Core Functionality

### 1. Rollback Switch Shard (`cmd_rollback_switch_shard`)
**Parameters:**
- Connection information structure

**Returns:** None (returns not implemented status)

**Purpose:** Placeholder for rollback functionality to reverse failed shard switching operations. Currently not implemented but reserved for future transaction rollback capabilities.

**Process:**
1. **Status Return:**
   - Returns ERROR_NOT_IMPLEMENTED status
   - Placeholder for future rollback functionality
   - Maintains API compatibility for future implementation

**Used By:** Future transaction rollback systems

### 2. Get Serial Numbers (`cmd_get_sns`)
**Parameters:**
- Connection information structure containing request data
- Input: 39-byte payload (session ID + operation type + denomination selection)

**Returns:** None (modifies connection structure with available serial numbers and ranges)

**Purpose:** Retrieves available coin serial numbers for shard switching operations, providing optimized range and individual serial number information for efficient bulk operations.

**Process:**
1. **Request Validation:**
   - Validates exact 39-byte payload size
   - Extracts session ID and operation type
   - Validates operation type (must be 3 or 4)
   - Extracts denomination selection bitmap

2. **Denomination Processing:**
   - Iterates through requested denominations from bitmap
   - For each denomination:
     - Scans all pages using on-demand cache
     - Identifies available coins (MFS = 0)
     - Groups contiguous serial numbers into ranges
     - Collects individual serial numbers

3. **Range Optimization:**
   - Converts contiguous available coins into start/end ranges
   - Separates individual serial numbers from ranges
   - Limits total results to MAX_AVAILABLE_COINS per denomination
   - Optimizes response size through range compression

4. **Response Generation:**
   - For each denomination with available coins:
     - Denomination identifier (1 byte)
     - Number of ranges (1 byte)  
     - Number of individual serial numbers (1 byte)
     - Range data (start/end pairs, 8 bytes each)
     - Individual serial numbers (4 bytes each)

**Used By:** Shard preparation workflows, bulk coin allocation

**Dependencies:** Database layer for page access

### 3. Switch Shard Sum with Serial Numbers (`cmd_switch_shard_sum_with_sns`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 50 bytes) with shard data, coin lists, and authentication

**Returns:** None (modifies connection structure with operation result)

**Purpose:** Performs atomic shard switching by destroying coins in legacy shards and creating equivalent value coins in the current RAIDA system with complete value verification and dual hashing support.

**Process:**
1. **Request Validation:**
   - Validates minimum 50-byte payload size
   - Extracts session ID, target shard ID, and coin data
   - Validates shard ID within acceptable range (≤ MAX_SHARD)

2. **Payload Parsing:**
   - Extracts legacy coin count and data
   - Parses new coin specifications
   - Extracts PANG (authentication data) for new coins
   - Validates data structure alignment

3. **Value Verification:**
   - Calculates total value of legacy coins being destroyed
   - Calculates total value of new coins being created
   - Ensures value conservation (input value = output value)
   - Rejects operation if values don't match

4. **Legacy Shard Processing:**
   - **SHARD_CLOUDCOIN:** 
     - Uses legacy calculation for total value
     - Calls legacy_detect for test mode or legacy_delete for execution
   - **SHARD_SUPERCOIN:**
     - Uses fixed SuperCoin value calculation (85.125 per coin)
     - Calls cc2_detect for test mode or cc2_delete for execution

5. **New Coin Creation with Dual Hash Support:**
   - For each new coin specification:
     - Retrieves target page using on-demand cache
     - Validates page is available for new coin creation
     - Constructs hash input from PANG authentication data
     - **Legacy Clients (encryption_type < 4):** Uses MD5 hash for authentication number generation
     - **Modern Clients (encryption_type >= 4):** Uses SHA-256 hash for authentication number generation
     - Sets MFS timestamp and marks page as dirty
     - Updates statistics for ownership and value tracking

**Security Features:**
- Value conservation enforced across shard boundaries
- Legacy system integration for secure coin destruction
- Dual hashing maintains compatibility with all client versions
- Test mode allows validation without permanent changes

**Used By:** Cross-shard migration tools

### 4. Switch Shard Sum (`cmd_switch_shard_sum`)
**Parameters:**
- Connection information structure

**Returns:** None (returns not implemented status)

**Purpose:** Placeholder for alternative shard switching implementation. Currently not implemented but reserved for future shard switching variants.

**Process:**
1. **Status Return:**
   - Returns ERROR_NOT_IMPLEMENTED status
   - Placeholder for future implementation
   - Maintains API compatibility

**Used By:** Future shard switching systems

### 5. Pickup Coins (`cmd_pickup_coins`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 43 bytes) with session ID and coin specifications

**Returns:** None (modifies connection structure with operation result)

**Purpose:** Takes ownership of coins that have been moved from another shard, completing the shard transfer process by creating coins in the current system with proper authentication.

**Process:**
1. **Request Validation:**
   - Validates minimum 43-byte payload size
   - Calculates coin count from payload (5 bytes per coin)
   - Validates coin data alignment

2. **Session Validation:**
   - Extracts session ID from request
   - Validates session for coin pickup authorization

3. **Coin Creation with Dual Hash Support:**
   - For each coin in the pickup request:
     - Extracts denomination and serial number
     - Retrieves target page using on-demand cache
     - Validates page is reserved by the requesting session
     - Constructs hash input from session authentication data
     - **Legacy Clients (encryption_type < 4):** Uses MD5 hash for authentication number generation
     - **Modern Clients (encryption_type >= 4):** Uses SHA-256 hash for authentication number generation
     - Sets MFS timestamp for new coin
     - Marks page as dirty for persistence
     - Updates statistics for ownership and value tracking

4. **Statistics Update:**
   - Increments POWN (ownership) counter
   - Adds coin value to total value statistics
   - Tracks successful shard transfers

**Security Features:**
- Session-based authorization prevents unauthorized pickup
- Page reservation ensures exclusive access
- Dual hashing maintains client compatibility
- Statistics tracking provides audit trail

**Used By:** Shard transfer completion workflows, coin migration systems

## Data Structures and Formats

### Request Formats
| Operation | Minimum Size | Structure |
|-----------|-------------|-----------|
| Get SNs | 39 bytes | Session ID (4) + Operation Type (1) + Denomination Bitmap (16) + Reserved (18) |
| Switch Shard Sum | 50 bytes | Session ID (4) + Shard ID (1) + Signature (16) + Coin Counts + Coin Data + PANG + New Coins |
| Pickup Coins | 43 bytes | Session ID (4) + Authentication (16) + Coin Specs (5 × N) + Reserved (18) |

### Coin Data Formats
| Format | Size | Structure |
|--------|------|-----------|
| Coin Specification | 5 bytes | Denomination (1) + Serial Number (4) |
| Legacy Coin Data | Variable | Shard-specific format for legacy coins |

### Response Formats
| Operation | Response Format |
|-----------|----------------|
| Get SNs | Per denomination: Den (1) + Range Count (1) + Individual Count (1) + Range Data + Individual SNs |
| Switch/Pickup | Status code only |

### Value Calculation Structures
| Shard Type | Value Calculation |
|------------|------------------|
| CloudCoin | Legacy calculation from coin data |
| SuperCoin | Fixed value: 85.125 per coin |
| Current RAIDA | Denomination-based value calculation |

## Multi-Shard Integration

### Legacy System Support
- **CloudCoin v1:** Full integration with legacy detection and deletion systems
- **SuperCoin:** Support for CloudCoin v2 protocols and value calculations
- **Value Mapping:** Accurate value conversion between shard systems

### Shard-Specific Operations
- **Detection Phase:** Validates coins exist in source shard before transfer
- **Deletion Phase:** Securely removes coins from source shard
- **Creation Phase:** Creates equivalent value coins in target shard

## Security Considerations

### Value Conservation
- **Atomic Operations:** All transfers succeed completely or fail entirely
- **Value Verification:** Input and output values must match exactly
- **Cross-Shard Validation:** Legacy system integration ensures coin authenticity

### Authentication and Authorization
- **Session Management:** Reservation system prevents concurrent conflicts
- **Dual Hash Security:** Backward compatible cryptographic security
- **Legacy Integration:** Secure integration with existing shard systems

### Audit and Compliance
- **Statistics Tracking:** All successful transfers recorded in statistics
- **Operation Logging:** Complete audit trail for shard operations
- **Error Tracking:** Failed operations logged for analysis

## Error Handling and Validation

### Input Validation
- **Size Validation:** Payload sizes validated for each operation type
- **Shard Validation:** Shard IDs validated against supported values
- **Value Validation:** Input and output coin values must match exactly
- **Coin Validation:** All coin specifications validated for proper format

### Runtime Validation
- **Session Validation:** Sessions validated for proper authorization
- **Page Validation:** Target pages validated for availability and reservation
- **Legacy Validation:** Legacy system coins validated before destruction

### Error Recovery
- **Resource Cleanup:** Page locks and memory freed on error conditions
- **State Consistency:** Failed operations leave all shards unchanged
- **Transaction Integrity:** Atomic operations ensure no partial state

## Performance Characteristics

### Bulk Operation Optimization
- **Range Processing:** Efficient handling of contiguous serial number ranges
- **Batch Validation:** Legacy systems support batch coin validation
- **Memory Efficiency:** Optimized data structures for large coin collections

### Cross-Shard Efficiency
- **Single-Pass Operations:** Minimize legacy system calls
- **Value Caching:** Efficient value calculations for different shard types
- **Statistics Batching:** Efficient statistics updates for bulk operations

### Database Integration
- **On-Demand Cache:** Efficient use of database layer caching
- **Page Reservation:** Prevents conflicts during multi-step operations
- **Dirty Page Management:** Optimized persistence for bulk changes

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache and reservation system
- **Legacy Systems:** CloudCoin v1 and SuperCoin detection/deletion interfaces
- **Utilities Module:** Dual hash functions and value calculations
- **Statistics System:** Operation counting and value tracking
- **Configuration Module:** Shard limits and server identification

### External Constants Required
- `SHARD_*`: Shard type identifiers
- `MAX_SHARD`: Maximum valid shard identifier
- `RECORDS_PER_PAGE`: Database page organization
- `POWN_FIELD_IDX`, `POWN_VALUE_FIELD_IDX`: Statistics field identifiers

### Legacy System Dependencies
- **CloudCoin v1 Interface:** Functions for legacy coin detection and deletion
- **SuperCoin Interface:** Functions for CloudCoin v2 operations
- **Value Calculation:** Legacy-specific value calculation methods

### Used By
- **Migration Tools:** Primary interface for cross-shard coin migration
- **Administrative Tools:** Bulk shard management and maintenance
- **Trading Systems:** Cross-shard value transfers

### Cross-File Dependencies
- **Database Layer:** Page access, locking, and persistence
- **Legacy Systems:** CloudCoin and SuperCoin integration modules
- **Utilities Module:** Hash generation and value calculation functions
- **Statistics Module:** Operation tracking and audit trail
- **Configuration Module:** Shard configuration and limits

## Threading and Concurrency

### Session-Based Isolation
- **Reservation System:** Each session operates on reserved pages
- **Atomic Operations:** Individual shard operations are atomic
- **Concurrent Sessions:** Multiple shard operations can proceed simultaneously
- **Resource Safety:** Proper cleanup ensures thread safety

### Legacy System Integration
- **Synchronous Operations:** Legacy system calls are synchronous
- **Thread Safety:** Legacy interfaces accessed in thread-safe manner
- **Resource Management:** Proper cleanup of legacy system resources

## Backward Compatibility

### Dual Hash Support
- **Algorithm Selection:** Automatic selection based on client encryption type
- **Legacy MD5:** Full support for existing client implementations
- **Modern SHA-256:** Enhanced security for newer clients
- **Transparent Operation:** Hash selection transparent to calling code

### Legacy System Compatibility
- **CloudCoin v1:** Full backward compatibility with original system
- **SuperCoin:** Complete support for CloudCoin v2 protocols
- **Value Mapping:** Accurate conversion between legacy and current value systems

## Value Conservation and Verification

### Multi-Shard Value Calculations
- **CloudCoin:** Uses legacy system's native value calculation
- **SuperCoin:** Fixed value per coin (85.125)
- **Current RAIDA:** Denomination-based value calculation
- **Precision Handling:** Proper handling of fractional values across systems

### Conservation Enforcement
- **Pre-Operation Validation:** Values calculated before any modifications
- **Atomic Verification:** Value match required before proceeding
- **Error on Mismatch:** Operations fail if values don't match exactly
- **Audit Trail:** Value calculations logged for verification

## Test Mode Support

### Non-Destructive Testing
- **Session ID Zero:** Special session ID enables test mode
- **Validation Only:** Test mode validates without making changes
- **Legacy Testing:** Legacy systems support test-only operations
- **Result Verification:** Test results provided without side effects

### Production Safeguards
- **Test Detection:** Automatic detection of test vs. production operations
- **State Preservation:** Test operations leave all state unchanged
- **Error Simulation:** Test mode can simulate various error conditions

## Migration and Integration Support

### Gradual Migration
- **Shard Coexistence:** Multiple shard types can operate simultaneously
- **Progressive Transfer:** Supports gradual migration between shards
- **Rollback Preparation:** Infrastructure for future rollback capabilities

### Administrative Support
- **Bulk Operations:** Efficient handling of large-scale migrations
- **Progress Tracking:** Statistics provide migration progress visibility
- **Error Recovery:** Comprehensive error handling for failed migrations

This shard command module provides essential functionality for  migration within the RAIDA network, enabling secure value transfers between different cryptocurrency systems while maintaining value conservation, backward compatibility, and comprehensive audit capabilities.