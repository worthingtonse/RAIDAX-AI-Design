
#  Specification: Locker Index Implementation

## 1. Module Purpose
This module provides the concrete implementation for the in-memory Locker Index. It is responsible for the one-time initial build of the index and for providing thread-safe functions for incremental updates for both standard and trade lockers.

## 2. Data Model Summary

### 2.1 Core Data Structures

**Locker Indices**: Two separate, global, thread-safe collections (array/map) must be maintained:
- **locker_index**: Array of pointers to Index Entry Objects (size: MAX_LOCKER_RECORDS = 10000)
- **trade_locker_index**: Array of pointers to Index Entry Objects (size: MAX_LOCKER_RECORDS = 10000)

**Index Entry Object**:
- an: 16-byte array (Authenticity Number)
- coins: Dynamic array of Coin objects
- num_coins: Integer count of coins in the array

**Coin Object**:
- denomination: 8-bit signed integer (valid range: MIN_DENOMINATION to MAX_DENOMINATION)
- sn: 32-bit unsigned integer (serial number)

### 2.2 Locking Mechanism

Two separate mutexes:
- **locker_mtx**: Protects the standard locker index
- **trade_locker_mtx**: Protects the trade locker index

### 2.3 Constants
- **PREALLOCATE_COINS**: 100 (initial allocation size for coins array)
- **RECORDS_PER_PAGE**: Number of coin records per database page
- **TOTAL_PAGES**: Total number of pages per denomination
- **MIN_DENOMINATION, MAX_DENOMINATION**: Valid denomination range

## 3. Architectural Requirements
- **One-Time Build**: The indices must be fully populated from the primary database on disk only once when the server starts.
- **No Background Thread**: There should be no periodic rebuilding thread.
- **Incremental Updates**: All modifications after startup must be handled incrementally.
- **Concurrency**: Thread-safe operations using fine-grained locking per index type.
- **Memory Management**: Dynamic allocation with pre-allocation strategy for performance.

## 4. Core Logic and Implementation Details

### 4.1. initialize_locker_index()
**Purpose**: Initialize the index system at server startup.

**Logic**:
1. Initialize both index arrays to NULL pointers
2. Initialize both mutexes (locker_mtx, trade_locker_mtx)
3. Call build_initial_locker_indices()
4. Return 0 on success, -1 on failure

**Error Handling**: Log and return error if mutex initialization fails.

### 4.2. build_initial_locker_indices()
**Purpose**: One-time build of indices from disk database.

**Logic**:
1. Acquire both locks (locker_mtx and trade_locker_mtx)
2. Clear any existing index entries (call free_index() and free_trade_index())
3. For each denomination (MIN_DENOMINATION to MAX_DENOMINATION):
   - For each page (0 to TOTAL_PAGES-1):
     - Lock and retrieve the page from database
     - For each record in page (0 to RECORDS_PER_PAGE-1):
       - Calculate serial number: sn = page_number * RECORDS_PER_PAGE + record_index
       - Extract AN from record (offset: record_index * 17, length: 16 bytes)
       - Check AN format:
         - If bytes 12-15 are all 0xFF: Add to standard locker index
         - If byte 13 is valid trade type AND bytes 14-15 are 0xEE: Add to trade index
     - Unlock the page
4. Release both locks

**Trade Type Validation**: Valid types are SALE_TYPE_CC, SALE_TYPE_BTC, SALE_TYPE_XMR

### 4.3. locker_index_add_coins(an, coins_array, num_coins)
**Purpose**: Add coins to a standard locker.

**Parameters**:
- an: 16-byte array (locker identifier)
- coins_array: Array of Coin objects to add
- num_coins: Number of coins to add

**Logic**:
1. Acquire locker_mtx
2. For each coin in coins_array:
   - Call internal add_index_entry_internal(coin.denomination, coin.sn, an)
3. Release locker_mtx

### 4.4. trade_locker_index_add_coins(an, coins_array, num_coins)
**Purpose**: Add coins to a trade locker.

**Logic**: Same as 4.3 but uses trade_locker_mtx and add_trade_index_entry_internal()

### 4.5. Internal add_index_entry_internal(denomination, sn, an)
**Purpose**: Add a single coin to an index entry (mutex must be held by caller).

**Logic**:
1. Search index array for matching AN or first NULL slot
2. If new entry (NULL slot found):
   - Allocate new Index Entry Object
   - Allocate coins array with PREALLOCATE_COINS capacity
   - Set num_coins = 1
   - Copy AN and add first coin
3. If existing entry found:
   - Check if reallocation needed: (num_coins % PREALLOCATE_COINS) == 0
   - If yes:
     - Calculate new size: ((num_coins / PREALLOCATE_COINS) + 1) * PREALLOCATE_COINS
     - Allocate new array, copy old data, free old array
   - Add coin at index num_coins
   - Increment num_coins
4. Return 0 on success, -1 on failure

### 4.6. locker_index_remove_coins(an, coins_array, num_coins)
**Purpose**: Remove coins from a standard locker.

**Logic**:
1. Acquire locker_mtx
2. Find index entry with matching AN
3. If not found, release lock and return
4. For each coin to remove:
   - Linear search through entry's coins array
   - If found, shift remaining elements left to remove gap
   - Decrement num_coins
5. If num_coins reaches 0:
   - Free coins array
   - Free index entry
   - Set index slot to NULL
6. Release locker_mtx

### 4.7. trade_locker_index_remove_coins(an, coins_array, num_coins)
**Logic**: Same as 4.6 but uses trade_locker_mtx and trade_locker_index

### 4.8. Query Functions

#### get_coins_from_index(an) / get_coins_from_trade_index(an)
**Returns**: Pointer to Index Entry Object or NULL

**Logic**:
1. Acquire appropriate lock
2. Linear search through index for matching AN (compare all 16 bytes)
3. Store result pointer
4. Release lock
5. Return result

#### get_coins_from_index_by_prefix(an_prefix)
**Purpose**: Find entry matching first 5 bytes of AN (for encryption scenarios)

**Logic**: Same as above but compare only first 5 bytes

#### load_coins_from_trade_index(coin_type, max_results, output_array)
**Purpose**: Get up to max_results trade lockers of specified type

**Logic**:
1. Acquire trade_locker_mtx
2. Iterate through trade_locker_index
3. If entry's AN[13] matches coin_type, add to output_array
4. Stop when max_results reached or end of index
5. Release lock
6. Return actual count found

#### get_entry_from_trade_index(coin_type, amount, price)
**Purpose**: Find specific trade locker by criteria

**Logic**:
1. Acquire trade_locker_mtx
2. For each entry in trade_locker_index:
   - Check if AN[13] matches coin_type
   - Extract price from AN[9:12] (4 bytes, big-endian)
   - Calculate total value by summing coin values
   - If all match, store result and break
3. Release lock
4. Return result

### 4.9. Utility Functions

#### calc_coins_in_trade_locker(index_entry)
**Purpose**: Calculate total value of coins in a trade locker

**Logic**:
1. Initialize sum to 0
2. For each coin in entry:
   - Add coin_value(coin.denomination, coin.sn) to sum
3. Return sum

#### free_index() / free_trade_index()
**Purpose**: Clean up all index entries (mutex must be held by caller)

**Logic**:
1. For each non-NULL entry in index:
   - Free coins array
   - Free index entry
   - Set slot to NULL
2. Log number of entries freed

## 5. Error Handling

### 5.1 Memory Allocation Failures
- All malloc/realloc calls must be checked
- On failure: Log error with context, clean up partial allocations, return error code
- Never leave dangling pointers or partially initialized structures

### 5.2 Database Access Errors
- If page retrieval fails during initial build: Log error, skip that page, continue
- System should be resilient to partial data

### 5.3 Concurrency Safety
- Lock acquisition order must be consistent to prevent deadlocks
- When both locks needed: Always acquire in order (locker_mtx, then trade_locker_mtx)
- All lock acquisitions must have corresponding releases on all code paths

### 5.4 Input Validation
- Validate denomination is within valid range
- Validate coin type for trade operations
- Check array bounds before access
- Validate AN format for locker type

## 6. Performance Considerations

### 6.1 Memory Management
- Pre-allocate in chunks of PREALLOCATE_COINS to reduce reallocation frequency
- Use array-based storage for O(1) indexed access where possible

### 6.2 Search Optimization
- Linear search is acceptable for MAX_LOCKER_RECORDS = 10000
- Consider upgrading to hash table if performance becomes an issue

### 6.3 Lock Granularity
- Separate locks for standard and trade indices allow concurrent operations
- Hold locks for minimum time necessary
- Avoid nested function calls while holding locks

## 7. Thread Safety Contract
- All public functions are thread-safe
- Internal functions ending in _internal require caller to hold appropriate lock
- No function should return while holding a lock
- Index Entry Objects returned by query functions must not be modified by callers