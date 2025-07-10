# Language-Agnostic Specification: Locker Header Interface

## 1. Module Purpose
This specification defines the public interface for the CloudCoin locker system. It establishes the data structures, constants, and function signatures required to implement an in-memory index system for both standard coin lockers and trade lockers with thread-safe incremental updates.

## 2. System Constants

### 2.1 Index Configuration
```
MAX_LOCKER_RECORDS = 100000      // Maximum locker records in index
INDEX_UPDATE_PERIOD = 3600       // Legacy constant (no longer used for periodic rebuilds)
PREALLOCATE_COINS = 2           // Initial allocation size for coins array
```

### 2.2 Trade Sale Types
```
SALE_TYPE_CC = 0x0              // CloudCoin trade type
SALE_TYPE_BTC = 0x1             // Bitcoin trade type
SALE_TYPE_XMR = 0x2             // Monero trade type
```

## 3. Core Data Structures

### 3.1 Index Entry Structure
The index entry structure represents a single locker and must contain:

```
index_entry_structure:
    an: byte_array[16]                    // Authenticity Number (locker identifier)
    num_coins: integer                    // Number of coins in the locker
    coins: pointer_to_coin_array          // Dynamic array of coin structures
```

### 3.2 Coin Structure Dependency
The locker system depends on the coin structure from the protocol layer:
```
coin_structure:
    denomination: signed_8bit_integer     // Coin denomination
    sn: unsigned_32bit_integer           // Serial number
```

## 4. Public Function Interface

### 4.1 Initialization Functions

#### 4.1.1 init_locker_index()
```
Function: init_locker_index
Parameters: none
Returns: integer (0 = success, negative = error)
Purpose: Initialize the locker index system at server startup
```

#### 4.1.2 build_initial_locker_indices()
```
Function: build_initial_locker_indices
Parameters: none
Returns: void
Purpose: Perform one-time build of indices from database
```

### 4.2 Standard Locker Index Functions

#### 4.2.1 locker_index_add_coins(an, coins_to_add, num_coins)
```
Function: locker_index_add_coins
Parameters:
    an: pointer_to_byte_array (16-byte locker identifier)
    coins_to_add: pointer_to_coin_array
    num_coins: integer (number of coins to add)
Returns: void
Purpose: Add coins to a standard locker index entry
Thread Safety: Thread-safe
```

#### 4.2.2 locker_index_remove_coins(an, coins_to_remove, num_coins)
```
Function: locker_index_remove_coins
Parameters:
    an: pointer_to_byte_array (16-byte locker identifier)
    coins_to_remove: pointer_to_coin_array
    num_coins: integer (number of coins to remove)
Returns: void
Purpose: Remove coins from a standard locker index entry
Thread Safety: Thread-safe
```

### 4.3 Trade Locker Index Functions

#### 4.3.1 trade_locker_index_add_coins(an, coins_to_add, num_coins)
```
Function: trade_locker_index_add_coins
Parameters:
    an: pointer_to_byte_array (16-byte locker identifier)
    coins_to_add: pointer_to_coin_array
    num_coins: integer (number of coins to add)
Returns: void
Purpose: Add coins to a trade locker index entry
Thread Safety: Thread-safe
```

#### 4.3.2 trade_locker_index_remove_coins(an, coins_to_remove, num_coins)
```
Function: trade_locker_index_remove_coins
Parameters:
    an: pointer_to_byte_array (16-byte locker identifier)
    coins_to_remove: pointer_to_coin_array
    num_coins: integer (number of coins to remove)
Returns: void
Purpose: Remove coins from a trade locker index entry
Thread Safety: Thread-safe
```

### 4.4 Index Cleanup Functions

#### 4.4.1 free_index()
```
Function: free_index
Parameters: none
Returns: void
Purpose: Free all resources used by standard locker index
```

#### 4.4.2 free_trade_index()
```
Function: free_trade_index
Parameters: none
Returns: void
Purpose: Free all resources used by trade locker index
```

#### 4.4.3 show_index()
```
Function: show_index
Parameters: none
Returns: void
Purpose: Display debug information about standard locker index
```

#### 4.4.4 show_trade_index()
```
Function: show_trade_index
Parameters: none
Returns: void
Purpose: Display debug information about trade locker index
```

### 4.5 Query Functions

#### 4.5.1 get_coins_from_index(an)
```
Function: get_coins_from_index
Parameters: an (pointer_to_byte_array - 16-byte locker identifier)
Returns: pointer_to_index_entry_structure (NULL if not found)
Purpose: Retrieve standard locker entry by full AN
Thread Safety: Thread-safe
```

#### 4.5.2 get_coins_from_index_by_prefix(an_prefix)
```
Function: get_coins_from_index_by_prefix
Parameters: an_prefix (pointer_to_byte_array - partial locker identifier)
Returns: pointer_to_index_entry_structure (NULL if not found)
Purpose: Retrieve standard locker entry by AN prefix (first 5 bytes)
Thread Safety: Thread-safe
```

#### 4.5.3 get_coins_from_trade_index(an)
```
Function: get_coins_from_trade_index
Parameters: an (pointer_to_byte_array - 16-byte locker identifier)
Returns: pointer_to_index_entry_structure (NULL if not found)
Purpose: Retrieve trade locker entry by full AN
Thread Safety: Thread-safe
```

#### 4.5.4 load_coins_from_trade_index(coin_type, max_results, output_array)
```
Function: load_coins_from_trade_index
Parameters:
    coin_type: unsigned_8bit_integer (trade currency type)
    max_results: unsigned_8bit_integer (maximum entries to return)
    output_array: pointer_to_pointer_to_index_entry_structure
Returns: integer (actual number of entries found)
Purpose: Get multiple trade locker entries of specified type
Thread Safety: Thread-safe
```

#### 4.5.5 get_entry_from_trade_index(coin_type, amount, price)
```
Function: get_entry_from_trade_index
Parameters:
    coin_type: unsigned_8bit_integer (trade currency type)
    amount: unsigned_64bit_integer (total value)
    price: unsigned_32bit_integer (price per unit)
Returns: pointer_to_index_entry_structure (NULL if not found)
Purpose: Find specific trade locker by criteria
Thread Safety: Thread-safe
```

### 4.6 Utility Functions

#### 4.6.1 is_good_trade_coin_type(coin_type)
```
Function: is_good_trade_coin_type
Parameters: coin_type (unsigned_8bit_integer)
Returns: integer (non-zero if valid, 0 if invalid)
Purpose: Validate trade currency type
```

#### 4.6.2 calc_coins_in_trade_locker(index_entry)
```
Function: calc_coins_in_trade_locker
Parameters: index_entry (pointer_to_index_entry_structure)
Returns: unsigned_64bit_integer (total value)
Purpose: Calculate total value of coins in trade locker
```

## 5. Data Structure Requirements

### 5.1 Index Entry Memory Layout
- **an field**: 16 consecutive bytes storing locker identifier
- **num_coins field**: Integer tracking current coin count
- **coins field**: Pointer to dynamically allocated coin array

### 5.2 Dynamic Memory Management
- Coin arrays must be dynamically allocated and resizable
- Initial allocation should use PREALLOCATE_COINS constant
- Memory must be freed when index entries are removed
- Reallocation should occur in chunks for efficiency

## 6. Thread Safety Requirements

### 6.1 Concurrent Access Protection
- All public functions must be thread-safe
- Internal synchronization must protect both indices independently
- Query functions must return stable references during concurrent modifications
- Add/remove operations must be atomic per function call

### 6.2 Index Separation
- Standard locker index and trade locker index must use separate synchronization
- Concurrent operations on different index types should not block each other
- Lock granularity should allow maximum concurrency

## 7. Integration Dependencies

### 7.1 Protocol Layer Integration
```
Required from protocol.h:
    coin_t structure definition
    Basic coin type definitions
```

### 7.2 Memory Management Requirements
- All dynamic allocations must be tracked and freed appropriately
- Index entries must handle memory allocation failures gracefully
- Partial operations must clean up allocated resources on failure

## 8. Trade System Specifications

### 8.1 Trade Type Validation
- Only SALE_TYPE_CC, SALE_TYPE_BTC, and SALE_TYPE_XMR are valid
- Trade type validation must occur before index operations
- Invalid trade types must be rejected with appropriate error handling

### 8.2 Trade Locker Identification
- Trade lockers identified by specific AN format patterns
- AN bytes 14-15 must be 0xEE 0xEE for trade lockers
- Standard lockers identified by AN bytes 12-15 being 0xFF 0xFF 0xFF 0xFF

## 9. Performance Considerations

### 9.1 Search Efficiency
- Index lookups should be optimized for the MAX_LOCKER_RECORDS limit
- Prefix matching should be efficient for encryption key lookups
- Trade locker filtering by type should minimize full index scans

### 9.2 Memory Efficiency
- Coin array preallocation should balance memory usage and reallocation overhead
- Index entries should be packed efficiently in memory
- Unused index slots should not consume excessive memory

## 10. Error Handling Standards

### 10.1 Function Return Conventions
- Query functions return NULL for not found conditions
- Integer return functions use 0 for success, negative for errors
- Utility functions return appropriate typed values with error indicators

### 10.2 Resource Cleanup
- All functions must clean up resources on error paths
- Partial index updates must not leave indices in inconsistent state
- Memory leaks must be prevented on all error conditions


This specification provides the complete interface definition needed to implement the CloudCoin locker index system while remaining language-agnostic and accurately reflecting the actual header requirements.