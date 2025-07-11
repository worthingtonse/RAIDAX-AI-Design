
# Locker Index Management Implementation (locker)

## Module Purpose
This module implements an optimized indexing system for coin lockers and trade lockers in the RAIDA network. It provides fast lookup capabilities for coin collections stored in secure lockers, manages a marketplace for trading coin bundles, and implements incremental index updates for optimal performance. The system replaces expensive full index rebuilds with efficient incremental updates, significantly improving performance and scalability.

## Core Architecture

*Note: Authentication number is a unique 16-byte identifier, used as the locker ID.*
*Synchronization primitives (mutexes) refer to thread-safe locks ensuring atomic access to shared data structures.*

## Constants Reference
- `MAX_LOCKER_RECORDS = 100000`  // Max active lockers
- `PREALLOCATE_COINS = 2`        // Initial coin array size
- `INDEX_UPDATE_PERIOD = 3600`   // Time in seconds
- `RECORDS_PER_PAGE = X`         // Depends on DB layout
- ...

### Coin Structure
- **denomination**: Integer (typically 1–250)
- **serial_number**: Integer
- **other optional fields**: Owner info, flags, or history (implementation-specific)


### Indexing Strategy
**Traditional Approach:** Periodic full index rebuilds scanning entire database
**Optimized Approach:** Incremental updates with occasional verification, dramatically reducing CPU overhead

**Performance Benefits:**
- Eliminates expensive full database scans during normal operation
- Reduces index update frequency from continuous to occasional
- Maintains index accuracy through targeted updates
- Scales efficiently with growing locker populations

### Dual Index System
- **Locker Index:** Standard coin storage lockers for secure collection management
- **Trade Locker Index:** Marketplace lockers for coin trading with pricing information
- **Incremental Updates:** Both indices updated incrementally as operations occur
- **Verification Thread:** Background verification ensures long-term index consistency

## Core Functionality

### 1. System Initialization (`init_locker_index`)
**Parameters:** None

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Initializes both locker indexing systems and starts optimized background processing.

**Process:**
1. **Index Array Initialization:**
   - Clears all locker index entries to NULL
   - Clears all trade locker index entries to NULL
   - Prepares index arrays for dynamic population

2. **Mutex Initialization:**
   - Creates locker index mutex for thread safety
   - Creates trade locker index mutex for independent locking
   - Enables concurrent access to different index types

3. **Initial Index Building:**
   - Performs full index build for locker entries
   - Performs full index build for trade locker entries
   - Establishes baseline state for incremental updates

4. **Background Thread Launch:**
   - Starts verification thread with reduced frequency (4x less frequent)
   - Configures thread for periodic consistency checking
   - Enables long-term index integrity maintenance

**Dependencies:**
- Threading system for background processing
- Database layer for initial index population
- Synchronization primitives for thread safety

### 2. Background Verification Thread (`index_thread`)
**Parameters:** Thread argument pointer (unused)

**Returns:** Thread result pointer

**Purpose:** Optimized background thread that performs periodic verification rather than continuous rebuilding.

**Process:**
1. **Reduced Frequency Operation:**
   - Sleeps for INDEX_UPDATE_PERIOD × 4 (4x less frequent than original)
   - Minimizes CPU overhead during normal operation

2. **Verification and Cleanup:**
   - Calls verify_and_cleanup_indices for consistency checking
   - Trusts incremental updates for accuracy
   - Performs minimal validation and orphan cleanup

**Performance Optimization:**
- Dramatically reduced CPU usage compared to continuous rebuilding
- Maintains long-term consistency without performance penalty
- Enables system to scale to much larger locker populations

### 3. Periodic Verification (`verify_and_cleanup_indices`)
**Parameters:** None

**Returns:** None

**Purpose:** Lightweight verification function that checks for orphaned entries and maintains index consistency.

**Process:**
1. Performs minimal consistency checking
2. Removes any orphaned or stale index entries
3. Validates index integrity without full rebuilds
4. Logs verification status

**Design Philosophy:** Trust incremental updates, verify occasionally

### 4. Initial Index Population (`update_index`)
**Parameters:** None

**Returns:** None

**Purpose:** Builds initial locker index by scanning database for locker-stored coins (called only at startup or for recovery).

**Process:**
1. **Index Cleanup:**
   - Acquires exclusive lock on locker index
   - Frees any existing index entries
   - Prepares for fresh index population

2. **Database Scanning:**
   - Iterates through all denominations from MIN to MAX
   - For each denomination, scans all possible database pages
   - Retrieves pages using database layer page access functions

3. **Locker Detection:**
   - For each coin record on each page:
     - Checks authentication number pattern for locker storage
     - Identifies lockers by specific byte pattern (bytes 12-15 = 0xff)
     - Calculates serial number from page and record position

4. **Index Entry Creation:**
   - Creates index entries for detected locker coins
   - Groups coins by locker authentication number
   - Uses internal helper functions for thread-safe addition

**Used By:** System initialization, index recovery operations

### 5. Trade Index Population (`update_trade_index`)
**Parameters:** None

**Returns:** None

**Purpose:** Builds initial trade locker index by scanning for marketplace-enabled coin storage.

**Process:**
1. **Trade Index Cleanup:**
   - Acquires exclusive lock on trade locker index
   - Frees existing trade index entries
   - Prepares for fresh trade index population

2. **Trade Locker Detection:**
   - Scans database pages for trade locker patterns
   - Identifies trade lockers by currency type in byte 13
   - Validates trade markers in bytes 14-15 (must be 0xee)
   - Validates supported trade coin types

3. **Trade Index Creation:**
   - Creates trade index entries for detected trade lockers
   - Includes pricing and currency information
   - Groups coins by trade locker authentication number

**Dependencies:**
- Database layer for page access and coin data retrieval
- Trade coin type validation utilities
- Currency type validation functions

## Incremental Update System

### 6. Locker Coin Addition (`locker_index_add_coins`)
**Parameters:**
- Authentication number (16-byte locker identifier)
- Coin array pointer (array of coin structures)
- Coin count (integer number of coins to add)

**Returns:** None

**Purpose:** Incrementally adds coins to locker index without full rebuild.

**Process:**
1. Acquires locker index mutex for thread safety
2. For each coin in the provided array:
   - Calls internal helper function to add coin to index
   - Creates new index entry if locker doesn't exist
   - Extends existing index entry if locker already exists
3. Releases locker index mutex

**Performance Benefit:** O(n) complexity for n coins vs. O(database_size) for full rebuild

### 7. Locker Coin Removal (`locker_index_remove_coins`)
**Parameters:**
- Authentication number (16-byte locker identifier)
- Coin array pointer (array of coin structures to remove)
- Coin count (integer number of coins to remove)

**Returns:** None

**Purpose:** Incrementally removes coins from locker index.

**Process:**
1. **Index Entry Lookup:**
   - Acquires locker index mutex
   - Searches for index entry matching authentication number

2. **Coin Removal:**
   - For each coin to remove:
     - Finds coin in index entry's coin array
     - Removes by shifting remaining coins in array
     - Decrements coin count in index entry

3. **Empty Locker Cleanup:**
   - If locker becomes empty (coin count = 0):
     - Frees coin array memory
     - Frees index entry structure
     - Sets index slot to NULL

4. **Mutex Release:** Releases locker index mutex

**Memory Management:** Automatically cleans up empty lockers to prevent memory leaks

### 8. Trade Locker Incremental Updates (`trade_locker_index_add_coins`, `trade_locker_index_remove_coins`)
**Parameters:** Same as locker functions but for trade locker management

**Purpose:** Provides incremental update capabilities for trade locker index parallel to regular locker index.

**Functionality:** Mirrors locker index operations but maintains trade-specific metadata including pricing and currency information.

## Internal Helper Functions

### 9. Index Entry Addition (`add_index_entry_internal`)
**Parameters:**
- Denomination identifier (8-bit signed integer)
- Serial number (32-bit unsigned integer)
- Authentication number (16-byte identifier)

**Returns:** Integer status code (0 for success, negative for error)

**Purpose:** Thread-unsafe internal function for adding coins to locker index (caller must hold mutex).

**Process:**
1. **Entry Location:**
   - Searches for existing entry with matching authentication number
   - Identifies empty slot if new entry needed

2. **New Entry Creation:**
   - Allocates new index entry structure
   - Allocates initial coin array (PREALLOCATE_COINS size)
   - Sets coin count to 1 and stores first coin

3. **Existing Entry Extension:**
   - Checks if coin array needs expansion
   - Reallocates memory in PREALLOCATE_COINS increments
   - Preserves existing coins while adding new coin
   - Increments coin count

**Memory Efficiency:** Preallocates coins in chunks to minimize allocation overhead

### 10. Trade Index Entry Addition (`add_trade_index_entry_internal`)
**Parameters:** Same as regular index entry addition

**Purpose:** Internal helper for trade locker index management with same optimization strategies.

## Index Access and Lookup Functions

### 11. Locker Contents Retrieval (`get_coins_from_index`)
**Parameters:**
- Authentication number (16-byte locker identifier)

**Returns:** Pointer to index entry structure (NULL if not found)

**Purpose:** Retrieves complete coin list for a specific locker.

**Process:**
1. Acquires shared lock on locker index
2. Iterates through index entries comparing authentication numbers
3. Returns matching entry or NULL if not found
4. Releases index lock

**Thread Safety:** Uses shared locking to allow concurrent read access

### 12. Trade Locker Lookup (`get_coins_from_trade_index`)
**Parameters:**
- Authentication number (16-byte trade locker identifier)

**Returns:** Pointer to trade index entry (NULL if not found)

**Purpose:** Retrieves trade locker information for marketplace operations.

### 13. Marketplace Listings (`load_coins_from_trade_index`)
**Parameters:**
- Currency type (8-bit identifier)
- Maximum results (8-bit count)
- Results array pointer (array of index entry pointers)

**Returns:** Integer count of results found

**Purpose:** Loads trade lockers for specific currency type for marketplace display.

**Process:**
1. Acquires shared lock on trade locker index
2. Iterates through trade index entries
3. Filters by currency type (byte 13 of authentication number)
4. Collects up to specified maximum results
5. Returns actual count found

### 14. Trade Locker Matching (`get_entry_from_trade_index`)
**Parameters:**
- Currency type (8-bit identifier)
- Expected amount (64-bit unsigned integer)
- Price (32-bit unsigned integer)

**Returns:** Pointer to matching trade locker entry (NULL if not found)

**Purpose:** Finds specific trade locker matching purchase criteria.

**Process:**
1. **Currency Filtering:** Filters entries by currency type
2. **Price Matching:** Compares embedded price in authentication number
3. **Value Calculation:** Calculates total coin value in trade locker
4. **Exact Matching:** Returns entry only if all criteria match exactly

**Used By:** Purchase operations requiring exact trade locker identification

## Value Calculation and Validation

### 15. Trade Locker Value Calculation (`calc_coins_in_trade_locker`)
**Parameters:**
- Index entry pointer

**Returns:** 64-bit unsigned integer total value

**Purpose:** Calculates total value of all coins in a trade locker.

**Process:**
1. Iterates through all coins in the trade locker
2. For each coin:
   - Calls coin_value function with denomination and serial number
   - Accumulates total value
3. Returns sum of all coin values

**Dependencies:**
- Utility functions for individual coin value calculation
- Denomination-based value lookup

### 16. Trade Coin Type Validation (`is_good_trade_coin_type`)
**Parameters:**
- Currency type (8-bit identifier)

**Returns:** Integer boolean (1 if valid, 0 if invalid)

**Purpose:** Validates if currency type is supported for trading.

**Supported Types:**
- **SALE_TYPE_CC (0x0):** CloudCoin trading
- **SALE_TYPE_BTC (0x1):** Bitcoin trading
- **SALE_TYPE_XMR (0x2):** Monero trading

## Memory Management and Cleanup

### 17. Index Memory Cleanup (`free_index`, `free_trade_index`)
**Parameters:** None

**Returns:** None

**Purpose:** Frees all memory associated with index entries.

**Process:**
1. Iterates through all index slots
2. For each non-NULL entry:
   - Frees coin array memory
   - Frees index entry structure
   - Sets slot to NULL
3. Logs cleanup statistics

**Used By:** System shutdown, index rebuilding operations

### 18. Index Display (`show_index`, `show_trade_index`)
**Parameters:** None

**Returns:** None

**Purpose:** Debug functions for displaying current index contents.

**Process:**
1. Iterates through all index entries
2. For each entry:
   - Displays authentication number in abbreviated format
   - Shows coin count and individual coin details
   - Logs denomination and serial number for each coin

**Used By:** Debugging, system monitoring, administrative tools

## Backward Compatibility Functions

### 19. Legacy Index Entry Functions (`add_index_entry`, `add_trade_index_entry`)
**Parameters:** Same as internal functions but with external locking

**Purpose:** Provides backward compatibility for code that calls index functions directly.

**Implementation:** Wrapper functions that acquire appropriate mutex, call internal function, and release mutex.

## Data Structures and Constants

### Index Entry Structure
**Purpose:** Stores complete information about a locker and its contents

#### Core Fields
- **an:** 16-byte authentication number (locker identifier)
- **num_coins:** Integer count of coins in the locker
- **coins:** Pointer to dynamically allocated coin array

#### Memory Management
- **PREALLOCATE_COINS:** 2 coins per allocation increment
- **Dynamic Expansion:** Coin array grows in increments as needed
- **Automatic Cleanup:** Empty lockers automatically removed

### Configuration Constants
- **MAX_LOCKER_RECORDS:** 100,000 maximum concurrent lockers
- **INDEX_UPDATE_PERIOD:** 3600 seconds (now used only for verification frequency)
- **SALE_TYPE_*:** Trade currency type identifiers

## Performance Characteristics

### Incremental Update Performance
- **Addition:** O(1) for new locker, O(k) for existing locker with k coins
- **Removal:** O(k) for k coins to remove, O(1) for empty locker cleanup
- **Lookup:** O(n) linear search through active lockers
- **Verification:** O(database_size) but only run occasionally

### Memory Efficiency
- **Preallocation Strategy:** Reduces memory allocation overhead
- **Automatic Cleanup:** Prevents memory leaks from empty lockers
- **Incremental Growth:** Memory usage grows with actual locker usage
- **Cache Locality:** Coin arrays provide good cache performance

### Scalability Improvements
- **Reduced CPU Usage:** 75% reduction in index maintenance overhead
- **Better Responsiveness:** No blocking full rebuilds during operation
- **Linear Scaling:** Performance scales linearly with locker operations
- **Background Verification:** Long-term consistency without performance impact

## Threading and Concurrency

### Synchronization Strategy
- **Dual Mutex Design:** Separate mutexes for locker and trade locker indices
- **Fine-Grained Locking:** Independent locking enables concurrent operations
- **Reader-Writer Pattern:** Shared read access, exclusive write access
- **Lock-Free Lookups:** Read operations don't block each other

### Thread Safety Guarantees
- **Atomic Updates:** Index modifications are atomic and consistent
- **No Race Conditions:** Proper synchronization prevents data corruption
- **Concurrent Access:** Multiple readers can access indices simultaneously
- **Deadlock Prevention:** Consistent lock ordering prevents deadlocks

### Performance Optimizations
- **Minimal Lock Time:** Critical sections kept as short as possible
- **Non-Blocking Reads:** Read operations don't interfere with each other
- **Batched Updates:** Multiple coin operations batched when possible
- **Lazy Cleanup:** Cleanup operations deferred to background thread

## Integration Dependencies

### Required Modules
- **Database Layer:** Page access for initial index building and verification
- **Threading System:** Mutex synchronization, background thread management
- **Memory Management:** Dynamic allocation for index entries and coin arrays
- **Utilities Module:** Coin value calculation, data validation functions
- **Logging System:** Debug output, error reporting, operational status

### External Constants Required
- `RECORDS_PER_PAGE`: Database page organization constant
- `MIN_DENOMINATION` / `MAX_DENOMINATION`: Denomination range constants
- `TOTAL_PAGES`: Maximum pages per denomination
- Coin value calculation constants

### Used By
- **Locker Commands:** Store, remove, peek operations for coin lockers
- **Trade Commands:** Marketplace operations, buying and selling
- **Administrative Tools:** Locker monitoring and management
- **Statistics Systems:** Usage tracking and performance monitoring

## Security and Validation

### Data Integrity
- **Authentication Number Validation:** Ensures proper locker identification
- **Coin Validation:** Verifies coin denomination and serial number validity
- **Index Consistency:** Periodic verification maintains long-term accuracy
- **Memory Protection:** Proper bounds checking prevents buffer overflows

### Access Control
- **Thread Safety:** Prevents concurrent modification corruption
- **Atomic Operations:** Index updates are atomic and consistent
- **Validation Checks:** Input parameters validated before processing
- **Error Handling:** Graceful handling of invalid requests

### Audit Trail
- **Operation Logging:** All index modifications logged for audit
- **Statistics Tracking:** Performance metrics for monitoring
- **Error Reporting:** Detailed error information for troubleshooting
- **Verification Results:** Periodic consistency check results logged

## Error Handling and Recovery

### Index Corruption Recovery
- **Verification Detection:** Background thread detects inconsistencies
- **Automatic Rebuild:** Corrupted indices can be rebuilt from database
- **Graceful Degradation:** System continues operating during recovery
- **Error Isolation:** Index problems don't affect other system components

### Memory Management Errors
- **Allocation Failures:** Graceful handling of memory allocation failures
- **Leak Prevention:** Automatic cleanup prevents memory leaks
- **Bounds Checking:** Array access protected against buffer overflows
- **Null Pointer Protection:** Defensive programming against null references

### Operational Errors
- **Database Access Failures:** Graceful handling of database errors
- **Thread Synchronization:** Robust error handling in concurrent scenarios
- **Invalid Parameters:** Validation and rejection of malformed requests
- **Partial Failures:** Consistent state maintained during partial operations

## Configuration and Tuning

### Index Size Tuning
- **MAX_LOCKER_RECORDS:** Configurable maximum number of concurrent lockers
- **PREALLOCATE_COINS:** Coin array allocation increment size
- **Verification Frequency:** Background thread sleep duration
- **Memory Limits:** Total memory usage limits for index storage

### Performance Tuning
- **Lock Contention:** Minimize time spent in critical sections
- **Memory Allocation:** Optimize allocation patterns for performance
- **Cache Efficiency:** Arrange data structures for cache locality
- **Thread Scheduling:** Balance verification frequency with performance

### Operational Parameters
- **Cleanup Thresholds:** When to remove empty index entries
- **Verification Triggers:** Conditions that trigger full verification
- **Error Recovery:** Automatic recovery strategies for index corruption
- **Monitoring Intervals:** Frequency of performance metric collection

## Future Enhancements

### Performance Improvements
- **Hash-Based Lookup:** Replace linear search with hash table lookup
- **Bloom Filters:** Reduce unnecessary searches for non-existent lockers
- **Lock-Free Algorithms:** Reduce synchronization overhead
- **Compressed Storage:** Reduce memory usage through data compression

### Scalability Enhancements
- **Distributed Indexing:** Support for multi-node index distribution
- **Persistent Indices:** Disk-based index storage for faster startup
- **Incremental Persistence:** Save index state to disk periodically
- **Load Balancing:** Distribute index load across multiple threads

### Advanced Features
- **Index Versioning:** Track index changes over time
- **Snapshot Support:** Point-in-time index snapshots
- **Query Optimization:** Advanced query processing for complex lookups
- **Metrics API:** Detailed performance and usage statistics

## Migration from Legacy System

### Compatibility
- **Data Format:** Maintains compatibility with existing locker formats
- **API Compatibility:** Existing code continues to work without changes
- **Gradual Migration:** Incremental updates can be enabled gradually
- **Fallback Support:** Can revert to full rebuild mode if needed

### Performance Benefits
- **Startup Time:** Faster system initialization with incremental updates
- **CPU Usage:** Significant reduction in ongoing CPU overhead
- **Memory Efficiency:** Better memory utilization patterns
- **Responsiveness:** Improved system responsiveness during operation

### Operational Changes
- **Monitoring:** New metrics for incremental update performance
- **Maintenance:** Reduced need for manual index maintenance
- **Scaling:** Better scaling characteristics for large installations
- **Reliability:** Improved reliability through reduced system stress

This optimized locker index management system provides the foundation for efficient coin collection storage and marketplace operations while dramatically improving performance and scalability compared to traditional full-rebuild approaches.