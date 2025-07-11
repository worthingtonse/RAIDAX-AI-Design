<!-- 
#  Specification: Locker Command Handlers

## 1. Module Purpose
This module implements the business logic for all commands related to coin lockers. These handlers interact with the primary database, validate client requests, and ensure the in-memory Locker Index is kept perfectly in sync with the state of the database.

## 2. Architectural Requirements

### 2.1 Data Flow Order
- **Primary Data Source**: All state changes must be written to the primary database first
- **Asynchronous Persistence**: Modified pages must be queued for disk write via add_page_to_dirty_queue()
- **Index Synchronization**: In-memory index updates occur only after successful database modifications

### 2.2 Transaction Integrity
- Database modifications must be atomic per command
- Index updates must match database state exactly
- Partial failures must not corrupt either database or index

## 3. Common Data Structures

### 3.1 Request/Response Structure
```
conn_info_t:
  - body_size: Integer (total request size)
  - command_status: Byte (response status code)
  - output: Byte array (response data)
  - output_size: Integer (response data length)
```

### 3.2 Status Codes
- **STATUS_ALL_PASS**: All operations successful
- **STATUS_ALL_FAIL**: All operations failed
- **STATUS_MIXED**: Partial success (batch operations)
- **STATUS_WAITING**: Operation pending external verification
- **ERROR_INVALID_PACKET_LENGTH**: Malformed request
- **ERROR_COINS_NOT_DIV**: Coin data not properly aligned
- **ERROR_INVALID_SN_OR_DENOMINATION**: Invalid coin identifier
- **ERROR_INVALID_PAN**: Invalid proposed AN format
- **ERROR_INVALID_PARAMETER**: General parameter error
- **ERROR_MEMORY_ALLOC**: Memory allocation failure
- **ERROR_INTERNAL**: Unexpected internal error
- **ERROR_TRADE_LOCKER_EXISTS**: Duplicate trade locker
- **ERROR_TRADE_LOCKER_NOT_FOUND**: Trade locker not found
- **ERROR_INVALID_TRADE_COIN**: Invalid currency type
- **ERROR_FILESYSTEM**: File I/O error
- **ERROR_NO_PRIVATE_KEY**: Missing crypto key

### 3.3 Constants
- Coin record size: 17 bytes (16 AN + 1 MFS)
- Standard locker AN suffix: 0xFF 0xFF 0xFF 0xFF
- Trade locker AN suffix: 0xEE 0xEE
- Trade type position: AN[13]
- Price position: AN[9:12] (4 bytes, big-endian)

## 4. Core Command Implementations

### 4.1. cmd_store_sum
**Purpose**: Creates or adds coins to a standard locker.

**Request Format**:
```
[16 bytes CH] [5 bytes per coin (DN + 4 SN)] [16 bytes SU] [16 bytes PN] [2 bytes EOF]
Minimum size: 55 bytes
```

**Validation Steps**:
- Verify request size >= 55 bytes
- Calculate coin count: (body_size - 50) / 5
- Verify coin data is divisible by 5
- Verify PN ends with 0xFF 0xFF 0xFF 0xFF

**Core Logic**:
1. Allocate array for coins to be added
2. Calculate XOR sum of current ANs:
   - For each coin:
     - Extract denomination (1 byte) and SN (4 bytes)
     - Lock database page containing coin
     - XOR the coin's current AN into running sum
     - Store coin info for later use
     - Unlock page
3. Compare calculated XOR with provided SU
4. If authentic:
   - For each coin:
     - Lock database page
     - Update coin's AN to PN
     - Set MFS byte
     - Call add_page_to_dirty_queue(page)
     - Increment statistics
     - Unlock page
5. Call locker_index_add_coins(PN, coins_array, count)
6. Set status to STATUS_ALL_PASS or STATUS_ALL_FAIL

**Error Handling**:
- Free allocated memory on any error
- Return appropriate error code
- Do not update index if database update fails

### 4.2. cmd_remove
**Purpose**: Removes specific coins from a standard locker.

**Request Format**:
```
[16 bytes CH] [16 bytes AN] [21 bytes per coin (DN + 4 SN + 16 CAN)] [2 bytes EOF]
Minimum size: 55 bytes
```

**Validation Steps**:
- Verify request size >= 55 bytes
- Calculate coin count: (body_size - 34) / 21
- Verify coin data is divisible by 21

**Core Logic**:
1. Call get_coins_from_index(AN) to get locker contents
2. If locker not found, return STATUS_ALL_FAIL
3. Allocate output buffer: (coin_count / 8) + 1 bytes for bitmap
4. For each coin to remove:
   - Extract DN, SN, and new CAN from request
   - Verify coin exists in locker (search index entry)
   - If found:
     - Lock database page
     - Update coin's AN to CAN
     - Set MFS byte
     - Call add_page_to_dirty_queue(page)
     - Set success bit in output bitmap
     - Add to removal list
     - Unlock page
   - If not found, increment failure count
5. Call locker_index_remove_coins(AN, removal_list, removal_count)
6. Set appropriate status based on success/failure counts

**Response Format**:
- Bitmap where bit N indicates success (1) or failure (0) for coin N

### 4.3. cmd_peek
**Purpose**: Retrieves coins in a standard locker (read-only).

**Request Format**:
```
[16 bytes CH] [16 bytes AN] [2 bytes EOF]
Size: exactly 34 bytes
```

**Core Logic**:
1. Validate request size == 34
2. Call get_coins_from_index(AN)
3. If not found, return STATUS_ALL_FAIL
4. Allocate output buffer: num_coins * 5 bytes
5. For each coin in index entry:
   - Write denomination (1 byte)
   - Write SN (4 bytes, big-endian)
6. Set status to STATUS_ALL_PASS

**Response Format**:
```
[5 bytes per coin (DN + 4 SN)]
```

### 4.4. cmd_put_for_sale
**Purpose**: Creates a trade locker for selling coins.

**Request Format**:
```
[16 bytes CH] [5 bytes per coin] [16 bytes SU] [16 bytes PN] 
[1 byte AS] [128 bytes AD] [2 bytes EOF]
Minimum size: 184 bytes
```

**Special PN Format**:
- PN[9:12]: Price (4 bytes, big-endian)
- PN[13]: Trade currency type
- PN[14:15]: Must be 0xEE 0xEE

**Validation Steps**:
- Verify request size >= 184 bytes
- Calculate coin count: (body_size - 179) / 5
- Verify PN[14:15] == 0xEE 0xEE
- Verify trade currency type is valid
- Verify address size (AS) is between 16-120 bytes
- Check no duplicate trade locker exists (same type, count, price)

**Core Logic**:
1. Calculate XOR sum and total value of coins
2. If authentic (XOR matches):
   - Generate filesystem path from PN
   - Write crypto address to file
   - For each coin:
     - Update AN to PN in database
     - Queue page for persistence
3. Call trade_locker_index_add_coins(PN, coins, count)

**Filesystem Storage**:
- Path: {config.cwd}/Trades/{PN_hex_string}
- Content: Crypto address (AS bytes)

### 4.5. cmd_list_lockers_for_sale
**Purpose**: Lists available trade lockers.

**Request Format**:
```
[16 bytes CH] [1 byte CT] [1 byte NR] [2 bytes EOF]
Size: exactly 20 bytes
```

**Core Logic**:
1. Validate currency type (CT)
2. Allocate array for NR index entries
3. Call load_coins_from_trade_index(CT, NR, array)
4. For each returned entry:
   - Verify filesystem path exists
   - Calculate total value via calc_coins_in_trade_locker()
   - Add to output: [1 byte CT] [8 bytes value] [4 bytes price]

**Response Format**:
```
[13 bytes per locker: CT + value + price]
```

### 4.6. cmd_buy
**Purpose**: Transfers trade locker ownership with payment verification.

**Request Format**:
```
[16 bytes CH] [16 bytes LK] [1 byte CT] [8 bytes AM] [4 bytes PRICE]
[32 bytes TXID] [16 bytes Receipt] [variable MEMO] [2 bytes EOF]
Minimum size: 96 bytes
```

**Validation Steps**:
- Verify LK ends with 0xFF 0xFF 0xFF 0xFF
- Verify trade currency type
- Calculate memo size: body_size - 94
- Verify memo size <= 254

**Complex Logic Flow**:
1. Find trade locker: get_entry_from_trade_index(CT, AM, PRICE)
2. Verify all coins still exist in database
3. Read seller's crypto address from filesystem
4. **Payment Verification (critical)**:
   - Get crypto private key for currency type
   - Build verification request:
     - Currency type
     - Receipt ID
     - Private key
     - Price
     - Seller address
     - Transaction ID
   - Send to external proxy: proxy_request(CMD_PROXY_SEND_TRANSACTION, ...)
   - Handle response:
     - STATUS_WAITING: Return immediately (async verification)
     - STATUS_SUCCESS: Continue with transfer
     - Other: Return error
5. **Transfer ownership**:
   - For each coin in trade locker:
     - Update AN to buyer's LK
     - Queue page for persistence
6. **Update indices atomically**:
   - trade_locker_index_remove_coins(seller_AN, coins, count)
   - locker_index_add_coins(buyer_LK, coins, count)

**External Dependencies**:
- Crypto key storage system
- Payment verification proxy
- Filesystem for address storage

### 4.7. cmd_store_multiple_sum
**Purpose**: Batch operation to create multiple lockers.

**Request Format**:
```
[16 bytes CH] [1 byte NL] 
For each locker:
  [2 bytes NC] [5 bytes per coin] [16 bytes SU] [16 bytes PN]
[2 bytes EOF]
Minimum size: 19 bytes
```

**Validation Steps**:
- Verify NL > 0
- Pre-validate total size for all lockers
- Each locker must have valid structure

**Core Logic**:
1. Allocate output bitmap: (NL / 8) + 1 bytes
2. For each locker:
   - Perform full cmd_store_sum logic
   - Track success/failure in bitmap
   - Continue on individual locker failure
3. Set status based on results:
   - All success: STATUS_ALL_PASS
   - All fail: STATUS_ALL_FAIL
   - Mixed: STATUS_MIXED (include bitmap in response)

**Response Format**:
- Bitmap where bit N indicates success for locker N

### 4.8. cmd_remove_trade_locker
**Purpose**: Removes coins from a trade locker.

**Logic**: Identical to cmd_remove but:
- Uses get_coins_from_trade_index() instead of regular index
- Updates trade index via trade_locker_index_remove_coins()

### 4.9. cmd_peek_trade_locker
**Purpose**: Views coins in a trade locker.

**Logic**: Identical to cmd_peek but uses get_coins_from_trade_index()

## 5. Critical Implementation Notes

### 5.1 Database Page Management
For each page modification:
1. page = get_page_by_sn_lock(denomination, serial_number)
2. Modify page data
3. add_page_to_dirty_queue(page)
4. unlock_page(page)

### 5.2 Serial Number Encoding
- **Storage**: 4 bytes big-endian
- **Functions**: get_sn(bytes) to decode, put_sn(value, bytes) to encode

### 5.3 Statistics Tracking
After each successful coin update:
- inc_stat(POWN_FIELD_IDX, 1)
- inc_stat(POWN_VALUE_FIELD_IDX, get_den_value(denomination))

### 5.4 Memory Safety
- Always free allocated memory on all code paths
- Check all allocations before use
- Clean up partial operations on failure

### 5.5 Index Consistency
- Never update index before database
- Group related index updates together
- Ensure atomicity of paired operations (e.g., remove from one index, add to another)

## 6. Error Recovery

### 6.1 Partial Operation Failure
- In batch operations, continue processing remaining items
- Track individual successes/failures
- Return accurate status reflecting partial success

### 6.2 External Service Failure
- Payment proxy timeout: Return STATUS_WAITING
- Filesystem errors: Log and return ERROR_FILESYSTEM
- Missing crypto keys: Return ERROR_NO_PRIVATE_KEY

### 6.3 Database Inconsistency
- If page lock fails after initial success: Log error, attempt rollback
- If index update fails: Log critical error (indicates programming bug)
- Never leave database and index out of sync

## 7. Performance Considerations

### 7.1 Batch Processing
- Pre-allocate arrays for known sizes
- Minimize page locks held simultaneously
- Group sequential page accesses where possible

### 7.2 Index Operations
- Perform bulk index updates in single call
- Avoid repeated index lookups for same AN
- Cache index results within command when safe

### 7.3 I/O Optimization
- Queue dirty pages instead of synchronous writes
- Batch filesystem operations where possible
- Minimize crypto key file reads -->




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