# Shard Command Handlers (cmd_shards.c)

## Module Purpose
This module implements shard management operations for the RAIDA network, enabling seamless transitions between different coin systems (CloudCoin v1, CloudCoin v2/SuperCoin, and current RAIDA system). It provides functionality for moving coins between shards, converting legacy coins to current system, and managing cross-shard operations while maintaining value conservation and cryptographic security.

## Core Functionality

### 1. Shard Switch Rollback (`cmd_rollback_switch_shard`)
**Parameters:**
- Connection information structure containing request data

**Returns:** None (modifies connection structure with error status)

**Purpose:** Provides rollback capability for failed shard switching operations, allowing clients to recover from incomplete transactions.

**Process:**
1. **Stub Implementation:**
   - Currently returns NOT_IMPLEMENTED error
   - Reserved for future rollback functionality
   - Placeholder for transaction rollback logic

**Input Format:**
- Not yet implemented

**Output Format:**
- Error status indicating not implemented


### 2. Available Serial Numbers for Shards (`cmd_get_sns`)
**Parameters:**
- Connection information structure containing request data
- Input: 39-byte payload with session ID, operation type, and denomination requests

**Returns:** None (modifies connection structure with available serial numbers and ranges)

**Purpose:** Retrieves available serial numbers for shard operations, providing both individual serial numbers and contiguous ranges for efficient coin allocation during shard transitions.

**Process:**
1. **Request Validation:**
   - Validates exact payload size (39 bytes)
   - Extracts session ID (4 bytes) and operation type (1 byte)
   - Validates operation type (must be 3 or 4 for shard operations)
   - Extracts denomination request bitmap (16 bytes)

2. **Denomination Processing:**
   - Iterates through all possible denominations based on request bitmap
   - For each requested denomination, scans all pages for available coins
   - Processes pages sequentially without reservation (different from executive commands)
   - Identifies available coin slots based on MFS values

3. **Range Optimization:**
   - Identifies contiguous ranges of available serial numbers
   - Separates individual serial numbers from ranges
   - Optimizes response format for efficient client processing
   - Limits total coins returned to prevent oversized responses

4. **Response Generation:**
   - For each denomination with available coins:
     - Includes denomination identifier
     - Lists range count and individual serial number count
     - Provides range start/end pairs
     - Includes individual serial numbers

**Shard-Specific Features:**
- No page reservation required for shard operations
- Support for multiple denomination selection
- Optimized for large-scale shard transitions
- Efficient range-based response format

**Input Format:**
- 4 bytes: Session ID
- 1 byte: Operation type (3 or 4)
- 16 bytes: Denomination request bitmap
- 18 bytes: Reserved/padding

**Output Format:**
Per denomination:
- 1 byte: Denomination identifier
- 1 byte: Range count
- 1 byte: Individual serial number count
- Variable: Range pairs (8 bytes each: 4 bytes start + 4 bytes end)
- Variable: Individual serial numbers (4 bytes each)

**Dependencies:**
- Database layer for on-demand page access
- Range optimization algorithms

### 3. Shard Switching with Serial Numbers (`cmd_switch_shard_sum_with_sns`)
**Parameters:**
- Connection information structure containing request data
- Input: Variable-length payload (minimum 50 bytes) with coin conversion data

**Returns:** None (modifies connection structure with conversion results)

**Purpose:** Converts coins from legacy shard systems to current RAIDA system, ensuring value conservation and proper authentication number generation.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size (50 bytes)
   - Extracts session ID, shard ID, and coin counts
   - Validates shard ID within acceptable range
   - Extracts shard methodology and coin lists

2. **Value Conservation Verification:**
   - Calculates total value of coins to be deleted from legacy shard
   - Calculates total value of coins to be created in current system
   - Ensures value conservation across shard transition
   - Validates conversion ratios based on shard type

3. **Legacy Coin Processing:**
   - **CloudCoin v1 (SHARD_CLOUDCOIN):**
     - Uses legacy_calc_total to calculate total value
     - Validates 1:1 value conversion ratio
     - Uses legacy_detect for test mode or legacy_delete for execution
   - **SuperCoin (SHARD_SUPERCOIN):**
     - Uses fixed conversion rate (85.125 per coin)
     - Validates conversion ratio against total value
     - Uses cc2_detect for test mode or cc2_delete for execution

4. **New Coin Creation:**
   - For each new coin in current system:
     - Retrieves coin page using denomination and serial number
     - Generates new authentication number using cryptographic hash
     - Updates coin record with new authentication number
     - Sets MFS timestamp to current month
     - Marks database page as dirty for persistence
     - Updates statistics for successful conversions

5. **Test Mode Support:**
   - Session ID of 0 enables test mode
   - Test mode validates conversion without making changes
   - Allows verification of conversion parameters
   - Provides safe validation of large operations

**Value Conservation:**
- Strict value matching between legacy and current systems
- Prevents value inflation or deflation during conversion
- Validates conversion ratios for each shard type
- Ensures mathematical accuracy of conversions

**Input Format:**
- 4 bytes: Session ID (0 for test mode)
- 1 byte: Source shard ID
- 1 byte: Reserved
- 16 bytes: Shard methodology data
- 2 bytes: Legacy coin count
- Variable: Legacy coin list (format depends on shard type)
- 16 bytes: PAN (Proposed Authentication Number) guidance
- 2 bytes: New coin count
- Variable: New coin list (5 bytes each: 1 byte denomination + 4 bytes serial)

**Output Format:**
- Status code indicating success or specific failure reason

### 4. Shard Switching Sum (`cmd_switch_shard_sum`)
**Parameters:**
- Connection information structure containing request data

**Returns:** None (modifies connection structure with error status)

**Purpose:** Provides alternative shard switching interface, currently not implemented.

**Process:**
1. **Stub Implementation:**
   - Currently returns NOT_IMPLEMENTED error
   - Reserved for future shard switching functionality
   - Placeholder for alternative conversion logic

**Input Format:**
- Not yet implemented

**Output Format:**
- Error status indicating not implemented

**Dependencies:**
- None (stub function)

### 5. Coin Pickup Operation (`cmd_pickup_coins`)
**Parameters:**
- Connection information structure containing request data
- Input: Variable-length payload (minimum 43 bytes) with coin pickup data

**Returns:** None (modifies connection structure with pickup results)

**Purpose:** Takes ownership of coins that have been moved from another shard, completing the shard transition process.

**Process:**
1. **Request Validation:**
   - Validates minimum payload size (43 bytes)
   - Extracts session ID and validates coin list alignment
   - Calculates coin count from payload size (5 bytes per coin)

2. **Coin Ownership Transfer:**
   - For each coin in the pickup list:
     - Retrieves coin page using denomination and serial number
     - Verifies page is reserved by requesting session
     - Generates new authentication number using cryptographic hash
     - Updates coin record with new authentication number
     - Sets MFS timestamp to current month
     - Marks database page as dirty for persistence

3. **Statistics Update:**
   - Updates coin count statistics for successful pickups
   - Updates value statistics based on coin denomination
   - Tracks successful shard transition operations

4. **Session Management:**
   - Validates session ID matches page reservation
   - Ensures exclusive access to coin slots
   - Prevents concurrent access conflicts

**Pickup Features:**
- Session-based access control
- Cryptographically secure authentication number generation
- Statistics tracking for monitoring
- Atomic operations per coin

**Input Format:**
- 4 bytes: Session ID
- 16 bytes: Authentication guidance data
- Variable: Coin list (5 bytes each: 1 byte denomination + 4 bytes serial number)
- 2 bytes: End-of-frame marker

**Output Format:**
- Status code indicating success or failure reason

## Data Structures and Formats

### Shard Identifiers
- **SHARD_CLOUDCOIN (1):** CloudCoin v1 legacy system
- **SHARD_SUPERCOIN (2):** CloudCoin v2/SuperCoin system
- **SHARD_NEW (3):** Current RAIDA system
- **SHARD_UNKNOWN (0):** Unknown or invalid shard

### Coin Record Formats
- **Shard Query Record:** Session ID + operation type + denomination bitmap
- **Legacy Coin Record:** Format depends on source shard type
- **Current Coin Record:** 1 byte denomination + 4 bytes serial number
- **Pickup Record:** Session ID + guidance + coin list

### Conversion Ratios
- **CloudCoin v1:** 1:1 value conversion (whole value matching)
- **SuperCoin:** 85.125 value per coin conversion
- **Current System:** Fractional value support with 8 decimal places

### Response Formats
- **Availability Response:** Denomination + range count + individual count + data
- **Conversion Response:** Status code indicating success or failure reason
- **Pickup Response:** Status code indicating success or failure reason

## Security Considerations

### Value Conservation
- **Strict Validation:** Total value must match exactly between shards
- **Conversion Ratios:** Predefined ratios prevent value manipulation
- **Mathematical Accuracy:** Precise calculations for value conservation
- **Audit Trail:** All conversions tracked through statistics

### Authentication Security
- **Cryptographic Generation:** Secure authentication number generation
- **Session Management:** Session-based access control
- **Page Reservation:** Prevents concurrent access conflicts
- **Test Mode:** Safe validation without permanent changes

### Data Integrity
- **Atomic Operations:** All-or-nothing conversion operations
- **Page Locking:** Database page locking ensures consistency
- **MFS Timestamps:** All changes timestamped for audit trail
- **Dirty Page Tracking:** Modified data marked for persistence

## Error Handling and Validation

### Input Validation
- **Size Validation:** All payloads validated for expected length
- **Parameter Validation:** All parameters within acceptable ranges
- **Shard Validation:** Shard IDs must be valid and supported
- **Session Validation:** Pages must be reserved by requesting session

### Error Conditions
- `ERROR_INVALID_PACKET_LENGTH`: Incorrect payload size
- `ERROR_INVALID_PARAMETER`: Invalid operation type
- `ERROR_INVALID_SHARD`: Invalid shard identifier
- `ERROR_INVALID_SHARD_ID`: Shard ID out of range
- `ERROR_AMOUNT_MISMATCH`: Value conservation violation
- `ERROR_BAD_COINS`: Legacy coin validation failure
- `ERROR_PAGE_IS_NOT_RESERVED`: Page not reserved by session
- `ERROR_COINS_NOT_DIV`: Coin data alignment error
- `ERROR_NOT_IMPLEMENTED`: Feature not yet implemented

### Recovery Mechanisms
- **Test Mode:** Validate operations without permanent changes
- **Rollback Support:** Framework for transaction rollback
- **State Consistency:** Failed operations leave state unchanged
- **Resource Cleanup:** Proper cleanup on error conditions

## Performance Characteristics

### Batch Processing
- **Large-Scale Operations:** Support for bulk coin conversions
- **Efficient Value Calculation:** Optimized value computation
- **Range-Based Responses:** Efficient representation of available coins
- **Parallel Processing:** Multiple coins processed efficiently

### Memory Management
- **Dynamic Allocation:** Response buffers allocated as needed
- **Resource Cleanup:** Proper cleanup on all code paths
- **Cache Integration:** Efficient page access through on-demand cache
- **Statistics Integration:** Efficient tracking of operations

### Network Efficiency
- **Batch Operations:** Multiple coins in single request
- **Compressed Responses:** Range-based format reduces data transfer
- **Test Mode:** Validate before execution to prevent errors
- **Session Management:** Efficient session-based access control

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache for coin data access
- **Legacy Systems:** CloudCoin v1 and SuperCoin interfaces
- **Statistics System:** Operation counting and value tracking
- **Cryptographic Utilities:** Secure authentication number generation
- **Configuration System:** Shard configuration and validation

### External Constants Required
- `MAX_AVAILABLE_COINS`: Maximum coins returned in availability query
- `SHARD_*`: Shard identification constants
- `POWN_FIELD_IDX`, `POWN_VALUE_FIELD_IDX`: Statistics field indices
- `STATUS_*`: Operation result status codes
- `ERROR_*`: Error condition definitions

### Used By
- **Client Applications:** Primary interface for shard transitions
- **Migration Tools:** Bulk migration from legacy systems
- **Administrative Tools:** Shard management and monitoring
- **Trading Systems:** Cross-shard value transfers

## Threading and Concurrency
- **Page Locking:** Thread-safe access to coin data
- **Session Management:** Session-based concurrency control
- **Atomic Operations:** Individual operations are atomic
- **Resource Safety:** Proper cleanup ensures no resource leaks

## Legacy System Integration
- **CloudCoin v1:** Complete integration with legacy detection and deletion
- **SuperCoin:** Support for CloudCoin v2 protocols
- **Value Conversion:** Accurate conversion between systems
- **Migration Support:** Smooth transition from legacy to current

## Mathematical Properties
- **Value Conservation:** Total value preserved across conversions
- **Conversion Ratios:** Fixed ratios for each shard type
- **Precision Handling:** Proper handling of fractional values
- **Overflow Protection:** Prevents arithmetic overflow in calculations

This shard command module provides essential cross-system compatibility for the RAIDA network, enabling seamless transitions between different coin systems while maintaining value conservation and cryptographic security.