# Shard Command Handlers (cmd_shards)

## Module Purpose
This module implements shard management commands for cross-shard coin migration and conversion operations. It provides functionality for moving coins between different blockchain shards (CloudCoin v1, CloudCoin v2, and current RAIDAX system) with dual hashing support and free pages bitmap integration for seamless cross-shard operations.

## Constants and Configuration
| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_AVAILABLE_COINS` | 1024 | Maximum number of coins returned per denomination in shard operations |
| `SHARD_CLOUDCOIN` | Variable | CloudCoin v1 shard identifier |
| `SHARD_SUPERCOIN` | Variable | CloudCoin v2/SuperCoin shard identifier |
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

### 1. Rollback Switch Shard Command (`cmd_rollback_switch_shard`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status)

**Purpose:** Placeholder for rollback functionality in failed shard switching operations (currently unimplemented).

**Process:**
1. **Stub Implementation:**
   - Logs command execution for debugging
   - Returns ERROR_NOT_IMPLEMENTED status
   - **Future Extension Point:** Ready for rollback logic implementation

2. **Design Intent:**
   - **Transaction Safety:** Intended for rolling back failed shard switches
   - **Data Integrity:** Would restore coins to previous shard state
   - **Error Recovery:** Provides mechanism for operation recovery

**Used By:** Shard switching error recovery (when implemented)

**Dependencies:** None (stub implementation)

### 2. Get SNs Command (`cmd_get_sns`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status and output)

**Purpose:** Retrieves available serial numbers for shard switching operations with denomination filtering and session-based reservation.

**Process:**
1. **Request Validation:**
   - Validates exact request size (39 bytes)
   - Extracts session ID, operation type, and denomination mask
   - Validates operation type (3 or 4 only)

2. **Operation Type Validation:**
   - **Operation 3/4:** Specific shard switching operations
   - **Parameter Validation:** Ensures valid operation parameters
   - **Request Integrity:** Validates all request components

3. **Denomination Processing:**
   - **Selective Processing:** Uses 16-byte denomination mask for filtering
   - **Index Calculation:** Converts denominations to array indexes
   - **Efficient Filtering:** Processes only requested denominations

4. **Page Analysis with Fixed Logic:**
   - For each requested denomination:
     - **Page Iteration:** Scans all pages systematically
     - **Range Detection:** Identifies contiguous ranges of available coins
     - **Individual Coins:** Tracks isolated available coins
     - **Fixed Algorithm:** Corrected loop logic handles page boundaries properly

5. **Available Coin Detection:**
   - **Coin Status Check:** Examines MFS byte for each coin (0 = available)
   - **Range Optimization:** Groups consecutive available coins into ranges
   - **Efficiency:** Minimizes response size through range encoding
   - **Limit Enforcement:** Respects MAX_AVAILABLE_COINS limit

6. **Response Construction:**
   - For each denomination with available coins:
     - Denomination identifier (1 byte)
     - Number of ranges (1 byte)  
     - Number of individual coins (1 byte)
     - Range data: start_sn, end_sn pairs (8 bytes each)
     - Individual coin serial numbers (4 bytes each)

**Shard Features:**
- **Cross-Shard Preparation:** Identifies coins available for shard migration
- **Session Integration:** Supports session-based operations
- **Efficient Encoding:** Optimized response format for network efficiency
- **Scalable Queries:** Handles large denomination queries efficiently

**Used By:** Shard switching preparation, coin migration planning

**Dependencies:** Database layer, denomination utilities

### 3. Switch Shard Sum with SNs Command (`cmd_switch_shard_sum_with_sns`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status)

**Purpose:** Executes cross-shard coin migration by deleting coins from legacy shards and creating new coins in current RAIDAX system.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (50 bytes)
   - Extracts session ID, target shard ID, and coin data
   - Validates shard ID is within acceptable range

2. **Multi-Shard Support:**
   - **CloudCoin v1 (SHARD_CLOUDCOIN):** Legacy CloudCoin system
   - **CloudCoin v2 (SHARD_SUPERCOIN):** SuperCoin system  
   - **Current System:** RAIDAX native coins
   - **Error Handling:** Invalid shard IDs rejected

3. **Value Verification:**
   - **V3 Value Calculation:** Calculates total value of new coins to create
   - **Legacy Value Calculation:** Calculates value of coins to delete
   - **Cross-Shard Validation:** Ensures value conservation across migration
   - **Precision Handling:** Maintains accurate value calculations

4. **Legacy Coin Processing:**
   - **CloudCoin v1:** Uses legacy_calc_total and legacy_delete functions
   - **CloudCoin v2:** Uses fixed SuperCoin value calculation (85.125 per coin)
   - **Test Mode:** Session ID = 0 enables testing without actual deletion
   - **Authentication:** Validates legacy coins before proceeding

5. **New Coin Creation:**
   - **Dual Hashing Support:** Chooses algorithm based on client encryption type
     - **Legacy (encryption_type < 4):** Uses generate_an_hash_legacy (MD5)
     - **Modern (encryption_type >= 4):** Uses generate_an_hash (SHA-256)
   - **Authentication Generation:** Creates authentication numbers from provided data
   - **Coin Initialization:** Sets MFS to current timestamp and marks as owned

6. **Bitmap Integration:**
   - **NEW:** Updates free pages bitmap for each created coin
   - **Status Tracking:** Marks new coins as not free in bitmap
   - **Consistency:** Maintains perfect sync between coin data and bitmap

7. **Statistics Update:**
   - **Operation Tracking:** Updates POWN statistics for each created coin
   - **Value Tracking:** Records total value of migrated coins
   - **Performance Metrics:** Tracks shard switching performance

**Migration Features:**
- **Value Conservation:** Ensures equal value across shard migration
- **Atomic Operations:** Either complete migration succeeds or fails
- **Legacy Compatibility:** Supports migration from multiple legacy systems
- **Test Mode:** Enables testing without actual coin destruction

**Used By:** Cross-shard migration, legacy system retirement, coin system upgrades

**Dependencies:** Database layer, legacy coin systems, cryptographic functions, bitmap system

### 4. Switch Shard Sum Command (`cmd_switch_shard_sum`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status)

**Purpose:** Alternative shard switching implementation (currently unimplemented stub).

**Process:**
1. **Stub Implementation:**
   - Logs command execution
   - Returns ERROR_NOT_IMPLEMENTED status
   - **Future Extension:** Ready for alternative switching logic

**Used By:** Alternative shard switching (when implemented)

**Dependencies:** None (stub implementation)

### 5. Pickup Coins Command (`cmd_pickup_coins`)
**Parameters:**
- Connection info structure containing request body

**Returns:** None (sets connection status)

**Purpose:** Takes ownership of coins that have been moved from another shard, completing the cross-shard migration process.

**Process:**
1. **Request Validation:**
   - Validates minimum request size (43 bytes)
   - Extracts session ID and coin list
   - Calculates coin count: (body_size - 38) / 5

2. **Session Verification:**
   - **Session-Based Security:** Validates session ID for each coin
   - **Page Reservation Check:** Ensures pages are reserved by requesting session
   - **Access Control:** Prevents unauthorized coin pickup

3. **Coin Creation Process:**
   - For each coin:
     - **Page Loading:** Uses get_page_by_sn_lock for thread-safe access
     - **Reservation Validation:** Confirms page reserved by session
     - **Authentication Generation:** Creates authentication number from session data

4. **Dual Hashing Integration:**
   - **Algorithm Selection:** Based on client encryption type
     - **Legacy Support:** MD5 hashing for older clients
     - **Modern Security:** SHA-256 hashing for newer clients
   - **Hash Input:** Uses session data for deterministic generation
   - **Security:** Prevents authentication number prediction

5. **Coin Finalization:**
   - **AN Assignment:** Sets generated authentication number
   - **Timestamp:** Sets MFS to current timestamp
   - **Persistence:** Marks page as dirty for automatic persistence
   - **Bitmap Update:** Marks coin as not free in bitmap

6. **Statistics and Auditing:**
   - **Operation Tracking:** Updates POWN statistics
   - **Value Tracking:** Records coin values
   - **Audit Trail:** Maintains complete audit trail

**Pickup Features:**
- **Session Security:** Strict session-based access control
- **Dual Hashing:** Supports both legacy and modern clients
- **Atomic Creation:** Either all coins picked up or operation fails
- **Audit Trail:** Complete tracking of coin creation

**Used By:** Cross-shard migration completion, coin claiming operations

**Dependencies:** Database layer, session management, cryptographic functions, bitmap system

## Cross-Shard Architecture

### Shard Identification
- **Shard Types:** Multiple blockchain/coin system types supported
- **Legacy Support:** Backward compatibility with older coin systems
- **Migration Paths:** Clear migration paths between shard types
- **Extensibility:** Architecture supports additional shard types

### Value Conservation
- **Cross-Shard Validation:** Ensures equal value across migrations
- **Precision Handling:** Maintains accurate value calculations
- **Exchange Rate Management:** Handles different value systems
- **Audit Trail:** Complete tracking of value transfers

### Session Management
- **Session-Based Operations:** All operations tied to specific sessions
- **Security:** Session IDs provide access control
- **State Management:** Maintains operation state across requests
- **Timeout Handling:** Automatic cleanup of expired sessions

## Legacy System Integration

### CloudCoin v1 Integration
- **Legacy Functions:** Uses legacy_calc_total, legacy_delete, legacy_detect
- **Value Calculation:** Dynamic value calculation based on actual coins
- **Authentication:** Full authentication before deletion
- **Compatibility:** Maintains full backward compatibility

### CloudCoin v2/SuperCoin Integration
- **Fixed Value Model:** 85.125 value per SuperCoin
- **Simplified Processing:** Streamlined processing for SuperCoins
- **Migration Support:** Smooth migration to current system
- **Value Conservation:** Accurate value conversion

### Modern RAIDAX Integration
- **Native Processing:** Direct processing in current system
- **Advanced Features:** Full feature set available
- **Performance Optimization:** Optimized for current architecture
- **Future Development:** Platform for future enhancements

## Dual Hashing Support

### Algorithm Selection
- **Client-Driven:** Hash algorithm based on client capabilities
- **Legacy Support:** MD5 for older clients requiring compatibility
- **Modern Security:** SHA-256 for newer clients with enhanced security
- **Seamless Integration:** Transparent algorithm selection

### Hash Input Construction
- **Deterministic Generation:** Reproducible hash inputs
- **Session Integration:** Session data provides unique input
- **Security:** Prevents authentication number prediction
- **Compatibility:** Works across all client versions

## Free Pages Bitmap Integration

### Migration Tracking
- **Real-Time Updates:** Bitmap updated immediately during migrations
- **Status Accuracy:** Perfect tracking of coin availability
- **Performance:** Instant availability queries after migration
- **Consistency:** Maintains sync with actual coin data

### Cross-Shard Consistency
- **Universal Tracking:** Bitmap tracks coins regardless of origin shard
- **Migration Visibility:** Migrated coins immediately visible in bitmap
- **Performance Benefits:** Fast queries independent of shard history
- **System Integration:** Seamless integration with existing systems

## Error Handling and Recovery

### Migration Error Handling
- **Value Validation:** Comprehensive value validation before migration
- **Session Validation:** Strict session validation throughout process
- **Atomic Operations:** Either complete migration or clean failure
- **Rollback Preparation:** Infrastructure for future rollback capability

### Cross-Shard Validation
- **Shard ID Validation:** Comprehensive validation of shard identifiers
- **Legacy System Errors:** Proper handling of legacy system failures
- **Value Mismatch Detection:** Detection and handling of value mismatches
- **Session Timeout Handling:** Graceful handling of session timeouts

### System Recovery
- **Partial Migration Recovery:** Handling of partially completed migrations
- **Data Consistency:** Maintains consistent state across failures
- **Audit Trail:** Complete audit trail for recovery procedures
- **State Reconstruction:** Ability to reconstruct system state

## Performance Characteristics

### Migration Performance
- **Batch Processing:** Efficient handling of multiple coin migrations
- **Legacy System Optimization:** Optimized interaction with legacy systems
- **Database Efficiency:** Leverages on-demand page cache for performance
- **Network Optimization:** Efficient protocols for cross-shard communication

### Memory Management
- **Dynamic Allocation:** Memory allocated based on actual migration sizes
- **Efficient Processing:** Optimized data structures for migration operations
- **Resource Cleanup:** Comprehensive cleanup prevents resource leaks
- **Cache Utilization:** Efficient use of database page cache

### Scalability Features
- **Large Migrations:** Supports large-scale cross-shard migrations
- **Concurrent Operations:** Multiple migrations can proceed simultaneously
- **Resource Bounds:** All resource usage is bounded and predictable
- **Load Distribution:** Even distribution of migration processing load

## Dependencies and Integration

### Required Modules
- **Database Layer:** Coin data access and modification
- **Legacy Systems:** CloudCoin v1 and v2 integration modules
- **Cryptographic Functions:** Hash generation for authentication numbers
- **Session Management:** Session-based security and state management
- **Bitmap System:** Real-time coin availability tracking

### Used By
- **Migration Tools:** Primary interface for cross-shard migration
- **Administrative Systems:** System administration and maintenance
- **Client Applications:** User-initiated migration operations
- **Legacy Integration:** Bridge between old and new systems

### Cross-File Dependencies
- **Database Module:** Page access and coin data management
- **Legacy Modules:** CloudCoin v1/v2 integration functions
- **Utilities Module:** Cryptographic functions and data conversion
- **Configuration Module:** Shard configuration and system parameters
- **Statistics Module:** Operation tracking and performance metrics

This shard command module provides comprehensive cross-shard migration capabilities with legacy system integration, dual hashing support, session-based security, and real-time bitmap synchronization, enabling seamless migration of coins between different blockchain and coin systems while maintaining value conservation and data integrity.