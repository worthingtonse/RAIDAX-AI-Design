
# Locker Services Implementation (cmd_locker)

## Module Purpose
This module implements a comprehensive digital locker system for coin storage, trading, and management within the RAIDA network. It provides secure storage for collections of coins, marketplace functionality for trading coin bundles, and multi-locker operations for batch processing. The system operates on an on-demand page cache database with incremental index updates for optimal performance.

## Core Functionality

### 1. Single Locker Storage (`cmd_store_sum`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 55 bytes) containing coin list + sum verification + locker key

**Returns:** None (modifies connection structure with status)

**Purpose:** Stores a collection of coins in a secure locker after cryptographic verification of ownership.

**Process:**
1. Validates minimum payload size and coin data alignment
2. Extracts coin list (5 bytes per coin: 1-byte denomination + 4-byte serial number)
3. Retrieves authentication numbers for all coins using database layer
4. Computes XOR sum of all authentication numbers
5. Verifies computed sum matches provided sum (cryptographic proof of ownership)
6. Validates locker key format (last 4 bytes must be 0xff)
7. Updates all coin authentication numbers with locker key
8. Adds coins to locker index for fast retrieval

**Dependencies:**
- Database layer for coin data access and modification
- Locker indexing system for incremental updates
- Cryptographic verification through XOR sum validation

### 2. Coin Removal from Locker (`cmd_remove`)
**Parameters:**
- Connection information structure  
- Input: Variable-length payload (minimum 55 bytes) containing locker key + coin specifications

**Returns:** None (modifies connection structure with status)

**Purpose:** Removes specified coins from a locker and transfers ownership to new authentication numbers.

**Process:**
1. Validates payload size and coin record alignment (21 bytes per coin)
2. Extracts locker authentication number from payload
3. Retrieves coin list from locker index
4. For each coin removal request:
   - Verifies coin exists in the specified locker
   - Updates coin authentication number with new owner data
   - Marks database page as modified
5. Updates locker index by removing transferred coins
6. Returns mixed status indicating partial success if applicable

**Dependencies:**
- Locker indexing system for coin lookups and updates
- Database layer for coin ownership transfer
- Utility functions for data extraction

### 3. Locker Contents Inspection (`cmd_peek`)
**Parameters:**
- Connection information structure
- Input: 34-byte payload containing locker authentication number

**Returns:** None (modifies connection structure with coin list)

**Purpose:** Retrieves the list of all coins stored in a specified locker without transferring ownership.

**Process:**
1. Validates exact payload size (34 bytes)
2. Extracts locker authentication number
3. Queries locker index for associated coins
4. Constructs response containing denomination and serial number for each coin
5. Returns complete coin inventory (5 bytes per coin)

**Dependencies:**
- Locker indexing system for coin enumeration
- Memory management for response buffer allocation

### 4. Trade Locker Creation (`cmd_put_for_sale`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 184 bytes) containing coins + verification + trade parameters

**Returns:** None (modifies connection structure with status)

**Purpose:** Creates a trade locker containing coins available for purchase, with specified currency type and pricing.

**Process:**
1. Validates minimum payload size and coin alignment
2. Extracts coin list and computes ownership verification sum
3. Validates trade currency type and special markers (bytes 14-15 must be 0xee)
4. Updates all coin authentication numbers with trade locker identifier
5. Adds coins to trade locker index with pricing information
6. Enables marketplace visibility for the coin bundle

**Dependencies:**
- Database layer for coin data modification
- Trade locker indexing system
- Currency type validation utilities

### 5. Marketplace Listing (`cmd_list_lockers_for_sale`)
**Parameters:**
- Connection information structure
- Input: 20-byte payload containing currency type and result limit

**Returns:** None (modifies connection structure with trade locker list)

**Purpose:** Lists available trade lockers for a specific currency type with pricing and value information.

**Process:**
1. Validates payload size and currency type
2. Queries trade locker index for matching currency
3. For each matching trade locker:
   - Calculates total coin value in the bundle
   - Extracts pricing information
   - Formats response record (29 bytes per trade locker)
4. Returns complete marketplace listing

**Dependencies:**
- Trade locker indexing system
- Coin value calculation utilities
- Endianness conversion for network transmission

### 6. Trade Locker Purchase (`cmd_buy`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload (minimum 96 bytes) containing buyer information and trade parameters

**Returns:** None (modifies connection structure with status)

**Purpose:** Executes purchase of a trade locker, transferring coin ownership from seller to buyer.

**Process:**
1. Validates payload size and extracts purchase parameters
2. Locates trade locker matching currency type, value, and price
3. Transfers coin ownership from seller to buyer locker
4. Updates both trade locker and regular locker indices
5. Removes completed trade from marketplace

**Dependencies:**
- Trade locker indexing system for transaction lookup
- Database layer for ownership transfer
- Dual index management for trade completion

### 7. Trade Locker Removal (`cmd_remove_trade_locker`)
**Parameters:**
- Connection information structure
- Input: Payload containing trade locker authentication number

**Returns:** None (modifies connection structure with status)

**Purpose:** Removes a trade locker from the marketplace and destroys the associated coins.

**Process:**
1. Retrieves trade locker information from index
2. Marks all associated coins as destroyed (sets MFS to 0)
3. Removes trade locker from index
4. Updates database to reflect coin destruction

**Dependencies:**
- Trade locker indexing system
- Database layer for coin destruction
- Coin lifecycle management

### 8. Trade Locker Inspection (`cmd_peek_trade_locker`)
**Parameters:**
- Connection information structure
- Input: Payload containing trade locker authentication number

**Returns:** None (modifies connection structure with coin list)

**Purpose:** Inspects contents of a trade locker without affecting its marketplace status.

**Process:**
1. Retrieves trade locker from index
2. Constructs coin list response
3. Returns denomination and serial number for each coin

**Dependencies:**
- Trade locker indexing system
- Memory management for response formatting

### 9. Multiple Locker Storage (`cmd_store_multiple_sum`)
**Parameters:**
- Connection information structure
- Input: Variable-length payload containing multiple locker definitions

**Returns:** None (modifies connection structure with per-locker status array)

**Purpose:** Processes multiple locker storage operations in a single request for batch efficiency.

**Process:**
1. Validates payload structure and extracts locker count
2. For each locker in the batch:
   - Validates coin count and data integrity
   - Performs ownership verification
   - Updates coin authentication numbers
   - Adds to locker index
3. Returns per-locker success/failure status
4. Sets overall status based on batch results

**Dependencies:**
- Database layer for batch coin processing
- Locker indexing system for multiple additions
- Batch transaction management

## Data Structures and Constants

### conn_info_t:
  - body_size: Integer (total request size)
  - command_status: Byte (response status code)
  - output: Byte array (response data)
  - output_size: Integer (response data length)

### Input Data Formats
- **Coin Record:** 5 bytes (1-byte denomination + 4-byte serial number)
- **Coin Removal Record:** 21 bytes (16-byte locker key + 5-byte coin identifier)
- **Trade Parameters:** Currency type, value, pricing information
- **Locker Verification:** 16-byte XOR sum + 16-byte locker authentication number

### Output Data Formats
- **Coin Inventory:** Array of 5-byte coin records
- **Trade Listing:** 29-byte records (16-byte AN + 1-byte currency + 8-byte value + 4-byte price)
- **Status Arrays:** Per-operation success/failure indicators

### Error Conditions
- **ERROR_INVALID_PACKET_LENGTH**: The request was too short or too long
- **ERROR_COINS_NOT_DIV**: Coin data in the payload is not correctly aligned
- **ERROR_INVALID_SN_OR_DENOMINATION**: Invalid serial number or denomination
- **ERROR_INVALID_PAN**: Invalid locker or trade locker authentication number
- **ERROR_INVALID_PARAMETER**: General parameter validation error
- **ERROR_MEMORY_ALLOC**: Server failed to allocate required memory
- **ERROR_INTERNAL**: Internal error (logic bug, failed assertion, etc.)
- **ERROR_TRADE_LOCKER_EXISTS**: A duplicate trade locker was detected
- **ERROR_TRADE_LOCKER_NOT_FOUND**: Requested trade locker not found
- **ERROR_INVALID_TRADE_COIN**: Unsupported or invalid currency type
- **ERROR_FILESYSTEM**: File system error while accessing or writing data
- **ERROR_NO_PRIVATE_KEY**: Required cryptographic key was not found on the server

### Status Codes
- `STATUS_ALL_PASS`: All operations successful
- `STATUS_ALL_FAIL`: All operations failed
- `STATUS_MIXED`: Partial success in batch operations
- `STATUS_WAITING`: Operation pending external verification (e.g., crypto transaction confirmation)


## Security and Validation

### Ownership Verification
- XOR sum validation proves possession of all coins in a collection
- Authentication number verification prevents unauthorized access
- Locker key format validation ensures proper access control

### Trade Security
- Currency type validation prevents invalid trading pairs
- Value calculation verification ensures accurate pricing
- Atomic transfer operations prevent partial transaction states

### Data Integrity
- Database page locking ensures consistent updates
- Index synchronization maintains data coherence
- Transaction rollback on validation failures

## Dependencies and Integration

### Required Modules
- **Database Layer:** On-demand page cache with locking for coin data access , Each coin operation involves acquiring a lock on the database page containing the       coin’sserial number before modification. Page modifications are followed by marking the page as dirty for persistence.
- **Locker Indexing System:** Fast lookup and modification of locker contents
- **Trade Locker Indexing:** Marketplace management and trade matching
- **Utilities Module:** Data extraction, endianness conversion, value calculations
- **Configuration System:** Server parameters and file paths
- **Statistics System:** Operation counting and performance metrics

### Used By
- **Client Applications:** Coin storage and trading operations
- **Marketplace Interface:** Trade locker listing and purchase
- **Batch Processing Systems:** Multiple locker operations

### External Constants Required
- `RECORDS_PER_PAGE`: Database organization constant
- `ERROR_*`: Protocol error definitions
- `STATUS_*`: Operation result indicators
- Currency type constants for trade validation

## Performance Optimizations

### Incremental Index Updates
- Locker index modifications use targeted updates instead of full rebuilds
- Trade locker index supports efficient marketplace queries
- Batch operations minimize database round trips

### Memory Management
- Dynamic allocation for variable-sized responses
- Efficient memory reuse in batch operations
- Proper cleanup on error conditions
- Always check result of malloc/realloc
- Free any allocated memory on all error paths
- Avoid memory leaks and double frees during partial failure

### Database Efficiency
- Page-level locking reduces contention
- On-demand loading minimizes memory usage
- Dirty page tracking optimizes synchronization

## Threading and Concurrency
- Operations execute within thread pool context
- Database page locking ensures thread safety
- Index operations use appropriate synchronization
- Atomic updates prevent race conditions


This module provides the foundation for secure coin storage and trading within the RAIDA ecosystem, enabling users to safely manage coin collections and participate in the marketplace economy.