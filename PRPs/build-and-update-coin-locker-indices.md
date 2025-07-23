# Locker Index Management System (locker)

## Module Purpose
This module implements optimized indexing for coin lockers, providing fast lookup and management of coins stored in lockers and trade lockers. It uses incremental updates, periodic verification, and integrates with the new on-demand page cache system for efficient locker operations while supporting both regular lockers and trading functionality.

## Constants and Configuration

### Index Limits
| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_LOCKER_RECORDS` | Variable | Maximum number of locker entries that can be indexed |
| `PREALLOCATE_COINS` | Variable | Number of coins to preallocate per locker entry for efficiency |
| `INDEX_UPDATE_PERIOD` | Variable | Base period for index updates (multiplied by 4 for verification) |

### Locker Identification Patterns
| Pattern | Type | Description |
|---------|------|-------------|
| `0xffffffff` | Regular Locker | Last 4 bytes of authentication number for regular lockers |
| `0xeeee` | Trade Locker | Bytes 14-15 of authentication number for trade lockers |

### Trade Coin Types
| Constant | Value | Description |
|----------|-------|-------------|
| `SALE_TYPE_CC` | Variable | CloudCoin trade type identifier |
| `SALE_TYPE_BTC` | Variable | Bitcoin trade type identifier |
| `SALE_TYPE_XMR` | Variable | Monero trade type identifier |

## Data Structures

### Index Entry Structure
| Field | Type | Description |
|-------|------|-------------|
| `an` | Byte Array[16] | Authentication number (locker identifier) |
| `coins` | Coin Pointer | Dynamic array of coins in this locker |
| `num_coins` | Integer | Number of coins currently in locker |

### Coin Structure
| Field | Type | Description |
|-------|------|-------------|
| `denomination` | 8-bit Integer | Coin denomination |
| `sn` | 32-bit Integer | Coin serial number |

### Index Management
| Field | Type | Description |
|-------|------|-------------|
| `locker_index` | Index Entry Pointer Array[MAX_LOCKER_RECORDS] | Main locker index array |
| `trade_locker_index` | Index Entry Pointer Array[MAX_LOCKER_RECORDS] | Trade locker index array |
| `locker_mtx` | Mutex | Thread safety for main locker operations |
| `trade_locker_mtx` | Mutex | Thread safety for trade locker operations |

## Core Functionality

### 1. Initialize Locker Index (`init_locker_index`)
**Parameters:** None

**Returns:** Integer (0 for success, -1 for failure)

**Purpose:** Initializes locker index system with thread safety, builds initial indices, and launches maintenance thread.

**Process:**
1. **Index Array Initialization:**
   - Initializes all locker_index entries to NULL
   - Initializes all trade_locker_index entries to NULL
   - Prepares clean state for index building

2. **Mutex Initialization:**
   - Creates locker_mtx for main locker thread safety
   - Creates trade_locker_mtx for trade locker thread safety
   - Handles mutex creation failures

3. **Initial Index Building:**
   - Calls update_index() to build complete regular locker index
   - Calls update_trade_index() to build complete trade locker index
   - Establishes baseline index state

4. **Background Thread Launch:**
   - Launches index_thread for periodic maintenance
   - Thread performs verification and cleanup operations
   - Configured for continuous background operation

**Used By:** Server initialization

**Dependencies:** Threading system, database layer, memory management

### 2. Index Thread (`index_thread`)
**Parameters:**
- Thread argument (unused)

**Returns:** Thread result

**Purpose:** Background maintenance thread that performs periodic verification and cleanup with reduced frequency due to incremental updates.

**Process:**
1. **Periodic Cycle:**
   - Sleeps for INDEX_UPDATE_PERIOD * 4 seconds
   - Runs 4x less frequently than original due to incremental updates
   - Continues until system shutdown

2. **Verification and Cleanup:**
   - Calls verify_and_cleanup_indices() for maintenance
   - Performs consistency checks and orphaned entry removal
   - Maintains index integrity over time

**Performance Optimization:**
- **Reduced Frequency:** Incremental updates allow less frequent full verification
- **Efficient Maintenance:** Focus on verification rather than rebuilding
- **Background Operation:** Non-blocking maintenance

**Used By:** Background maintenance

**Dependencies:** Timing functions, index verification

### 3. Verify and Cleanup Indices (`verify_and_cleanup_indices`)
**Parameters:** None

**Returns:** None

**Purpose:** Performs periodic verification to ensure indices are consistent and removes orphaned entries.

**Process:**
1. **Consistency Verification:**
   - Checks for orphaned entries that no longer correspond to valid lockers
   - Verifies index integrity and structure
   - Identifies inconsistencies for correction

2. **Cleanup Operations:**
   - Removes invalid or expired entries
   - Consolidates fragmented index space
   - Optimizes memory usage

**Note:** Current implementation trusts incremental updates are correct and focuses on basic verification.

**Used By:** Index thread

**Dependencies:** Index structures

### 4. Update Index (`update_index`)
**Parameters:** None

**Returns:** None

**Purpose:** Performs complete rebuild of regular locker index, now only called at startup or for recovery operations.

**Process:**
1. **Index Clearing:**
   - Acquires locker_mtx for thread safety
   - Calls free_index() to clear existing index
   - Prepares for complete rebuild

2. **Denomination Iteration:**
   - Iterates through all denominations (MIN_DENOMINATION to MAX_DENOMINATION)
   - For each denomination, processes all pages
   - Builds comprehensive index

3. **Page Processing with New Database System:**
   - **Fixed Integration:** Uses get_page_by_sn_lock instead of deprecated get_page_lock
   - Gets page using representative serial number (page_number * RECORDS_PER_PAGE)
   - Handles page access failures gracefully

4. **Locker Detection:**
   - For each coin record in page:
     - Checks bytes 12-15 of authentication number
     - Identifies regular lockers by 0xffffffff pattern
     - Adds matching coins to index using add_index_entry_internal

5. **Thread Safety:**
   - Releases page locks properly using unlock_page
   - Maintains locker_mtx throughout operation
   - Ensures atomic index updates

**Used By:** System initialization, recovery operations

**Dependencies:** Database layer, memory management

### 5. Update Trade Index (`update_trade_index`)
**Parameters:** None

**Returns:** None

**Purpose:** Performs complete rebuild of trade locker index with support for multiple trade coin types.

**Process:**
1. **Index Clearing:**
   - Acquires trade_locker_mtx for thread safety
   - Calls free_trade_index() to clear existing index
   - Prepares for complete rebuild

2. **Denomination and Page Iteration:**
   - Iterates through all denominations and pages
   - **Fixed Integration:** Uses get_page_by_sn_lock for database access
   - Processes each page systematically

3. **Trade Locker Detection:**
   - For each coin record in page:
     - Extracts coin type from byte 13 of authentication number
     - Validates coin type using is_good_trade_coin_type
     - Checks bytes 14-15 for trade locker pattern (0xeeee)
     - Adds matching coins to trade index

4. **Thread Safety:**
   - Maintains proper page locking with unlock_page
   - Holds trade_locker_mtx throughout operation
   - Ensures atomic trade index updates

**Used By:** System initialization, recovery operations

**Dependencies:** Database layer, trade validation functions

### 6. Incremental Index Management

#### Locker Index Add Coins (`locker_index_add_coins`)
**Parameters:**
- Authentication number (16-byte array)
- Coins to add (coin array)
- Number of coins (integer)

**Returns:** None

**Purpose:** Incrementally adds coins to locker index when new coins are stored in lockers.

**Process:**
1. **Thread Safety:**
   - Acquires locker_mtx for exclusive access
   - Ensures atomic updates to index

2. **Coin Addition:**
   - For each coin to add:
     - Calls add_index_entry_internal with authentication number
     - Handles memory allocation for new entries
     - Updates existing entries with additional coins

3. **Performance Benefits:**
   - Avoids full index rebuild
   - Provides immediate index updates
   - Maintains index consistency

**Used By:** Locker storage operations

**Dependencies:** Internal index management functions

#### Locker Index Remove Coins (`locker_index_remove_coins`)
**Parameters:**
- Authentication number (16-byte array)
- Coins to remove (coin array)
- Number of coins (integer)

**Returns:** None

**Purpose:** Incrementally removes coins from locker index when coins are retrieved from lockers.

**Process:**
1. **Index Entry Location:**
   - Acquires locker_mtx for thread safety
   - Searches through locker_index for matching authentication number
   - Handles case where locker not found

2. **Coin Removal Process:**
   - For each coin to remove:
     - Searches through locker's coin array
     - Finds matching denomination and serial number
     - Removes coin by shifting remaining coins in array

3. **Array Compaction:**
   - Shifts remaining coins to fill removed coin slots
   - Decrements num_coins counter
   - Maintains array integrity

4. **Empty Locker Cleanup:**
   - Detects when locker becomes empty (num_coins == 0)
   - Frees coin array memory
   - Removes index entry entirely
   - Prevents memory leaks

**Used By:** Locker retrieval operations

**Dependencies:** Memory management, array manipulation

#### Trade Locker Index Add Coins (`trade_locker_index_add_coins`)
**Parameters:**
- Authentication number (16-byte array)
- Coins to add (coin array)
- Number of coins (integer)

**Returns:** None

**Purpose:** Incrementally adds coins to trade locker index for trading operations.

**Process:**
1. **Thread Safety:**
   - Acquires trade_locker_mtx for exclusive access
   - Ensures atomic updates to trade index

2. **Coin Addition:**
   - For each coin to add:
     - Calls add_trade_index_entry_internal
     - Updates trade locker entries
     - Handles memory allocation

**Used By:** Trade locker creation operations

**Dependencies:** Trade index management functions

#### Trade Locker Index Remove Coins (`trade_locker_index_remove_coins`)
**Parameters:**
- Authentication number (16-byte array)
- Coins to remove (coin array)
- Number of coins (integer)

**Returns:** None

**Purpose:** Incrementally removes coins from trade locker index when trades are completed.

**Process:**
1. **Trade Entry Location:**
   - Searches trade_locker_index for matching authentication number
   - Handles missing trade locker gracefully

2. **Coin Removal:**
   - Similar process to regular locker removal
   - Maintains trade-specific index integrity
   - Handles empty trade locker cleanup

**Used By:** Trade completion operations

**Dependencies:** Memory management, trade index structures

### 7. Internal Index Management

#### Add Index Entry Internal (`add_index_entry_internal`)
**Parameters:**
- Denomination (8-bit integer)
- Serial number (32-bit integer)
- Authentication number (16-byte array)

**Returns:** Integer (0 for success, -1 for failure, static function)

**Purpose:** Internal function that adds entry to regular locker index with efficient memory management.

**Process:**
1. **Entry Search:**
   - Searches for existing entry with same authentication number
   - Finds empty slot if new entry needed
   - Handles index table full condition

2. **New Entry Creation:**
   - Allocates new index_entry structure
   - Preallocates PREALLOCATE_COINS coin slots for efficiency
   - Initializes entry with first coin

3. **Existing Entry Update:**
   - Adds coin to existing entry's coin array
   - Handles memory reallocation when array full
   - Uses PREALLOCATE_COINS chunks for efficiency

4. **Memory Management:**
   - Reallocates coin array in PREALLOCATE_COINS increments
   - Reduces memory allocation overhead
   - Prevents excessive reallocation calls

**Used By:** Index building and incremental updates

**Dependencies:** Memory allocation

#### Add Trade Index Entry Internal (`add_trade_index_entry_internal`)
**Parameters:**
- Denomination (8-bit integer)
- Serial number (32-bit integer)
- Authentication number (16-byte array)

**Returns:** Integer (0 for success, -1 for failure, static function)

**Purpose:** Internal function that adds entry to trade locker index with similar memory management.

**Process:**
1. **Similar to Regular Index:**
   - Follows same pattern as add_index_entry_internal
   - Uses trade_locker_index array instead
   - Maintains same memory allocation strategy

2. **Trade-Specific Logging:**
   - Logs trade locker additions for debugging
   - Provides visibility into trade operations

**Used By:** Trade index building and updates

**Dependencies:** Memory allocation, logging

### 8. Index Cleanup Functions

#### Free Index (`free_index`)
**Parameters:** None

**Returns:** None

**Purpose:** Frees all memory associated with regular locker index (must be called with mutex held).

**Process:**
1. **Entry Iteration:**
   - Iterates through all locker_index entries
   - Skips NULL entries

2. **Memory Deallocation:**
   - Frees coin array for each entry
   - Frees index entry structure
   - Sets array slot to NULL

3. **Statistics:**
   - Counts freed entries for debugging
   - Logs cleanup statistics

**Used By:** Index rebuilding, system shutdown

**Dependencies:** Memory management

#### Free Trade Index (`free_trade_index`)
**Parameters:** None

**Returns:** None

**Purpose:** Frees all memory associated with trade locker index with same pattern as regular index.

**Process:**
1. **Similar Cleanup:**
   - Follows same pattern as free_index
   - Works on trade_locker_index array
   - Maintains same statistics

**Used By:** Trade index rebuilding, system shutdown

**Dependencies:** Memory management

### 9. Index Access Functions

#### Get Coins from Index (`get_coins_from_index`)
**Parameters:**
- Authentication number (16-byte array)

**Returns:** Index entry pointer (NULL if not found)

**Purpose:** Searches regular locker index for exact authentication number match and returns complete entry.

**Process:**
1. **Thread-Safe Search:**
   - Acquires locker_mtx for read access
   - Ensures consistent view during search

2. **Linear Search:**
   - Iterates through all locker_index entries
   - Skips NULL entries
   - Compares full 16-byte authentication number using memcmp

3. **Result Handling:**
   - Returns matching entry pointer if found
   - Returns NULL if no match found
   - Releases mutex before returning

**Used By:** Locker access operations, coin retrieval

**Dependencies:** Threading system, memory comparison

#### Get Coins from Trade Index (`get_coins_from_trade_index`)
**Parameters:**
- Authentication number (16-byte array)

**Returns:** Index entry pointer (NULL if not found)

**Purpose:** Searches trade locker index for exact authentication number match.

**Process:**
1. **Thread-Safe Search:**
   - Acquires trade_locker_mtx for read access
   - Ensures consistent trade index view

2. **Search Process:**
   - Similar to regular index search
   - Works on trade_locker_index array
   - Uses same comparison logic

**Used By:** Trade operations, trade locker access

**Dependencies:** Threading system, memory comparison

#### Load Coins from Trade Index (`load_coins_from_trade_index`)
**Parameters:**
- Trade coin type (8-bit integer)
- Maximum number of entries (8-bit integer)
- Output index entry array (index entry pointer array)

**Returns:** Integer (number of entries found)

**Purpose:** Searches trade index for specific coin type and loads multiple matching entries.

**Process:**
1. **Thread-Safe Search:**
   - Acquires trade_locker_mtx for read access
   - Ensures consistent view during batch operation

2. **Type-Based Search:**
   - Iterates through trade_locker_index entries
   - Checks byte 13 of authentication number for coin type match
   - Collects matching entries up to specified limit

3. **Result Collection:**
   - Adds matching entries to output array
   - Counts total entries found
   - Stops when limit reached

**Used By:** Trade browsing operations, market listings

**Dependencies:** Threading system, trade type validation

#### Get Coins from Index by Prefix (`get_coins_from_index_by_prefix`)
**Parameters:**
- Authentication number prefix (5-byte array)

**Returns:** Index entry pointer (NULL if not found)

**Purpose:** Searches regular locker index using only first 5 bytes of authentication number for encryption operations.

**Process:**
1. **Prefix Search:**
   - Acquires locker_mtx for thread safety
   - Searches using partial authentication number match
   - Compares only first 5 bytes using memcmp

2. **Use Case:**
   - Supports encryption operations that only have partial keys
   - Enables locker access with abbreviated authentication numbers
   - Provides compatibility with legacy operations

**Used By:** Encryption operations, legacy compatibility

**Dependencies:** Threading system, partial memory comparison

### 10. Trade-Specific Functions

#### Is Good Trade Coin Type (`is_good_trade_coin_type`)
**Parameters:**
- Trade coin type (8-bit integer)

**Returns:** Integer (1 if valid, 0 if invalid)

**Purpose:** Validates whether a coin type identifier represents a supported trade currency.

**Process:**
1. **Type Validation:**
   - Checks if coin type equals SALE_TYPE_CC (CloudCoin)
   - Checks if coin type equals SALE_TYPE_BTC (Bitcoin)
   - Checks if coin type equals SALE_TYPE_XMR (Monero)

2. **Return Logic:**
   - Returns 1 if type matches any supported currency
   - Returns 0 if type is not supported

**Used By:** Trade validation, index building

**Dependencies:** Trade type constants

#### Get Entry from Trade Index (`get_entry_from_trade_index`)
**Parameters:**
- Trade coin type (8-bit integer)
- Amount (64-bit integer)
- Price (32-bit integer)

**Returns:** Index entry pointer (NULL if not found)

**Purpose:** Searches trade index for specific trading criteria including type, amount, and price.

**Process:**
1. **Thread-Safe Search:**
   - Acquires trade_locker_mtx for exclusive access
   - Ensures atomic search operation

2. **Multi-Criteria Matching:**
   - Filters by coin type (byte 13 of authentication number)
   - Extracts price from bytes 9-12 using get_u32
   - Calculates locker amount using calc_coins_in_trade_locker
   - Matches all three criteria (type, amount, price)

3. **Exact Matching:**
   - Requires precise match of all criteria
   - Returns first matching entry found
   - Logs successful matches for debugging

**Used By:** Trade execution, order matching

**Dependencies:** Value calculation, network byte order conversion

#### Calculate Coins in Trade Locker (`calc_coins_in_trade_locker`)
**Parameters:**
- Index entry pointer

**Returns:** 64-bit integer (total value)

**Purpose:** Calculates total monetary value of all coins in a trade locker.

**Process:**
1. **Value Accumulation:**
   - Iterates through all coins in trade locker entry
   - For each coin, calls coin_value function
   - Accumulates total value across all denominations

2. **Precision Handling:**
   - Uses 64-bit arithmetic for precise calculations
   - Handles large value sums correctly
   - Maintains accuracy across different denominations

**Used By:** Trade matching, value verification

**Dependencies:** Value calculation functions

### 11. Debug and Display Functions

#### Show Index (`show_index`)
**Parameters:** None

**Returns:** None

**Purpose:** Displays regular locker index contents for debugging and monitoring.

**Process:**
1. **Index Iteration:**
   - Iterates through all locker_index entries
   - Skips NULL entries

2. **Entry Display:**
   - For each entry, displays authentication number (abbreviated)
   - Shows coin count for entry
   - Lists all coins with denomination and serial number

3. **Format:**
   - Shows first 4 and last 4 bytes of authentication number
   - Provides compact but informative display
   - Useful for debugging and monitoring

**Used By:** Debugging, administrative monitoring

**Dependencies:** Logging system

#### Show Trade Index (`show_trade_index`)
**Parameters:** None

**Returns:** None

**Purpose:** Displays trade locker index contents with same format as regular index.

**Process:**
1. **Similar Display:**
   - Follows same pattern as show_index
   - Works on trade_locker_index array
   - Provides same level of detail

**Used By:** Trade debugging, administrative monitoring

**Dependencies:** Logging system

## Performance Characteristics

### Memory Efficiency
- **Preallocation Strategy:** PREALLOCATE_COINS reduces memory allocation overhead
- **Incremental Updates:** Avoid full rebuilds through targeted updates
- **Efficient Storage:** Compact data structures minimize memory usage
- **Dynamic Growth:** Arrays grow as needed without excessive waste

### Search Performance
- **Linear Search:** O(n) search through index entries
- **Thread-Safe Access:** Multiple readers supported concurrently
- **Cache-Friendly:** Sequential access patterns optimize cache usage
- **Indexed Access:** Direct array indexing for known positions

### Update Performance
- **Incremental Updates:** Add/remove operations are O(1) for existing entries
- **Batch Operations:** Multiple coins can be processed efficiently
- **Memory Reallocation:** Chunked allocation reduces reallocation frequency
- **Background Maintenance:** Verification runs in background without blocking

## Threading and Concurrency

### Thread Safety Design
- **Dual Mutex System:** Separate mutexes for regular and trade indices
- **Read-Write Separation:** Multiple readers can access index concurrently
- **Atomic Updates:** Index modifications are atomic operations
- **Deadlock Prevention:** Consistent lock ordering prevents deadlocks

### Concurrency Optimization
- **Fine-Grained Locking:** Separate locks for regular and trade operations
- **Short Critical Sections:** Minimal time spent holding locks
- **Background Processing:** Maintenance operations don't block access
- **Lock-Free Reads:** Some operations designed for minimal locking

## Integration with Database System

### Fixed Database Integration
- **Modern API Usage:** Uses get_page_by_sn_lock instead of deprecated functions
- **Proper Page Handling:** Correctly handles page locking and unlocking
- **Error Handling:** Graceful handling of page access failures
- **Memory Management:** Proper cleanup of database resources

### Performance Benefits
- **Cache Utilization:** Benefits from database page caching
- **Reduced I/O:** Database cache reduces disk access
- **Thread Safety:** Database layer provides thread-safe page access
- **Consistency:** Integrated with database consistency mechanisms

## Security Considerations

### Access Control
- **Authentication Required:** All operations require valid authentication numbers
- **Pattern Validation:** Locker patterns validated before index inclusion
- **Type Validation:** Trade coin types validated for security
- **Bounds Checking:** All array access protected by bounds checking

### Data Integrity
- **Atomic Operations:** Index updates are atomic to prevent corruption
- **Validation:** All input data validated before processing
- **Consistency Checks:** Periodic verification maintains data integrity
- **Error Recovery:** Graceful handling of inconsistent states

## Error Handling and Recovery

### Memory Management Errors
- **Allocation Failures:** Graceful handling of memory allocation failures
- **Resource Leaks:** Systematic cleanup prevents memory leaks
- **Fragmentation:** Preallocation strategy reduces fragmentation
- **Recovery:** System continues operating with partial failures

### Index Corruption Recovery
- **Verification:** Periodic verification detects corruption
- **Rebuilding:** Full index rebuild capability for recovery
- **Partial Recovery:** Incremental updates can fix specific issues
- **Logging:** Comprehensive logging aids in debugging

## Administrative Interface

### Monitoring Functions
- **Index Display:** Functions to display index contents
- **Statistics:** Counters for entries and operations
- **Performance Metrics:** Timing information for operations
- **Health Checks:** Verification of index integrity

### Maintenance Operations
- **Manual Rebuild:** Administrative functions to rebuild indices
- **Cleanup:** Functions to remove orphaned entries
- **Verification:** Manual integrity checking capabilities
- **Statistics Reset:** Ability to reset performance counters

## Dependencies and Integration

### Required Modules
- **Database Layer:** Page access and coin data reading
- **Threading System:** Mutex operations and thread management
- **Memory Management:** Dynamic allocation and deallocation
- **Configuration System:** System parameters and limits
- **Utilities Module:** Value calculations and data conversion

### Used By
- **Locker Commands:** Storage and retrieval operations
- **Trade Commands:** Trading and market operations
- **Administrative Tools:** Index management and monitoring
- **Healing System:** Consistency verification and repair

### Cross-File Dependencies
- **Database Module:** Page structure and access patterns
- **Command Modules:** Locker and trade command implementations
- **Configuration Module:** System limits and parameters
- **Utilities Module:** Value calculation and data conversion functions

This locker index system provides efficient, thread-safe management of locker and trade locker indices with optimized incremental updates, proper integration with the modern database system, and comprehensive support for both storage and trading operations while maintaining high performance and data integrity.